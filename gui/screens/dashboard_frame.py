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
        
        # --- BỔ SUNG: Thanh tìm kiếm vật phẩm ---
        tk.Label(toolbar, text="🔍 Tìm vật phẩm:", font=("Arial", 10)).pack(side="left", padx=(15, 2))
        self.ent_search = tk.Entry(toolbar, width=20, font=("Arial", 10))
        self.ent_search.pack(side="left", padx=2)
        # Cho phép nhấn Enter để tìm kiếm nhanh
        self.ent_search.bind("<Return>", lambda e: self.search_item())
        
        tk.Button(toolbar, text="Tìm", font=("Arial", 10, "bold"), 
                  bg="#f0f0f0", command=self.search_item).pack(side="left", padx=2)
        
        # Chỉ hiển thị nút "Tạo phòng" nếu người dùng là AUCTIONEER (Role 2)
        if self.controller.user_role == 2:
            tk.Button(toolbar, text="➕ Tạo phòng mới", font=("Arial", 10),
                      bg="#E3F2FD", command=self.open_create_room).pack(side="left", padx=15)
        
        tk.Button(toolbar, text="📜 Xem lịch sử", font=("Arial", 10),
                  command=self.view_history).pack(side="right", padx=5)

        # --- Bảng danh sách phòng (Treeview) ---
        style = ttk.Style()
        style.configure("Treeview.Heading", font=("Arial", 10, "bold"))
        
        columns = ("id", "owner", "current_price", "items")
        self.tree = ttk.Treeview(self, columns=columns, show="headings", height=15)
        
        self.tree.heading("id", text="ID Phòng")
        self.tree.heading("owner", text="Chủ phòng (Auctioneer)")
        self.tree.heading("current_price", text="Giá hiện tại")
        self.tree.heading("items", text="Số vật phẩm")
        
        self.tree.column("id", width=100, anchor="center")
        self.tree.column("owner", width=200, anchor="center")
        self.tree.column("current_price", width=150, anchor="e")
        self.tree.column("items", width=120, anchor="center")
        
        # Scrollbar cho bảng
        scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        self.tree.pack(fill="both", expand=True, padx=20, pady=10)
        scrollbar.pack(side="right", fill="y") # Gắn scrollbar vào bên phải bảng
        
        # Sự kiện click đúp để vào phòng nhanh
        self.tree.bind("<Double-1>", self.on_item_double_click)

        # --- Nút hành động chính ---
        self.btn_join = tk.Button(self, text="THAM GIA PHÒNG ĐÃ CHỌN", 
                                  font=("Arial", 11, "bold"), bg="#4CAF50", fg="white",
                                  padx=20, pady=10, command=self.join_selected_room)
        self.btn_join.pack(pady=15)

    # --- LOGIC XỬ LÝ ---

    def refresh_rooms(self):
        """Gửi yêu cầu lấy danh sách phòng mới (C2S_LIST_ROOMS = 201)"""
        self.controller.backend.send_command({"type": 201})

    def search_item(self):
        """Gửi yêu cầu tìm kiếm vật phẩm (C2S_SEARCH_ITEM = 205)"""
        keyword = self.ent_search.get().strip()
        if not keyword:
            messagebox.showwarning("Chú ý", "Vui lòng nhập từ khóa tìm kiếm!")
            return
        self.controller.backend.send_command({
            "type": 205,
            "keyword": keyword
        })

    def update_room_list(self, rooms_data):
        """Cập nhật dữ liệu vào bảng khi Server gửi về (S2C_ROOM_LIST = 810)"""
        # Xóa dữ liệu cũ
        for item in self.tree.get_children():
            self.tree.delete(item)
            
        # Thêm dữ liệu mới
        for room in rooms_data:
            price_val = room.get("current_price", 0)
            queue_len = len(room.get("queue", []))
            
            self.tree.insert("", "end", values=(
                room.get("id"),
                room.get("owner", "N/A"),
                f"{price_val:,} VND",
                f"{queue_len} món"
            ))

    def get_selected_room_id(self):
        """Lấy ID của phòng đang được chọn trong bảng"""
        selected = self.tree.selection()
        if not selected:
            messagebox.showwarning("Chú ý", "Vui lòng chọn một phòng từ danh sách để tham gia!")
            return None
        # item['values'][0] tương ứng với cột "id"
        return self.tree.item(selected[0])['values'][0]

    def join_selected_room(self):
        """Gửi lệnh tham gia phòng (C2S_JOIN_ROOM = 203)"""
        room_id = self.get_selected_room_id()
        if room_id:
            self.controller.backend.send_command({
                "type": 203, 
                "room_id": int(room_id)
            })

    def on_item_double_click(self, event):
        """Hỗ trợ vào phòng bằng cách click đúp chuột"""
        self.join_selected_room()

    def view_history(self):
        """Gửi yêu cầu xem lịch sử (C2S_GET_HISTORY = 501)"""
        self.controller.backend.send_command({"type": 501})

    def open_create_room(self):
        """Mở cửa sổ popup để nhập thông tin tạo phòng mới (C2S_CREATE_ROOM = 202)"""
        create_win = tk.Toplevel(self)
        create_win.title("Thiết lập phòng đấu giá")
        create_win.geometry("350x300")
        create_win.grab_set() # Ngăn tương tác với cửa sổ chính khi đang mở popup

        main_frame = tk.Frame(create_win, padx=20, pady=20)
        main_frame.pack(fill="both", expand=True)

        tk.Label(main_frame, text="Tên vật phẩm đấu giá:", font=("Arial", 10)).pack(anchor="w")
        ent_title = tk.Entry(main_frame, font=("Arial", 11))
        ent_title.pack(fill="x", pady=5)
        ent_title.focus_set()

        tk.Label(main_frame, text="Giá khởi điểm (VND):", font=("Arial", 10)).pack(anchor="w", pady=(10, 0))
        ent_price = tk.Entry(main_frame, font=("Arial", 11))
        ent_price.pack(fill="x", pady=5)

        tk.Label(main_frame, text="Giá mua ngay (VND):", font=("Arial", 10)).pack(anchor="w", pady=(10, 0))
        ent_buy = tk.Entry(main_frame, font=("Arial", 11))
        ent_buy.pack(fill="x", pady=5)

        def submit():
            title = ent_title.get().strip()
            price_str = ent_price.get().strip()
            buy_str = ent_buy.get().strip()

            if not title or not price_str or not buy_str:
                messagebox.showerror("Lỗi", "Vui lòng nhập đầy đủ thông tin!")
                return

            try:
                price = int(price_str)
                buy = int(buy_str)
                
                # Gửi gói tin tạo phòng sang C Client Daemon
                self.controller.backend.send_command({
                    "type": 202,
                    "title": title,
                    "start_price": price,
                    "buy_now": buy
                })
                create_win.destroy()
                # Tự động làm mới danh sách sau khi tạo
                self.after(500, self.refresh_rooms)
                
            except ValueError:
                messagebox.showerror("Lỗi", "Giá tiền phải là con số!")

        tk.Button(main_frame, text="TẠO PHÒNG NGAY", bg="#2196F3", fg="white", 
                  font=("Arial", 10, "bold"), pady=8, command=submit).pack(fill="x", pady=20)