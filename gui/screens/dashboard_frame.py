import tkinter as tk
from tkinter import ttk, messagebox

class DashboardFrame(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        
        # --- Tiêu đề ---
        tk.Label(self, text="HỆ THỐNG ĐẤU GIÁ - DANH SÁCH PHÒNG", 
                 font=("Arial", 16, "bold"), fg="#333").pack(pady=15)

        # --- Thanh công cụ (Toolbar) ---
        toolbar = tk.Frame(self)
        toolbar.pack(fill="x", padx=20, pady=5)
        
        tk.Button(toolbar, text="🔄 Làm mới", font=("Arial", 10),
                  command=self.refresh_rooms).pack(side="left", padx=5)
        
        # --- Thanh tìm kiếm vật phẩm ---
        tk.Label(toolbar, text="🔍 Tìm vật phẩm:", font=("Arial", 10)).pack(side="left", padx=(15, 2))
        self.ent_search = tk.Entry(toolbar, width=20, font=("Arial", 10))
        self.ent_search.pack(side="left", padx=2)
        self.ent_search.bind("<Return>", lambda e: self.search_item())
        
        tk.Button(toolbar, text="Tìm", font=("Arial", 10, "bold"), 
                  bg="#f0f0f0", command=self.search_item).pack(side="left", padx=2)
        
        # Chỉ hiển thị nút "Tạo phòng" nếu người dùng là AUCTIONEER (Role 2)
        if self.controller.user_role == 2:
            tk.Button(toolbar, text="➕ Tạo phòng mới", font=("Arial", 10),
                      bg="#E3F2FD", command=self.open_create_room).pack(side="left", padx=15)
        
        # Nút Đăng xuất - Cập nhật callback để tránh lỗi show_frame
        tk.Button(toolbar, text="🚪 Đăng xuất", font=("Arial", 10, "bold"),
                  fg="white", bg="#e74c3c", command=self.on_logout).pack(side="right", padx=5)

        tk.Button(toolbar, text="📜 Xem lịch sử", font=("Arial", 10),
                  command=self.view_history).pack(side="right", padx=5)

        # --- Bảng danh sách phòng (Treeview) ---
        style = ttk.Style()
        style.configure("Treeview.Heading", font=("Arial", 10, "bold"))
        
        columns = ("id", "room_name", "owner", "items")
        self.tree = ttk.Treeview(self, columns=columns, show="headings", height=15)
        
        self.tree.heading("id", text="ID")
        self.tree.heading("room_name", text="Tên phòng đấu giá")
        self.tree.heading("owner", text="Chủ phòng")
        self.tree.heading("items", text="Số vật phẩm")
        
        self.tree.column("id", width=60, anchor="center")
        self.tree.column("room_name", width=300, anchor="w")
        self.tree.column("owner", width=180, anchor="center")
        self.tree.column("items", width=120, anchor="center")
        
        # Scrollbar cho bảng
        scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        self.tree.pack(fill="both", expand=True, padx=20, pady=10)
        scrollbar.place(in_=self.tree, relx=1.0, relheight=1.0, bordermode="outside")
        
        self.tree.bind("<Double-1>", self.on_item_double_click)

        # --- Nút hành động chính ---
        self.btn_join = tk.Button(self, text="THAM GIA PHÒNG ĐẤU GIÁ", 
                                  font=("Arial", 11, "bold"), bg="#4CAF50", fg="white",
                                  padx=20, pady=10, command=self.join_selected_room)
        self.btn_join.pack(pady=15)

    # --- LOGIC XỬ LÝ ---

    def on_logout(self):
        """Xử lý đăng xuất (Mã 103) - Đã sửa lỗi gọi hàm chuyển màn hình"""
        if messagebox.askyesno("Xác nhận", "Bạn có chắc chắn muốn đăng xuất?"):
            # Gửi yêu cầu đăng xuất tới server (C2S_LOGOUT = 103)
            self.controller.backend.send_command({"type": 103})
            # SỬA LỖI: Gọi đúng hàm show_login_screen() trong main.py thay vì show_frame()
            self.controller.show_login_screen()

    def refresh_rooms(self):
        self.controller.backend.send_command({"type": 201})

    def search_item(self):
        keyword = self.ent_search.get().strip()
        if not keyword:
            messagebox.showwarning("Chú ý", "Vui lòng nhập từ khóa tìm kiếm!")
            return
        self.controller.backend.send_command({"type": 205, "keyword": keyword})

    def update_room_list(self, rooms_data):
        """Cập nhật dữ liệu vào bảng (S2C_ROOM_LIST = 810)"""
        for item in self.tree.get_children():
            self.tree.delete(item)
            
        for room in rooms_data:
            name = room.get("room_name") or f"Phòng đấu giá #{room.get('id')}"
            queue_len = len(room.get("queue", []))
            
            self.tree.insert("", "end", values=(
                room.get("id"),
                name,
                room.get("owner", "N/A"),
                f"{queue_len} món"
            ))

    def get_selected_room_id(self):
        selected = self.tree.selection()
        if not selected:
            messagebox.showwarning("Chú ý", "Vui lòng chọn một phòng từ danh sách!")
            return None
        return self.tree.item(selected[0])['values'][0]

    def join_selected_room(self):
        room_id = self.get_selected_room_id()
        if room_id:
            selected_item = self.tree.item(self.tree.selection()[0])
            # Lưu tên phòng vào controller để hiển thị ở màn hình Room
            self.controller.current_room_name = selected_item['values'][1]
            
            self.controller.backend.send_command({
                "type": 203, 
                "room_id": int(room_id)
            })

    def on_item_double_click(self, event):
        self.join_selected_room()

    def view_history(self):
        self.controller.backend.send_command({"type": 501})

    def open_create_room(self):
        """Mở popup tạo phòng với trường Tên phòng mới"""
        create_win = tk.Toplevel(self)
        create_win.title("Tạo phòng đấu giá mới")
        create_win.geometry("380x450") 
        create_win.grab_set()

        main_frame = tk.Frame(create_win, padx=25, pady=20)
        main_frame.pack(fill="both", expand=True)

        tk.Label(main_frame, text="Tên phòng đấu giá:", font=("Arial", 10, "bold")).pack(anchor="w")
        ent_room_name = tk.Entry(main_frame, font=("Arial", 11))
        ent_room_name.pack(fill="x", pady=(5, 15))
        ent_room_name.insert(0, "Phiên đấu giá số 1") 
        ent_room_name.focus_set()

        tk.Label(main_frame, text="Tên vật phẩm đầu tiên:", font=("Arial", 10)).pack(anchor="w")
        ent_title = tk.Entry(main_frame, font=("Arial", 11))
        ent_title.pack(fill="x", pady=(5, 15))

        tk.Label(main_frame, text="Giá khởi điểm (VND):", font=("Arial", 10)).pack(anchor="w")
        ent_price = tk.Entry(main_frame, font=("Arial", 11))
        ent_price.pack(fill="x", pady=(5, 15))

        tk.Label(main_frame, text="Giá mua ngay (VND):", font=("Arial", 10)).pack(anchor="w")
        ent_buy = tk.Entry(main_frame, font=("Arial", 11))
        ent_buy.pack(fill="x", pady=(5, 15))

        def submit():
            r_name = ent_room_name.get().strip()
            title = ent_title.get().strip()
            p_str = ent_price.get().strip()
            b_str = ent_buy.get().strip()

            if not r_name or not title or not p_str or not b_str:
                messagebox.showerror("Lỗi", "Vui lòng nhập đầy đủ các trường!")
                return

            try:
                price = int(p_str)
                buy = int(b_str)
                
                # Gửi kèm trường room_name lên server (Mã 202)
                self.controller.backend.send_command({
                    "type": 202,
                    "room_name": r_name,
                    "title": title,
                    "start_price": price,
                    "buy_now": buy
                })
                create_win.destroy()
                self.after(500, self.refresh_rooms)
                
            except ValueError:
                messagebox.showerror("Lỗi", "Giá tiền phải là số nguyên!")

        tk.Button(main_frame, text="XÁC NHẬN TẠO PHÒNG", bg="#4CAF50", fg="white", 
                  font=("Arial", 10, "bold"), pady=10, command=submit).pack(fill="x", pady=10)