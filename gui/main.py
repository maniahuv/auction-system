import tkinter as tk
from tkinter import messagebox
import json
import os

# Import các thành phần logic
from backend_bridge import AuctionBackend
from screens.dashboard_frame import DashboardFrame
from screens.auction_room_frame import AuctionRoomFrame

class AuctionApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hệ thống Đấu giá Trực tuyến")
        self.root.geometry("900x700")
        
        self.user_role = 0
        self.current_frame = None

        # 1. Khởi tạo Backend Bridge (C-Daemon connection)
        self.backend = AuctionBackend()
        self.backend.start_listening(self.queue_server_response)

        # 2. Container chính chứa các màn hình
        self.container = tk.Frame(self.root)
        self.container.pack(fill="both", expand=True)

        # 3. Vào màn hình đăng nhập mặc định
        self.show_login_screen()

    # --- ĐIỀU PHỐI GIAO DIỆN (UI COORDINATION) ---

    def queue_server_response(self, data):
        """Đảm bảo cập nhật UI luôn chạy trên luồng chính của Tkinter"""
        self.root.after(0, self.handle_server_response, data)

    def clear_frame(self):
        """Xóa sạch màn hình cũ trước khi vẽ màn hình mới"""
        if self.current_frame:
            self.current_frame.destroy()

    def show_login_screen(self):
        self.clear_frame()
        self.current_frame = tk.Frame(self.container)
        self.current_frame.pack(expand=True)

        tk.Label(self.current_frame, text="ĐĂNG NHẬP", font=("Arial", 18, "bold")).pack(pady=20)
        
        tk.Label(self.current_frame, text="Tên đăng nhập:").pack()
        self.ent_user = tk.Entry(self.current_frame, width=30)
        self.ent_user.pack(pady=5)

        tk.Label(self.current_frame, text="Mật khẩu:").pack()
        self.ent_pass = tk.Entry(self.current_frame, show="*", width=30)
        self.ent_pass.pack(pady=5)

        tk.Button(self.current_frame, text="Đăng nhập", bg="#2196F3", fg="white",
                  font=("Arial", 10, "bold"), width=20, command=self.login).pack(pady=10)
        
        tk.Button(self.current_frame, text="Chưa có tài khoản? Đăng ký ngay", 
                  relief="flat", command=self.show_register_screen).pack()

    def show_register_screen(self):
        self.clear_frame()
        self.current_frame = tk.Frame(self.container)
        self.current_frame.pack(expand=True)

        tk.Label(self.current_frame, text="ĐĂNG KÝ TÀI KHOẢN", font=("Arial", 16, "bold")).pack(pady=20)
        
        tk.Label(self.current_frame, text="Tên đăng nhập:").pack()
        self.ent_reg_user = tk.Entry(self.current_frame, width=30)
        self.ent_reg_user.pack(pady=5)

        tk.Label(self.current_frame, text="Mật khẩu:").pack()
        self.ent_reg_pass = tk.Entry(self.current_frame, show="*", width=30)
        self.ent_reg_pass.pack(pady=5)

        tk.Label(self.current_frame, text="Vai trò:").pack()
        self.role_var = tk.IntVar(value=1)
        tk.Radiobutton(self.current_frame, text="Người mua (Bidder)", variable=self.role_var, value=1).pack()
        tk.Radiobutton(self.current_frame, text="Người đấu giá (Auctioneer)", variable=self.role_var, value=2).pack()

        tk.Button(self.current_frame, text="Đăng ký", bg="#4CAF50", fg="white",
                  width=20, command=self.register).pack(pady=20)
        
        tk.Button(self.current_frame, text="Quay lại Đăng nhập", command=self.show_login_screen).pack()

    def show_dashboard(self):
        self.clear_frame()
        self.dashboard_frame = DashboardFrame(self.container, self)
        self.dashboard_frame.pack(fill="both", expand=True)
        self.current_frame = self.dashboard_frame

    def show_auction_room(self, room_id):
        self.clear_frame()
        self.room_frame = AuctionRoomFrame(self.container, self)
        self.room_frame.set_room_info(room_id)
        self.room_frame.pack(fill="both", expand=True)
        self.current_frame = self.room_frame

    # --- XỬ LÝ LỆNH (COMMAND HANDLING) ---

    def login(self):
        user = self.ent_user.get()
        pw = self.ent_pass.get()
        if user and pw:
            self.backend.send_command({"type": 102, "user": user, "pass": pw})

    def register(self):
        user = self.ent_reg_user.get()
        pw = self.ent_reg_pass.get()
        role = self.role_var.get()
        if user and pw:
            self.backend.send_command({"type": 101, "user": user, "pass": pw, "role": role})

    # --- PHẢN HỒI TỪ SERVER (SERVER RESPONSE LOGIC) ---

    def handle_server_response(self, data):
        print(f"DEBUG: Server -> {data}") 
        msg_type = data.get("type")
        
        # 802: S2C_LOGIN_SUCCESS
        if msg_type == 802: 
            self.user_role = data.get("role")
            self.show_dashboard()
            # Tự động load danh sách phòng sau 200ms
            self.root.after(200, lambda: self.backend.send_command({"type": 201}))

        # 801: S2C_GENERIC_ERROR
        elif msg_type == 801: 
            messagebox.showerror("Lỗi", data.get("message") or "Thao tác thất bại")

        # 800: S2C_GENERIC_OK
        elif msg_type == 800:
            messagebox.showinfo("Thành công", data.get("message") or "Thao tác hoàn tất")
            # Nếu đang ở màn hình đăng ký mà thành công thì quay về login
            if "REGISTER" in str(data.get("message", "")).upper() or not self.user_role:
                self.show_login_screen()

        # 810: S2C_ROOM_LIST
        elif msg_type == 810:
            if hasattr(self, 'dashboard_frame'):
                self.dashboard_frame.update_room_list(data.get("rooms", []))

        # 812: S2C_HISTORY_LIST
        elif msg_type == 812:
            # Bạn có thể tạo thêm HistoryFrame hoặc in ra messagebox tạm thời
            history = data.get("history", [])
            history_str = "\n".join([f"- {h['item']}: {h['price']} VND (Thắng: {h['winner']})" for h in history])
            messagebox.showinfo("Lịch sử đấu giá", history_str if history_str else "Chưa có dữ liệu")

        # 901: S2C_JOIN_ROOM_SUCCESS
        elif msg_type == 901:
            self.show_auction_room(data.get("room_id"))

        # 904: S2C_NEW_BID (Cập nhật giá trong phòng)
        elif msg_type == 904:
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)

        # 905: S2C_TIME_ALERT (Đồng hồ đếm ngược)
        elif msg_type == 905:
            if hasattr(self, 'room_frame'):
                self.room_frame.update_timer(data.get("time_left"))

        # 902: S2C_NEW_ITEM_PENDING (Vật phẩm tiếp theo)
        elif msg_type == 902:
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)

        # 906: S2C_AUCTION_ENDED (Kết thúc phiên)
        elif msg_type == 906:
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)
                winner = data.get("winner", "Không có")
                price = data.get("final_price", 0)
                messagebox.showinfo("Kết quả", f"Phiên đấu giá kết thúc!\nNgười thắng: {winner}\nGiá cuối: {price:,} VND")

if __name__ == "__main__":
    root = tk.Tk()
    app = AuctionApp(root)
    
    # Đảm bảo tắt backend khi đóng cửa sổ để tránh Broken Pipe sau này
    def on_closing():
        app.backend.stop()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()