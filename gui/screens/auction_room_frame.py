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
        self.queue_frame = tk.LabelFrame(self, text=" Danh sách hàng chờ (Tiếp theo) ", padx=10, pady=5, font=("Arial", 10, "bold"))
        self.queue_frame.pack(fill="both", expand=True, padx=30, pady=5)

        self.list_queue = tk.Listbox(self.queue_frame, height=4, font=("Arial", 11), selectmode=tk.SINGLE)
        self.list_queue.pack(side="left", fill="both", expand=True)
        
        q_scroll = tk.Scrollbar(self.queue_frame, orient="vertical", command=self.list_queue.yview)
        q_scroll.pack(side="left", fill="y")
        self.list_queue.config(yscrollcommand=q_scroll.set)

        if self.controller.user_role == 2:
            self.mgr_btn_frame = tk.Frame(self.queue_frame)
            self.mgr_btn_frame.pack(side="right", padx=10)
            
            self.btn_start = tk.Button(self.mgr_btn_frame, text="▶ BẮT ĐẦU", bg="#4CAF50", fg="white", 
                                      font=("Arial", 9, "bold"), width=12, command=self.send_start_auction)
            self.btn_start.pack(pady=5)

            self.btn_add = tk.Button(self.mgr_btn_frame, text="➕ Thêm món", bg="#E3F2FD", font=("Arial", 9, "bold"), width=12,
                                    command=self.open_add_item)
            self.btn_add.pack(pady=5)

            self.btn_delete = tk.Button(self.mgr_btn_frame, text="❌ Xóa chọn", bg="#FFEBEE", fg="red", font=("Arial", 9, "bold"), width=12,
                                       command=self.delete_selected_item)
            self.btn_delete.pack(pady=5)

        # --- Khung Trò chuyện (Chat) ---
        self.chat_frame = tk.LabelFrame(self, text=" Trò chuyện trực tuyến ", padx=10, pady=5, font=("Arial", 10, "bold"))
        self.chat_frame.pack(fill="both", expand=True, padx=30, pady=5)

        self.chat_display = tk.Text(self.chat_frame, height=6, state="disabled", font=("Arial", 10))
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
        self.btn_buy_now.pack(pady=10)

        self.footer_frame = tk.Frame(self)
        self.footer_frame.pack(side="bottom", fill="x", pady=15)
        tk.Button(self.footer_frame, text="⬅ Rời khỏi phòng", font=("Arial", 10), command=self.leave_room).pack()

    # --- HỆ THỐNG THỜI GIAN CHẠY TỪNG GIÂY ---

    def tick(self):
        """Vòng lặp chạy mỗi giây phía Client"""
        if not self.is_started:
            return

        if self.time_left > 0:
            self.time_left -= 1
            self.refresh_timer_ui()
            self.timer_job = self.after(1000, self.tick)
        else:
            self.lbl_timer.config(text="HẾT GIỜ", fg="white", bg="#B71C1C")
            self.timer_container.config(bg="#B71C1C")

    def refresh_timer_ui(self):
        """Cập nhật giao diện nhãn thời gian"""
        minutes, seconds = self.time_left // 60, self.time_left % 60
        self.lbl_timer.config(text=f"THỜI GIAN: {minutes:02d}:{seconds:02d}")
        
        if 0 < self.time_left <= 30:
            self.lbl_timer.config(fg="#FF5252")
            self.timer_container.config(bg="red" if self.time_left % 2 == 0 else "black")
        elif self.time_left <= 0:
            self.lbl_timer.config(text="HẾT GIỜ", fg="white")
        else:
            self.lbl_timer.config(fg="yellow", bg="black")
            self.timer_container.config(bg="black")

    def sync_timer(self, server_time):
        """Đồng bộ thời gian từ Server để tránh bị lệch (Timer Drift)"""
        if self.timer_job:
            self.after_cancel(self.timer_job)
        
        self.time_left = server_time
        self.refresh_timer_ui()
        
        if self.is_started and self.time_left > 0:
            self.tick()

    # --- QUẢN LÝ TRẠNG THÁI ---

    def set_room_info(self, room_id):
        self.room_id = room_id
        self.lbl_room.config(text=f"PHÒNG ĐẤU GIÁ #{room_id}")
        self.is_started = False
        self.reset_ui_state()

    def reset_ui_state(self):
        is_bidder = (self.controller.user_role == 1)
        state = "normal" if (is_bidder and self.is_started) else "disabled"
        
        self.btn_bid.config(state=state, bg="#4CAF50" if state == "normal" else "#ccc")
        self.btn_buy_now.config(state=state, bg="#FF5722" if state == "normal" else "#ccc")
        self.ent_bid.config(state=state)

        if not self.is_started:
            self.lbl_timer.config(text="CHỜ BẮT ĐẦU", bg="black", fg="yellow")
            self.timer_container.config(bg="black")

    def update_auction_state(self, data):
        msg_type = data.get("type")
        
        if "is_started" in data:
            self.is_started = bool(data.get("is_started"))

        if msg_type in [901, 903, 904]:
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('current_price', 0):,} VND")
            self.lbl_bidder.config(text=f"Người giữ giá: {data.get('bidder', 'Chưa có')}")
            
            if msg_type == 903:
                if self.controller.user_role == 2:
                    self.btn_start.pack_forget()
                    self.btn_add.config(state="disabled")
                    self.btn_delete.config(state="disabled")
                self.controller.show_toast("🚀 PHIÊN ĐẤU GIÁ CHÍNH THỨC BẮT ĐẦU!", bg="#4CAF50")

            if msg_type == 901:
                if "item_title" in data:
                    self.lbl_item_name.config(text=f"Vật phẩm: {data.get('item_title')}")
                if "queue" in data:
                    self.update_queue_list(data.get("queue"))
                if self.is_started and self.controller.user_role == 2:
                    self.btn_start.pack_forget()
                    self.btn_add.config(state="disabled")
                    self.btn_delete.config(state="disabled")

            if "time_left" in data:
                self.sync_timer(data.get("time_left"))
            self.reset_ui_state()
            
        elif msg_type == 906:
            self.is_started = False
            if self.timer_job: self.after_cancel(self.timer_job)
            self.lbl_timer.config(text="PHIÊN KẾT THÚC", fg="white", bg="#B71C1C")
            self.timer_container.config(bg="#B71C1C")
            self.btn_bid.config(state="disabled", bg="#ccc")
            self.btn_buy_now.config(state="disabled", bg="#ccc")
            self.ent_bid.config(state="disabled")
            
        elif msg_type == 902:
            self.lbl_item_name.config(text=f"Vật phẩm: {data.get('title', '---')}")
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('start_price', 0):,} VND")
            self.lbl_bidder.config(text="Người giữ giá: Chưa có")
            self.ent_bid.delete(0, tk.END)
            if "time_left" in data:
                self.sync_timer(data.get("time_left"))
            self.reset_ui_state()

        elif msg_type == 907:
            if "queue" in data: self.update_queue_list(data.get("queue"))

        elif msg_type == 905: # Cảnh báo thời gian từ Server
            if "time_left" in data:
                self.sync_timer(data.get("time_left"))

    def update_queue_list(self, queue_data):
        self.list_queue.delete(0, tk.END)
        if not queue_data:
            self.list_queue.insert(tk.END, "(Hàng chờ trống)")
            return
        for idx, item in enumerate(queue_data):
            status = "[ĐANG ĐẤU]" if idx == 0 else f"[#Kế tiếp {idx}]"
            self.list_queue.insert(tk.END, f"{status} {item['title']} - {item['start_price']:,} VND")

    # --- TRÒ CHUYỆN ---

    def send_chat(self):
        msg = self.ent_chat.get().strip()
        if msg:
            self.controller.backend.send_command({"type": 207, "message": msg})
            self.ent_chat.delete(0, tk.END)

    def display_chat_message(self, username, message):
        self.chat_display.config(state="normal")
        self.chat_display.insert(tk.END, f"{username}: ", "username")
        self.chat_display.insert(tk.END, f"{message}\n")
        self.chat_display.tag_config("username", foreground="#1976D2", font=("Arial", 10, "bold"))
        self.chat_display.see(tk.END)
        self.chat_display.config(state="disabled")

    # --- ĐIỀU KHIỂN ---

    def send_start_auction(self):
        if messagebox.askyesno("Xác nhận", "Bắt đầu đấu giá ngay bây giờ?"):
            self.controller.backend.send_command({"type": 206})

    def open_add_item(self):
        if self.is_started:
            self.controller.show_toast("⚠️ Đang đấu giá, không thể thêm món!", bg="#F44336")
            return
        # Popup thêm món (như cũ nhưng dùng toast báo lỗi)
        add_win = tk.Toplevel(self)
        add_win.title("Thêm vật phẩm")
        add_win.geometry("350x300")
        add_win.grab_set()
        
        tk.Label(add_win, text="Tên vật phẩm:").pack(pady=5)
        ent_t = tk.Entry(add_win); ent_t.pack()
        tk.Label(add_win, text="Giá khởi điểm:").pack(pady=5)
        ent_p = tk.Entry(add_win); ent_p.pack()
        tk.Label(add_win, text="Giá mua ngay:").pack(pady=5)
        ent_b = tk.Entry(add_win); ent_b.pack()

        def sub():
            try:
                self.controller.backend.send_command({
                    "type": 301, "room_id": self.room_id, 
                    "title": ent_t.get(), "start_price": int(ent_p.get()), "buy_now": int(ent_b.get())
                })
                add_win.destroy()
            except: self.controller.show_toast("❌ Lỗi dữ liệu!", bg="#F44336")
        
        tk.Button(add_win, text="THÊM", command=sub, bg="#4CAF50", fg="white").pack(pady=10)

    def delete_selected_item(self):
        if self.is_started:
            self.controller.show_toast("⚠️ Không thể xóa khi đang đấu!", bg="#F44336")
            return
        idx = self.list_queue.curselection()
        if idx and idx[0] != 0:
            if messagebox.askyesno("Xác nhận", "Xóa vật phẩm này?"):
                self.controller.backend.send_command({"type": 302, "room_id": self.room_id, "item_index": idx[0] + 1})
        else:
            self.controller.show_toast("ℹ️ Không thể xóa món này!", bg="#2196F3")

    def send_bid(self):
        if not self.is_started: return
        val = self.ent_bid.get().strip()
        try:
            self.controller.backend.send_command({"type": 401, "price": int(val)})
            self.ent_bid.delete(0, tk.END)
        except: self.controller.show_toast("❌ Nhập số tiền hợp lệ!", bg="#F44336")

    def send_buy_now(self):
        if self.is_started and messagebox.askyesno("Mua ngay", "Bạn chắc chắn?"):
            self.controller.backend.send_command({"type": 402})

    def leave_room(self):
        if messagebox.askyesno("Thoát", "Rời khỏi phòng?"):
            if self.timer_job: self.after_cancel(self.timer_job)
            self.controller.backend.send_command({"type": 204})