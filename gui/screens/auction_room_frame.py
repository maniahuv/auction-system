import tkinter as tk
from tkinter import messagebox

class AuctionRoomFrame(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.room_id = None
        self.is_started = False  # Trạng thái phiên đấu giá đã bắt đầu hay chưa

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
        self.lbl_timer = tk.Label(self.timer_container, text="CHỜ BẮT ĐẦU", font=("Courier", 20, "bold"), bg="black", fg="yellow", width=15)
        self.lbl_timer.pack()

        # --- Danh sách hàng chờ (Queue Management) ---
        self.queue_frame = tk.LabelFrame(self, text=" Danh sách hàng chờ (Tiếp theo) ", padx=10, pady=10, font=("Arial", 10, "bold"))
        self.queue_frame.pack(fill="both", expand=True, padx=30, pady=10)

        self.list_queue = tk.Listbox(self.queue_frame, height=5, font=("Arial", 11), selectmode=tk.SINGLE)
        self.list_queue.pack(side="left", fill="both", expand=True)
        
        q_scroll = tk.Scrollbar(self.queue_frame, orient="vertical", command=self.list_queue.yview)
        q_scroll.pack(side="left", fill="y")
        self.list_queue.config(yscrollcommand=q_scroll.set)

        # Nút chức năng dành cho Auctioneer (Role 2)
        if self.controller.user_role == 2:
            self.mgr_btn_frame = tk.Frame(self.queue_frame)
            self.mgr_btn_frame.pack(side="right", padx=10)
            
            # Gán thuộc tính để có thể ẩn/khóa khi đấu giá bắt đầu
            self.btn_start = tk.Button(self.mgr_btn_frame, text="▶ BẮT ĐẦU", bg="#4CAF50", fg="white", 
                                      font=("Arial", 9, "bold"), width=12, command=self.send_start_auction)
            self.btn_start.pack(pady=5)

            self.btn_add = tk.Button(self.mgr_btn_frame, text="➕ Thêm món", bg="#E3F2FD", font=("Arial", 9, "bold"), width=12,
                                    command=self.open_add_item)
            self.btn_add.pack(pady=5)

            self.btn_delete = tk.Button(self.mgr_btn_frame, text="❌ Xóa chọn", bg="#FFEBEE", fg="red", font=("Arial", 9, "bold"), width=12,
                                       command=self.delete_selected_item)
            self.btn_delete.pack(pady=5)

        # --- Khu vực điều khiển Đấu giá (Dành cho Bidder) ---
        self.control_frame = tk.Frame(self)
        self.control_frame.pack(pady=10)

        tk.Label(self.control_frame, text="Mức giá của bạn:", font=("Arial", 11)).grid(row=0, column=0, padx=5)
        self.ent_bid = tk.Entry(self.control_frame, width=15, font=("Arial", 12))
        self.ent_bid.grid(row=0, column=1, padx=5)

        self.btn_bid = tk.Button(self.control_frame, text="ĐẶT THẦU (BID)", bg="#4CAF50", fg="white", 
                                 font=("Arial", 10, "bold"), padx=10, command=self.send_bid)
        self.btn_bid.grid(row=0, column=2, padx=5)

        self.btn_buy_now = tk.Button(self, text="MUA NGAY (BUY NOW)", bg="#FF5722", fg="white", 
                                     font=("Arial", 11, "bold"), width=30, pady=5, command=self.send_buy_now)
        self.btn_buy_now.pack(pady=10)

        self.footer_frame = tk.Frame(self)
        self.footer_frame.pack(side="bottom", fill="x", pady=15)
        tk.Button(self.footer_frame, text="⬅ Rời khỏi phòng", font=("Arial", 10), command=self.leave_room).pack()

    def set_room_info(self, room_id):
        self.room_id = room_id
        self.lbl_room.config(text=f"PHÒNG ĐẤU GIÁ #{room_id}")
        self.is_started = False
        self.reset_ui_state()

    def reset_ui_state(self):
        """Khôi phục trạng thái UI dựa trên vai trò và biến is_started"""
        is_bidder = (self.controller.user_role == 1)
        # Chỉ cho phép Bidder tương tác khi phiên đã bắt đầu
        state = "normal" if (is_bidder and self.is_started) else "disabled"
        
        self.btn_bid.config(state=state, bg="#4CAF50" if state == "normal" else "#ccc")
        self.btn_buy_now.config(state=state, bg="#FF5722" if state == "normal" else "#ccc")
        self.ent_bid.config(state=state)

        if not self.is_started:
            self.lbl_timer.config(text="CHỜ BẮT ĐẦU", bg="black", fg="yellow")
            self.timer_container.config(bg="black")

    def update_auction_state(self, data):
        """Cập nhật dữ liệu từ Server gửi về (901, 903, 904, 906, 902, 907)"""
        msg_type = data.get("type")
        
        # Cập nhật trạng thái bắt đầu từ Server
        if "is_started" in data:
            self.is_started = bool(data.get("is_started"))

        # 901: Join | 903: Bắt đầu | 904: Có thầu mới
        if msg_type in [901, 903, 904]:
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('current_price', 0):,} VND")
            self.lbl_bidder.config(text=f"Người giữ giá: {data.get('bidder', 'Chưa có')}")
            
            if msg_type == 903:
                # Ẩn nút Bắt đầu và khóa chỉnh sửa hàng chờ cho Auctioneer
                if self.controller.user_role == 2:
                    self.btn_start.pack_forget()
                    self.btn_add.config(state="disabled")
                    self.btn_delete.config(state="disabled")
                messagebox.showinfo("Thông báo", data.get("message", "Phiên đấu giá đã bắt đầu!"))

            if msg_type == 901:
                if "item_title" in data:
                    self.lbl_item_name.config(text=f"Vật phẩm: {data.get('item_title')}")
                if "queue" in data:
                    self.update_queue_list(data.get("queue"))
                # Nếu join phòng đã bắt đầu, ẩn nút Start cho Auctioneer
                if self.is_started and self.controller.user_role == 2:
                    self.btn_start.pack_forget()
                    self.btn_add.config(state="disabled")
                    self.btn_delete.config(state="disabled")

            if "time_left" in data:
                self.update_timer(data.get("time_left"))
            self.reset_ui_state()
            
        # 906: Phiên kết thúc
        elif msg_type == 906:
            self.lbl_timer.config(text="PHIÊN KẾT THÚC", fg="white", bg="#B71C1C")
            self.timer_container.config(bg="#B71C1C")
            self.btn_bid.config(state="disabled", bg="#ccc")
            self.btn_buy_now.config(state="disabled", bg="#ccc")
            self.ent_bid.config(state="disabled")
            
        # 902: Chuyển món tiếp theo
        elif msg_type == 902:
            self.reset_ui_state()
            self.lbl_item_name.config(text=f"Vật phẩm: {data.get('title', '---')}")
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('start_price', 0):,} VND")
            self.lbl_bidder.config(text="Người giữ giá: Chưa có")
            self.ent_bid.delete(0, tk.END)

        # 907: Cập nhật hàng chờ độc lập
        elif msg_type == 907:
            if "queue" in data:
                self.update_queue_list(data.get("queue"))

    def update_timer(self, time_left):
        """Cập nhật đồng hồ và hiệu ứng nháy đỏ khi còn dưới 30s"""
        if not self.is_started: return
        minutes, seconds = time_left // 60, time_left % 60
        self.lbl_timer.config(text=f"THỜI GIAN: {minutes:02d}:{seconds:02d}")
        
        if 0 < time_left <= 30:
            self.lbl_timer.config(fg="#FF5252")
            self.timer_container.config(bg="red" if time_left % 2 == 0 else "black")
        elif time_left <= 0:
            self.lbl_timer.config(text="HẾT GIỜ", fg="white", bg="#B71C1C")
            self.timer_container.config(bg="#B71C1C")
        else:
            self.lbl_timer.config(fg="yellow", bg="black")
            self.timer_container.config(bg="black")

    def update_queue_list(self, queue_data):
        self.list_queue.delete(0, tk.END)
        if not queue_data:
            self.list_queue.insert(tk.END, "(Hàng chờ trống)")
            return
        for idx, item in enumerate(queue_data):
            status = "[ĐANG ĐẤU]" if idx == 0 else f"[#Kế tiếp {idx}]"
            self.list_queue.insert(tk.END, f"{status} {item['title']} - {item['start_price']:,} VND")

    # --- LOGIC QUẢN LÝ (AUCTIONEER - ROLE 2) ---

    def send_start_auction(self):
        """Gửi lệnh bắt đầu phiên đấu giá (C2S_START_AUCTION = 206)"""
        if messagebox.askyesno("Xác nhận", "Bắt đầu đấu giá ngay bây giờ?\nSau khi bắt đầu sẽ KHÔNG thể thêm/xóa vật phẩm."):
            self.controller.backend.send_command({"type": 206})

    def open_add_item(self):
        if self.is_started:
            messagebox.showerror("Lỗi", "Không thể thêm vật phẩm khi phiên đã bắt đầu!")
            return
        # Popup logic...
        add_win = tk.Toplevel(self)
        add_win.title("Thêm vật phẩm vào hàng chờ")
        add_win.geometry("380x350")
        add_win.grab_set()
        main_f = tk.Frame(add_win, padx=25, pady=20)
        main_f.pack(fill="both", expand=True)
        
        tk.Label(main_f, text="Tên vật phẩm:").pack(anchor="w")
        ent_title = tk.Entry(main_f, font=("Arial", 11)); ent_title.pack(fill="x", pady=5)
        tk.Label(main_f, text="Giá khởi điểm (VND):").pack(anchor="w")
        ent_price = tk.Entry(main_f, font=("Arial", 11)); ent_price.pack(fill="x", pady=5)
        tk.Label(main_f, text="Giá mua ngay (VND):").pack(anchor="w")
        ent_buy = tk.Entry(main_f, font=("Arial", 11)); ent_buy.pack(fill="x", pady=5)

        def submit():
            try:
                t, p, b = ent_title.get().strip(), int(ent_price.get()), int(ent_buy.get())
                if not t or p <= 0: raise ValueError
                self.controller.backend.send_command({"type": 301, "room_id": self.room_id, "title": t, "start_price": p, "buy_now": b})
                add_win.destroy()
            except ValueError: messagebox.showerror("Lỗi", "Vui lòng nhập số hợp lệ!")
        tk.Button(main_f, text="XÁC NHẬN", bg="#4CAF50", fg="white", font=("Arial", 10, "bold"), command=submit).pack(fill="x", pady=20)

    def delete_selected_item(self):
        if self.is_started:
            messagebox.showerror("Lỗi", "Không thể xóa vật phẩm khi phiên đã bắt đầu!")
            return
        idx = self.list_queue.curselection()
        if not idx: return
        if idx[0] == 0:
            messagebox.showwarning("Từ chối", "Không thể xóa vật phẩm đang đấu!")
            return
        if messagebox.askyesno("Xác nhận", "Xóa vật phẩm này?"):
            self.controller.backend.send_command({"type": 302, "room_id": self.room_id, "item_index": idx[0] + 1})

    # --- ĐIỀU KHIỂN ĐẤU GIÁ (BIDDER - ROLE 1) ---

    def send_bid(self):
        if not self.is_started: return
        val = self.ent_bid.get().strip()
        if not val: return
        try:
            self.controller.backend.send_command({"type": 401, "price": int(val)})
            self.ent_bid.delete(0, tk.END)
        except ValueError: messagebox.showerror("Lỗi", "Nhập số nguyên hợp lệ!")

    def send_buy_now(self):
        if not self.is_started: return
        if messagebox.askyesno("Xác nhận", "Mua ngay vật phẩm này?"):
            self.controller.backend.send_command({"type": 402})

    def leave_room(self):
        if messagebox.askyesno("Xác nhận", "Rời khỏi phòng đấu giá?"):
            self.controller.backend.send_command({"type": 204})