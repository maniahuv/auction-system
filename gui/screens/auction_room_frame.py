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

        # Khung thông tin vật phẩm
        self.info_frame = tk.LabelFrame(self, text=" Phiên đấu giá hiện tại ", padx=20, pady=15, font=("Arial", 10, "bold"))
        self.info_frame.pack(fill="x", padx=30)

        self.lbl_item_name = tk.Label(self.info_frame, text="Vật phẩm: ---", font=("Arial", 13))
        self.lbl_item_name.pack(anchor="w", pady=2)

        self.lbl_current_price = tk.Label(self.info_frame, text="Giá hiện tại: 0 VND", font=("Arial", 16, "bold"), fg="#D32F2F")
        self.lbl_current_price.pack(anchor="w", pady=5)

        self.lbl_bidder = tk.Label(self.info_frame, text="Người đang giữ giá: ---", font=("Arial", 11, "italic"), fg="#555")
        self.lbl_bidder.pack(anchor="w", pady=2)

        # Bộ đếm thời gian (Timer)
        self.timer_container = tk.Frame(self, bg="black", padx=2, pady=2)
        self.timer_container.pack(pady=20)
        self.lbl_timer = tk.Label(self.timer_container, text="THỜI GIAN: --:--", font=("Courier", 20, "bold"), bg="black", fg="yellow", width=15)
        self.lbl_timer.pack()

        # --- Khu vực điều khiển (Chỉ hiển thị đầy đủ cho BIDDER) ---
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

        # Nút chức năng phụ
        self.footer_frame = tk.Frame(self)
        self.footer_frame.pack(side="bottom", fill="x", pady=20)
        
        tk.Button(self.footer_frame, text="⬅ Rời phòng", font=("Arial", 10), command=self.leave_room).pack()

    def set_room_info(self, room_id):
        self.room_id = room_id
        self.lbl_room.config(text=f"PHÒNG ĐẤU GIÁ #{room_id}")
        self.reset_ui_state()

    def reset_ui_state(self):
        """Khôi phục trạng thái ban đầu của các nút bấm"""
        self.btn_bid.config(state="normal", bg="#4CAF50")
        self.btn_buy_now.config(state="normal", bg="#FF5722")
        self.lbl_timer.config(bg="black", fg="yellow")

    def update_auction_state(self, data):
        """Cập nhật dữ liệu từ Server gửi về (904, 906, 902)"""
        msg_type = data.get("type")
        
        # S2C_NEW_BID = 904 (Có người trả giá mới)
        if msg_type == 904:
            self.lbl_current_price.config(text=f"Giá hiện tại: {data.get('current_price', 0):,} VND")
            self.lbl_bidder.config(text=f"Người đang giữ giá: {data.get('bidder', '---')}")
            
        # S2C_AUCTION_ENDED = 906 (Kết thúc phiên)
        elif msg_type == 906:
            self.lbl_timer.config(text="PHIÊN KẾT THÚC", fg="white", bg="#B71C1C")
            self.btn_bid.config(state="disabled", bg="#ccc")
            self.btn_buy_now.config(state="disabled", bg="#ccc")
            
        # S2C_NEW_ITEM_PENDING = 902 (Chuyển sang vật phẩm tiếp theo)
        elif msg_type == 902:
            self.reset_ui_state()
            self.lbl_item_name.config(text=f"Vật phẩm: {data.get('title', '---')}")
            self.lbl_current_price.config(text=f"Giá khởi điểm: {data.get('start_price', 0):,} VND")
            self.lbl_bidder.config(text="Người giữ giá: Chưa có")

    def update_timer(self, time_left):
        """Cập nhật đồng hồ dựa trên S2C_TIME_ALERT (905)"""
        minutes = time_left // 60
        seconds = time_left % 60
        self.lbl_timer.config(text=f"THỜI GIAN: {minutes:02d}:{seconds:02d}")
        
        # Đổi màu cảnh báo khi sắp hết giờ (Dưới 30 giây)
        if time_left <= 30:
            self.lbl_timer.config(fg="#FF5252") # Màu đỏ sáng
            if time_left % 2 == 0: # Tạo hiệu ứng nháy nhẹ
                self.timer_container.config(bg="red")
            else:
                self.timer_container.config(bg="black")
        else:
            self.lbl_timer.config(fg="yellow")
            self.timer_container.config(bg="black")

    def send_bid(self):
        """Gửi lệnh đặt giá C2S_BID (401)"""
        val = self.ent_bid.get().strip()
        if not val: return
        
        try:
            amount = int(val)
            # Gửi gói tin JSON sang Client Daemon (C)
            self.controller.backend.send_command({
                "type": 401, 
                "price": amount
            })
            self.ent_bid.delete(0, tk.END)
        except ValueError:
            messagebox.showerror("Lỗi nhập liệu", "Vui lòng chỉ nhập số nguyên cho giá thầu.")

    def send_buy_now(self):
        """Gửi lệnh mua ngay C2S_BUY_NOW (402)"""
        if messagebox.askyesno("Xác nhận", "Bạn muốn mua đứt vật phẩm này với giá 'Mua ngay'?"):
            self.controller.backend.send_command({"type": 402})

    def leave_room(self):
        """Gửi lệnh rời phòng C2S_LEAVE_ROOM (204)"""
        self.controller.backend.send_command({"type": 204})