import tkinter as tk
from tkinter import messagebox

class AuctionRoomFrame(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.room_id = None

        # --- Bố cục giao diện ---
        self.lbl_room = tk.Label(self, text="PHÒNG ĐẤU GIÁ #", font=("Arial", 18, "bold"), fg="#2196F3")
        self.lbl_room.pack(pady=15)

        # Khung thông tin vật phẩm đang đấu giá
        self.info_frame = tk.LabelFrame(self, text=" Phiên đấu giá hiện tại ", padx=20, pady=15, font=("Arial", 10, "bold"))
        self.info_frame.pack(fill="x", padx=30)

        self.lbl_item_name = tk.Label(self.info_frame, text="Vật phẩm: ---", font=("Arial", 13, "bold"))
        self.lbl_item_name.pack(anchor="w", pady=2)

        self.lbl_current_price = tk.Label(self.info_frame, text="Giá hiện tại: 0 VND", font=("Arial", 16, "bold"), fg="#D32F2F")
        self.lbl_current_price.pack(anchor="w", pady=5)

        self.lbl_bidder = tk.Label(self.info_frame, text="Người đang giữ giá: ---", font=("Arial", 11, "italic"), fg="#555")
        self.lbl_bidder.pack(anchor="w", pady=2)

        # Bộ đếm thời gian (Timer)
        self.timer_container = tk.Frame(self, bg="black", padx=2, pady=2)
        self.timer_container.pack(pady=10)
        self.lbl_timer = tk.Label(self.timer_container, text="THỜI GIAN: --:--", font=("Courier", 20, "bold"), bg="black", fg="yellow", width=15)
        self.lbl_timer.pack()

        # --- Danh sách hàng chờ (Queue Management) ---
        self.queue_frame = tk.LabelFrame(self, text=" Danh sách hàng chờ (Tiếp theo) ", padx=10, pady=10, font=("Arial", 10, "bold"))
        self.queue_frame.pack(fill="both", expand=True, padx=30, pady=10)

        self.list_queue = tk.Listbox(self.queue_frame, height=5, font=("Arial", 11), selectmode=tk.SINGLE)
        self.list_queue.pack(side="left", fill="both", expand=True)
        
        # Scrollbar cho hàng chờ
        q_scroll = tk.Scrollbar(self.queue_frame, orient="vertical", command=self.list_queue.yview)
        q_scroll.pack(side="left", fill="y")
        self.list_queue.config(yscrollcommand=q_scroll.set)

        # Nút chức năng quản trị dành cho Auctioneer (Role 2)
        if self.controller.user_role == 2:
            self.mgr_btn_frame = tk.Frame(self.queue_frame)
            self.mgr_btn_frame.pack(side="right", padx=10)
            
            tk.Button(self.mgr_btn_frame, text="➕ Thêm món", bg="#E3F2FD", font=("Arial", 9, "bold"), width=12,
                      command=self.open_add_item).pack(pady=5)
            tk.Button(self.mgr_btn_frame, text="❌ Xóa chọn", bg="#FFEBEE", fg="red", font=("Arial", 9, "bold"), width=12,
                      command=self.delete_selected_item).pack(pady=5)

        # --- Khu vực điều khiển Đấu giá (Dành cho Bidder) ---
        self.control_frame = tk.Frame(self)
        self.control_frame.pack(pady=10)

        # Trạng thái nút: Chỉ Bidder (Role 1) mới được quyền đấu giá
        self.interaction_state = "normal" if self.controller.user_role == 1 else "disabled"

        tk.Label(self.control_frame, text="Mức giá của bạn:", font=("Arial", 11)).grid(row=0, column=0, padx=5)
        self.ent_bid = tk.Entry(self.control_frame, width=15, font=("Arial", 12), state=self.interaction_state)
        self.ent_bid.grid(row=0, column=1, padx=5)

        self.btn_bid = tk.Button(self.control_frame, text="ĐẶT THẦU (BID)", bg="#4CAF50", fg="white", 
                                 font=("Arial", 10, "bold"), padx=10, state=self.interaction_state, command=self.send_bid)
        self.btn_bid.grid(row=0, column=2, padx=5)

        self.btn_buy_now = tk.Button(self, text="MUA NGAY (BUY NOW)", bg="#FF5722", fg="white", 
                                     font=("Arial", 11, "bold"), width=30, pady=5, state=self.interaction_state, command=self.send_buy_now)
        self.btn_buy_now.pack(pady=10)

        # Nút rời phòng
        self.footer_frame = tk.Frame(self)
        self.footer_frame.pack(side="bottom", fill="x", pady=15)
        tk.Button(self.footer_frame, text="⬅ Rời khỏi phòng", font=("Arial", 10), command=self.leave_room).pack()

    def set_room_info(self, room_id):
        self.room_id = room_id
        self.lbl_room.config(text=f"PHÒNG ĐẤU GIÁ #{room_id}")
        self.reset_ui_state()

    def reset_ui_state(self):
        """Khôi phục trạng thái ban đầu của các nút bấm và màu sắc (Enable controls)"""
        if self.controller.user_role == 1:
            self.btn_bid.config(state="normal", bg="#4CAF50")
            self.btn_buy_now.config(state="normal", bg="#FF5722")
            self.ent_bid.config(state="normal")
        self.lbl_timer.config(bg="black", fg="yellow")
        self.timer_container.config(bg="black")

    def update_auction_state(self, data):
        """Cập nhật dữ liệu từ Server gửi về (901, 904, 906, 902)"""
        msg_type = data.get("type")
        
        # 901: Join thành công hoặc 904: Có giá thầu mới
        if msg_type in [901, 904]:
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('current_price', 0):,} VND")
            self.lbl_bidder.config(text=f"Người giữ giá: {data.get('bidder', 'Chưa có')}")
            
            # Khởi tạo thông tin món đồ khi vừa join
            if msg_type == 901:
                if "item_title" in data:
                    self.lbl_item_name.config(text=f"Vật phẩm: {data.get('item_title')}")
                if "time_left" in data:
                    self.update_timer(data.get("time_left"))
                if "queue" in data:
                    self.update_queue_list(data.get("queue"))
            
        # 906: Phiên đấu giá của vật phẩm hiện tại kết thúc
        elif msg_type == 906:
            self.lbl_timer.config(text="PHIÊN KẾT THÚC", fg="white", bg="#B71C1C")
            self.timer_container.config(bg="#B71C1C")
            # Khóa các nút để chờ Server xử lý vật phẩm tiếp theo
            self.btn_bid.config(state="disabled", bg="#ccc")
            self.btn_buy_now.config(state="disabled", bg="#ccc")
            self.ent_bid.config(state="disabled")
            
        # 902: Server chuyển sang món tiếp theo trong hàng chờ (ĐÂY LÀ PHẦN QUAN TRỌNG)
        elif msg_type == 902:
            self.reset_ui_state() # Kích hoạt lại nút cho Bidder để đấu giá vật phẩm mới
            self.lbl_item_name.config(text=f"Vật phẩm: {data.get('title', '---')}")
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('start_price', 0):,} VND")
            self.lbl_bidder.config(text="Người giữ giá: Chưa có")
            self.ent_bid.delete(0, tk.END)

    def update_timer(self, time_left):
        """Cập nhật đồng hồ và xử lý hiệu ứng nháy đỏ khi còn dưới 30s"""
        minutes = time_left // 60
        seconds = time_left % 60
        self.lbl_timer.config(text=f"THỜI GIAN: {minutes:02d}:{seconds:02d}")
        
        if time_left <= 30 and time_left > 0:
            self.lbl_timer.config(fg="#FF5252")
            # Hiệu ứng nháy
            self.timer_container.config(bg="red" if time_left % 2 == 0 else "black")
        elif time_left <= 0:
            self.lbl_timer.config(text="HẾT GIỜ", fg="white")
            self.timer_container.config(bg="#B71C1C")
        else:
            self.lbl_timer.config(fg="yellow")
            self.timer_container.config(bg="black")

    def update_queue_list(self, queue_data):
        """Đồng bộ hiển thị danh sách hàng chờ (Mã 907 hoặc từ 901)"""
        self.list_queue.delete(0, tk.END)
        if not queue_data:
            self.list_queue.insert(tk.END, "(Hàng chờ trống)")
            return
            
        for idx, item in enumerate(queue_data):
            # Món đang đấu là món ở index 0
            status = "[ĐANG ĐẤU]" if idx == 0 else f"[#Kế tiếp {idx}]"
            self.list_queue.insert(tk.END, f"{status} {item['title']} - {item['start_price']:,} VND")

    # --- LOGIC QUẢN LÝ DÀNH CHO CHỦ PHÒNG (AUCTIONEER) ---

    def open_add_item(self):
        """Popup thêm sản phẩm mới vào hàng chờ (C2S_ADD_ITEM = 301)"""
        add_win = tk.Toplevel(self)
        add_win.title("Thêm vật phẩm vào hàng chờ")
        add_win.geometry("380x350")
        add_win.resizable(False, False)
        add_win.grab_set()

        main_f = tk.Frame(add_win, padx=25, pady=20)
        main_f.pack(fill="both", expand=True)

        tk.Label(main_f, text="THÔNG TIN VẬT PHẨM MỚI", font=("Arial", 11, "bold")).pack(pady=(0, 10))

        tk.Label(main_f, text="Tên vật phẩm:").pack(anchor="w")
        ent_title = tk.Entry(main_f, font=("Arial", 11)); ent_title.pack(fill="x", pady=5)
        ent_title.focus_set()

        tk.Label(main_f, text="Giá khởi điểm (VND):").pack(anchor="w", pady=(10, 0))
        ent_price = tk.Entry(main_f, font=("Arial", 11)); ent_price.pack(fill="x", pady=5)

        tk.Label(main_f, text="Giá mua ngay (VND):").pack(anchor="w", pady=(10, 0))
        ent_buy = tk.Entry(main_f, font=("Arial", 11)); ent_buy.pack(fill="x", pady=5)

        def submit():
            try:
                title = ent_title.get().strip()
                price = int(ent_price.get())
                buy = int(ent_buy.get())
                if not title or price <= 0: raise ValueError
                
                self.controller.backend.send_command({
                    "type": 301,
                    "room_id": self.room_id,
                    "title": title,
                    "start_price": price,
                    "buy_now": buy
                })
                add_win.destroy()
            except ValueError:
                messagebox.showerror("Lỗi", "Vui lòng nhập số hợp lệ cho giá tiền!")

        tk.Button(main_f, text="XÁC NHẬN THÊM", bg="#4CAF50", fg="white", 
                  font=("Arial", 10, "bold"), pady=10, command=submit).pack(fill="x", pady=20)

    def delete_selected_item(self):
        """Xóa sản phẩm khỏi hàng chờ (C2S_DELETE_ITEM = 302)"""
        idx = self.list_queue.curselection()
        if not idx:
            messagebox.showwarning("Chú ý", "Vui lòng chọn một vật phẩm trong hàng chờ để xóa!")
            return
        
        # Ngăn xóa món đang đấu (vị trí 0)
        if idx[0] == 0:
            messagebox.showwarning("Từ chối", "Không thể xóa vật phẩm đang trong phiên đấu giá trực tiếp!")
            return

        if messagebox.askyesno("Xác nhận", "Bạn có chắc chắn muốn xóa vật phẩm này khỏi hàng chờ?"):
            self.controller.backend.send_command({
                "type": 302,
                "room_id": self.room_id,
                "item_index": idx[0] + 1  # Server tính chỉ số từ 1 trong hàng chờ
            })

    # --- CÁC HÀM GỬI LỆNH ĐẤU GIÁ (DÀNH CHO BIDDER) ---

    def send_bid(self):
        """Gửi giá thầu mới (C2S_BID = 401)"""
        val = self.ent_bid.get().strip()
        if not val: return
        try:
            amount = int(val)
            self.controller.backend.send_command({"type": 401, "price": amount})
            self.ent_bid.delete(0, tk.END)
        except ValueError:
            messagebox.showerror("Lỗi", "Vui lòng nhập số nguyên cho mức giá!")

    def send_buy_now(self):
        """Mua ngay lập tức (C2S_BUY_NOW = 402)"""
        if messagebox.askyesno("Xác nhận", "Bạn chắc chắn muốn 'Mua ngay' vật phẩm này với giá định sẵn?"):
            self.controller.backend.send_command({"type": 402})

    def leave_room(self):
        """Rời phòng đấu giá (C2S_LEAVE_ROOM = 204)"""
        if messagebox.askyesno("Xác nhận", "Bạn muốn rời khỏi phòng đấu giá này?"):
            self.controller.backend.send_command({"type": 204})