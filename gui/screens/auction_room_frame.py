import tkinter as tk
from tkinter import messagebox

class AuctionRoomFrame(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.room_id = None
        self.is_started = False
        self.time_left = 0
        self.timer_job = None  # Quản lý vòng lặp đếm ngược
        self.current_buy_now = 0 # Lưu trữ giá mua ngay hiện tại

        # --- Bố cục giao diện ---
        header_frame = tk.Frame(self)
        header_frame.pack(fill="x", pady=10)
        
        self.lbl_room = tk.Label(header_frame, text="PHÒNG ĐẤU GIÁ", font=("Arial", 18, "bold"), fg="#2196F3")
        self.lbl_room.pack(side="left", padx=30)

        # MỚI: Hiển thị số người trong phòng (Chỉ dành cho Auctioneer)
        self.lbl_user_count = tk.Label(header_frame, text="", font=("Arial", 10, "bold"), fg="#757575")
        if self.controller.user_role == 2: # ROLE_AUCTIONEER
            self.lbl_user_count.pack(side="right", padx=30)

        # Khung thông tin vật phẩm đang đấu giá
        self.info_frame = tk.LabelFrame(self, text=" Phiên đấu giá hiện tại ", padx=20, pady=15, font=("Arial", 10, "bold"))
        self.info_frame.pack(fill="x", padx=30)

        self.lbl_item_name = tk.Label(self.info_frame, text="Vật phẩm: ---", font=("Arial", 13, "bold"))
        self.lbl_item_name.pack(anchor="w", pady=2)

        self.lbl_current_price = tk.Label(self.info_frame, text="Giá hiện tại: ---", font=("Arial", 16, "bold"), fg="#D32F2F")
        self.lbl_current_price.pack(anchor="w", pady=5)

        self.lbl_buy_now_price = tk.Label(self.info_frame, text="Giá mua ngay: ---", font=("Arial", 11, "bold"), fg="#FF5722")
        self.lbl_buy_now_price.pack(anchor="w", pady=2)

        self.lbl_bidder = tk.Label(self.info_frame, text="Người đang giữ giá: ---", font=("Arial", 11, "italic"), fg="#555")
        self.lbl_bidder.pack(anchor="w", pady=2)

        # Bộ đếm thời gian (Timer)
        self.timer_container = tk.Frame(self, bg="black", padx=2, pady=2)
        self.timer_container.pack(pady=10)
        self.lbl_timer = tk.Label(self.timer_container, text="CHỜ BẮT ĐẦU", font=("Courier", 20, "bold"), bg="black", fg="yellow", width=15)
        self.lbl_timer.pack()

        # --- Danh sách hàng chờ (Queue Management) ---
        self.queue_frame = tk.LabelFrame(self, text=" Danh sách hàng chờ (Tiếp theo) ", padx=10, pady=5, font=("Arial", 10, "bold"))
        self.queue_frame.pack(fill="both", expand=True, padx=30, pady=5)

        self.list_queue = tk.Listbox(self.queue_frame, height=4, font=("Arial", 11), selectmode=tk.SINGLE)
        self.list_queue.pack(side="left", fill="both", expand=True)
        
        q_scroll = tk.Scrollbar(self.queue_frame, orient="vertical", command=self.list_queue.yview)
        q_scroll.pack(side="left", fill="y")
        self.list_queue.config(yscrollcommand=q_scroll.set)

        # Nút chức năng dành cho Auctioneer (Role 2)
        if self.controller.user_role == 2:
            self.mgr_btn_frame = tk.Frame(self.queue_frame)
            self.mgr_btn_frame.pack(side="right", padx=10)
            
            self.btn_start = tk.Button(self.mgr_btn_frame, text="▶ BẮT ĐẦU", bg="#4CAF50", fg="white", 
                                      font=("Arial", 9, "bold"), width=12, command=self.send_start_auction)
            self.btn_start.pack(pady=2)

            self.btn_add = tk.Button(self.mgr_btn_frame, text="➕ Thêm món", bg="#E3F2FD", font=("Arial", 9, "bold"), width=12,
                                    command=self.open_add_item)
            self.btn_add.pack(pady=2)

            self.btn_update = tk.Button(self.mgr_btn_frame, text="✏️ Sửa món", bg="#FFF9C4", font=("Arial", 9, "bold"), width=12,
                                       command=self.open_update_item)
            self.btn_update.pack(pady=2)

            self.btn_delete = tk.Button(self.mgr_btn_frame, text="❌ Xóa chọn", bg="#FFEBEE", fg="red", font=("Arial", 9, "bold"), width=12,
                                       command=self.delete_selected_item)
            self.btn_delete.pack(pady=2)

        # --- Khung Trò chuyện (Chat) ---
        self.chat_frame = tk.LabelFrame(self, text=" Trò chuyện trực tuyến ", padx=10, pady=5, font=("Arial", 10, "bold"))
        self.chat_frame.pack(fill="both", expand=True, padx=30, pady=5)

        self.chat_display = tk.Text(self.chat_frame, height=5, state="disabled", font=("Arial", 10))
        self.chat_display.pack(side="top", fill="both", expand=True)
        
        c_scroll = tk.Scrollbar(self.chat_display, orient="vertical", command=self.chat_display.yview)
        c_scroll.pack(side="right", fill="y")
        self.chat_display.config(yscrollcommand=c_scroll.set)

        self.chat_input_frame = tk.Frame(self.chat_frame)
        self.chat_input_frame.pack(side="bottom", fill="x", pady=(5, 0))

        self.ent_chat = tk.Entry(self.chat_input_frame, font=("Arial", 11))
        self.ent_chat.pack(side="left", fill="x", expand=True, padx=(0, 5))
        self.ent_chat.bind("<Return>", lambda e: self.send_chat())

        self.btn_send_chat = tk.Button(self.chat_input_frame, text="Gửi", bg="#2196F3", fg="white", 
                                       font=("Arial", 9, "bold"), width=8, command=self.send_chat)
        self.btn_send_chat.pack(side="right")

        # --- Khu vực điều khiển Đấu giá (Bidder) ---
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
        self.btn_buy_now.pack(pady=5)

        tk.Button(self, text="⬅ Rời khỏi phòng", font=("Arial", 10), command=self.leave_room).pack(side="bottom", pady=10)

    # --- LOGIC ĐIỀU PHỐI ---

    def tick(self):
        if not self.is_started: return
        if self.time_left > 0:
            self.time_left -= 1
            self.refresh_timer_ui()
            self.timer_job = self.after(1000, self.tick)
        else:
            self.lbl_timer.config(text="HẾT GIỜ", fg="white", bg="#B71C1C")
            self.timer_container.config(bg="#B71C1C")

    def refresh_timer_ui(self):
        minutes, seconds = self.time_left // 60, self.time_left % 60
        self.lbl_timer.config(text=f"{minutes:02d}:{seconds:02d}")
        
        if 0 < self.time_left <= 30:
            self.lbl_timer.config(fg="#FF5252")
            self.timer_container.config(bg="red" if self.time_left % 2 == 0 else "black")
        elif self.time_left <= 0:
            self.lbl_timer.config(fg="white")
        else:
            self.lbl_timer.config(fg="yellow", bg="black")
            self.timer_container.config(bg="black")

    def sync_timer(self, server_time):
        if self.timer_job: self.after_cancel(self.timer_job)
        self.time_left = server_time
        self.refresh_timer_ui()
        if self.is_started and self.time_left > 0: self.tick()

    def set_room_info(self, room_id):
        self.room_id = room_id
        room_name = getattr(self.controller, 'current_room_name', f"PHÒNG ĐẤU GIÁ #{room_id}")
        self.lbl_room.config(text=room_name)
        self.is_started = False
        
        # Reset các nhãn khi vào phòng mới
        self.lbl_item_name.config(text="Vật phẩm: ---")
        self.lbl_current_price.config(text="Giá hiện tại: ---")
        self.lbl_buy_now_price.config(text="Giá mua ngay: ---")
        self.lbl_bidder.config(text="Người giữ giá: ---")
        self.current_buy_now = 0
        self.reset_ui_state()

    def reset_ui_state(self):
        is_bidder = (self.controller.user_role == 1)
        # Nút Đặt thầu chỉ bật khi phiên đã bắt đầu
        bid_state = "normal" if (is_bidder and self.is_started) else "disabled"
        self.btn_bid.config(state=bid_state, bg="#4CAF50" if bid_state == "normal" else "#ccc")
        self.ent_bid.config(state=bid_state)

        # Nút Mua ngay chỉ bật nếu món hàng hỗ trợ (buy_now > 0)
        bn_state = "normal" if (bid_state == "normal" and self.current_buy_now > 0) else "disabled"
        self.btn_buy_now.config(state=bn_state, bg="#FF5722" if bn_state == "normal" else "#ccc")

        if self.controller.user_role == 2:
            mgr_state = "disabled" if self.is_started else "normal"
            self.btn_add.config(state=mgr_state)
            self.btn_delete.config(state=mgr_state)
            self.btn_update.config(state=mgr_state)
            if self.is_started: self.btn_start.pack_forget()
            else: self.btn_start.pack(pady=2)

        if not self.is_started:
            self.lbl_timer.config(text="CHỜ BẮT ĐẦU", bg="black", fg="yellow")
            self.timer_container.config(bg="black")

    def update_auction_state(self, data):
        msg_type = data.get("type")
        
        # 1. Cập nhật số người xem (Mã 909)
        if msg_type == 909:
            count = data.get("count", 0)
            self.lbl_user_count.config(text=f"👥 Đang xem: {count}")
            return

        # 2. Xử lý tin nhắn chat (Mã 908)
        if msg_type == 908:
            self.display_chat_message(data.get("username"), data.get("message"))
            return

        if "is_started" in data: self.is_started = bool(data.get("is_started"))

        # 3. Xử lý cập nhật thông tin đấu giá (901-904)
        if msg_type in [901, 902, 903, 904]:
            price = data.get('current_price') or data.get('start_price') or 0
            if price > 0:
                self.lbl_current_price.config(text=f"Giá hiện tại: {price:,} VND")
            
            self.lbl_bidder.config(text=f"Người giữ giá: {data.get('bidder', 'Chưa có')}")
            
            if "buy_now" in data:
                self.current_buy_now = data.get("buy_now", 0)
                self.lbl_buy_now_price.config(text=f"Giá mua ngay: {self.current_buy_now:,} VND" if self.current_buy_now > 0 else "Giá mua ngay: Không hỗ trợ")

            if msg_type == 903: # S2C_AUCTION_STARTED
                self.is_started = True
                self.controller.show_toast("🚀 PHIÊN ĐẤU GIÁ CHÍNH THỨC BẮT ĐẦU!", bg="#4CAF50")

            if msg_type == 901: # S2C_JOIN_ROOM_SUCCESS
                if "item_title" in data: self.lbl_item_name.config(text=f"Vật phẩm: {data.get('item_title')}")

            if msg_type == 902: # S2C_NEW_ITEM_PENDING
                self.lbl_item_name.config(text=f"Vật phẩm: {data.get('title', '---')}")
                self.lbl_bidder.config(text="Người giữ giá: Chưa có")
                self.ent_bid.delete(0, tk.END)

            # ĐỒNG BỘ THỜI GIAN: Reset đồng hồ khi nhận time_left (đặc biệt trong mã 904 - New Bid)
            if "time_left" in data: self.sync_timer(data.get("time_left"))
            self.reset_ui_state()
            
        elif msg_type == 906: # S2C_AUCTION_ENDED
            if self.timer_job: self.after_cancel(self.timer_job)
            self.lbl_timer.config(text="PHIÊN KẾT THÚC", fg="white", bg="#B71C1C")
            
            # Nếu hàng chờ chỉ còn 1 (món vừa xong là món cuối), xóa thông tin
            if self.list_queue.size() <= 1: 
                self.lbl_item_name.config(text="Vật phẩm: (Hết vật phẩm)")
                self.lbl_current_price.config(text="Giá hiện tại: ---")
                self.lbl_buy_now_price.config(text="Giá mua ngay: ---")
                self.lbl_bidder.config(text="Người giữ giá: ---")
                self.is_started = False
            self.reset_ui_state()

        # 4. Cập nhật hàng chờ (Kiểm tra dữ liệu "queue" hoặc mã 907)
        if "queue" in data or msg_type == 907:
            self.update_queue_list(data.get("queue"))

    def update_queue_list(self, queue_data):
        self.list_queue.delete(0, tk.END)
        if not queue_data:
            self.list_queue.insert(tk.END, " (Hàng chờ hiện đang trống) ")
            return
            
        for idx, item in enumerate(queue_data):
            status = "🔥 [ĐANG ĐẤU]" if idx == 0 else f"⏳ [#Kế tiếp {idx}]"
            bn = item.get('buy_now', 0)
            bn_str = f" | ⚡BN: {bn:,}" if bn > 0 else ""
            self.list_queue.insert(tk.END, f"{status} {item.get('title')} - {item.get('start_price', 0):,} VND{bn_str}")

    # --- HÀNH ĐỘNG ---

    def send_chat(self):
        msg = self.ent_chat.get().strip()
        if msg:
            self.controller.backend.send_command({"type": 207, "message": msg}) # C2S_CHAT
            self.ent_chat.delete(0, tk.END)

    def display_chat_message(self, username, message):
        self.chat_display.config(state="normal")
        self.chat_display.insert(tk.END, f"{username}: ", "username")
        self.chat_display.insert(tk.END, f"{message}\n")
        self.chat_display.tag_config("username", foreground="#1976D2", font=("Arial", 10, "bold"))
        self.chat_display.see(tk.END); self.chat_display.config(state="disabled")

    def send_start_auction(self):
        if messagebox.askyesno("Xác nhận", "Bắt đầu đấu giá vật phẩm này ngay bây giờ?"):
            self.controller.backend.send_command({"type": 206}) # C2S_START_AUCTION

    def open_add_item(self):
        if self.is_started: return
        add_win = tk.Toplevel(self); add_win.title("Thêm vật phẩm"); add_win.geometry("350x300"); add_win.grab_set()
        tk.Label(add_win, text="Tên vật phẩm:").pack(pady=5); ent_t = tk.Entry(add_win); ent_t.pack()
        tk.Label(add_win, text="Giá khởi điểm (VND):").pack(pady=5); ent_p = tk.Entry(add_win); ent_p.pack()
        tk.Label(add_win, text="Giá mua ngay (0 = Tắt):").pack(pady=5); ent_b = tk.Entry(add_win); ent_b.pack()
        def sub():
            try:
                self.controller.backend.send_command({
                    "type": 301, "room_id": self.room_id, "title": ent_t.get(), 
                    "start_price": int(ent_p.get()), "buy_now": int(ent_b.get())
                })
                add_win.destroy()
            except ValueError: self.controller.show_toast("❌ Giá phải là số nguyên!", bg="#F44336")
        tk.Button(add_win, text="XÁC NHẬN", command=sub, bg="#4CAF50", fg="white").pack(pady=10)

    def open_update_item(self):
        if self.is_started: return
        idx = self.list_queue.curselection()
        if not idx: 
            self.controller.show_toast("⚠️ Hãy chọn một vật phẩm!", bg="#FF9800")
            return
        upd_win = tk.Toplevel(self); upd_win.title("Cập nhật vật phẩm"); upd_win.geometry("350x300"); upd_win.grab_set()
        tk.Label(upd_win, text="Tên mới:").pack(pady=5); ent_t = tk.Entry(upd_win); ent_t.pack()
        tk.Label(upd_win, text="Giá khởi điểm mới:").pack(pady=5); ent_p = tk.Entry(upd_win); ent_p.pack()
        tk.Label(upd_win, text="Giá mua ngay mới:").pack(pady=5); ent_b = tk.Entry(upd_win); ent_b.pack()
        def do_update():
            try:
                self.controller.backend.send_command({
                    "type": 304, "room_id": self.room_id, "item_index": idx[0] + 1, 
                    "title": ent_t.get(), "start_price": int(ent_p.get()), "buy_now": int(ent_b.get())
                })
                upd_win.destroy()
            except ValueError: self.controller.show_toast("❌ Dữ liệu không hợp lệ!", bg="#F44336")
        tk.Button(upd_win, text="CẬP NHẬT", command=do_update, bg="#4CAF50", fg="white").pack(pady=10)

    def delete_selected_item(self):
        if self.is_started: return
        idx = self.list_queue.curselection()
        if idx and idx[0] != 0:
            if messagebox.askyesno("Xác nhận", "Xóa vật phẩm này?"):
                self.controller.backend.send_command({"type": 302, "room_id": self.room_id, "item_index": idx[0] + 1})

    def send_bid(self):
        val = self.ent_bid.get().strip()
        try:
            self.controller.backend.send_command({"type": 401, "price": int(val)}) # C2S_BID
            self.ent_bid.delete(0, tk.END)
        except ValueError: self.controller.show_toast("❌ Giá thầu không hợp lệ!", bg="#F44336")

    def send_buy_now(self):
        if self.is_started and messagebox.askyesno("Mua ngay", "Xác nhận mua ngay vật phẩm này?"):
            self.controller.backend.send_command({"type": 402}) # C2S_BUY_NOW

    def leave_room(self):
        if messagebox.askyesno("Thoát", "Rời khỏi phòng đấu giá?"):
            if self.timer_job: self.after_cancel(self.timer_job)
            self.controller.backend.send_command({"type": 204}) # C2S_LEAVE_ROOM