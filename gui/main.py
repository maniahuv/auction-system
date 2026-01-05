import tkinter as tk
from tkinter import ttk, messagebox
import json
import os

# Import các thành phần giao diện và logic
from backend_bridge import AuctionBackend
from screens.dashboard_frame import DashboardFrame
from screens.auction_room_frame import AuctionRoomFrame

class AuctionApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hệ thống Đấu giá Trực tuyến")
        self.root.geometry("950x750")
        
        # Trạng thái ứng dụng
        self.user_role = 0  # 1: Bidder, 2: Auctioneer
        self.current_frame = None

        # 1. Khởi tạo Backend Bridge
        self.backend = AuctionBackend()
        self.backend.start_listening(self.queue_server_response)

        # 2. Container chính
        self.container = tk.Frame(self.root)
        self.container.pack(fill="both", expand=True)

        # 3. Màn hình đầu tiên
        self.show_login_screen()

    # --- TIỆN ÍCH GIAO DIỆN (UI UTILS) ---

    def show_toast(self, message, duration=2500, bg="#333333"):
        """Hiển thị thông báo tự tắt sau một khoảng thời gian (Toast Notification)"""
        toast = tk.Toplevel(self.root)
        toast.overrideredirect(True)  # Xóa viền cửa sổ
        toast.attributes("-topmost", True)  # Luôn nằm trên cùng
        
        lbl = tk.Label(toast, text=message, bg=bg, fg="white", 
                       padx=25, pady=12, font=("Arial", 11, "bold"))
        lbl.pack()

        # Cập nhật để lấy kích thước chính xác
        toast.update_idletasks()
        
        # Tính toán vị trí hiển thị giữa màn hình chính
        main_x = self.root.winfo_x()
        main_y = self.root.winfo_y()
        main_width = self.root.winfo_width()
        
        x = main_x + (main_width // 2) - (toast.winfo_width() // 2)
        y = main_y + 100  # Hiển thị ở phía trên một chút cho dễ nhìn
        
        toast.geometry(f"+{x}+{y}")
        
        # Tự động đóng
        self.root.after(duration, toast.destroy)

    def queue_server_response(self, data):
        self.root.after(0, self.handle_server_response, data)

    def clear_frame(self):
        if self.current_frame:
            self.current_frame.destroy()

    # --- CHUYỂN ĐỔI MÀN HÌNH (SCREENS) ---

    def show_login_screen(self):
        self.clear_frame()
        self.current_frame = tk.Frame(self.container)
        self.current_frame.pack(expand=True)

        tk.Label(self.current_frame, text="HỆ THỐNG ĐẤU GIÁ", font=("Arial", 22, "bold"), fg="#1976D2").pack(pady=(0, 30))
        tk.Label(self.current_frame, text="ĐĂNG NHẬP", font=("Arial", 16, "bold")).pack(pady=10)
        
        tk.Label(self.current_frame, text="Tên đăng nhập:").pack(anchor="w")
        self.ent_user = tk.Entry(self.current_frame, width=35, font=("Arial", 11))
        self.ent_user.pack(pady=5)

        tk.Label(self.current_frame, text="Mật khẩu:").pack(anchor="w", pady=(10, 0))
        self.ent_pass = tk.Entry(self.current_frame, show="*", width=35, font=("Arial", 11))
        self.ent_pass.pack(pady=5)

        tk.Button(self.current_frame, text="ĐĂNG NHẬP", bg="#2196F3", fg="white",
                  font=("Arial", 11, "bold"), width=25, pady=8, command=self.login).pack(pady=25)
        
        tk.Button(self.current_frame, text="Chưa có tài khoản? Đăng ký ngay", 
                  relief="flat", fg="#666", command=self.show_register_screen).pack()

    def show_register_screen(self):
        self.clear_frame()
        self.current_frame = tk.Frame(self.container)
        self.current_frame.pack(expand=True)

        tk.Label(self.current_frame, text="ĐĂNG KÝ TÀI KHOẢN", font=("Arial", 18, "bold"), fg="#388E3C").pack(pady=30)
        
        tk.Label(self.current_frame, text="Tên đăng nhập:").pack(anchor="w")
        self.ent_reg_user = tk.Entry(self.current_frame, width=35, font=("Arial", 11))
        self.ent_reg_user.pack(pady=5)

        tk.Label(self.current_frame, text="Mật khẩu:").pack(anchor="w", pady=(10, 0))
        self.ent_reg_pass = tk.Entry(self.current_frame, show="*", width=35, font=("Arial", 11))
        self.ent_reg_pass.pack(pady=5)

        self.role_var = tk.IntVar(value=1)
        tk.Radiobutton(self.current_frame, text="Người mua (Bidder)", variable=self.role_var, value=1).pack(anchor="w")
        tk.Radiobutton(self.current_frame, text="Người bán (Auctioneer)", variable=self.role_var, value=2).pack(anchor="w")

        tk.Button(self.current_frame, text="ĐĂNG KÝ NGAY", bg="#4CAF50", fg="white",
                  font=("Arial", 11, "bold"), width=25, pady=8, command=self.register).pack(pady=25)
        
        tk.Button(self.current_frame, text="Quay lại đăng nhập", relief="flat", fg="#666", command=self.show_login_screen).pack()

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

    def show_search_results(self, results):
        search_win = tk.Toplevel(self.root)
        search_win.title("Kết quả tìm kiếm")
        search_win.geometry("700x450")
        search_win.grab_set()
        
        tk.Label(search_win, text=f"Tìm thấy {len(results)} vật phẩm phù hợp", font=("Arial", 12, "bold"), pady=15).pack()
        cols = ("room", "title", "price", "status")
        tree = ttk.Treeview(search_win, columns=cols, show="headings")
        tree.heading("room", text="ID Phòng"); tree.heading("title", text="Tên vật phẩm")
        tree.heading("price", text="Giá khởi điểm"); tree.heading("status", text="Trạng thái")
        
        for r in results:
            tree.insert("", "end", values=(r.get('room_id'), r.get('title'), f"{r.get('start_price', 0):,} VND", r.get('status')))
        tree.pack(fill="both", expand=True, padx=20, pady=10)
        tk.Button(search_win, text="Đóng", width=15, command=search_win.destroy).pack(pady=10)

    # --- LỆNH BACKEND ---

    def login(self):
        u, p = self.ent_user.get().strip(), self.ent_pass.get().strip()
        if u and p: self.backend.send_command({"type": 102, "user": u, "pass": p})

    def register(self):
        u, p, r = self.ent_reg_user.get().strip(), self.ent_reg_pass.get().strip(), self.role_var.get()
        if u and p: self.backend.send_command({"type": 101, "user": u, "pass": p, "role": r})

    # --- XỬ LÝ PROTOCOL ---

    def handle_server_response(self, data):
        msg_type = data.get("type")
        
        if msg_type == 802: # Login Success
            self.user_role = data.get("role")
            self.show_dashboard()
            self.show_toast(f"Chào mừng quay trở lại!", bg="#2196F3")
            self.root.after(200, lambda: self.backend.send_command({"type": 201}))

        elif msg_type == 801: # Server Error
            messagebox.showerror("Lỗi", data.get("message") or "Thao tác thất bại")

        elif msg_type == 800: # Generic OK
            msg_text = data.get("message", "")
            if msg_text == "BID_SUCCESS":
                self.show_toast("✅ Bạn đang dẫn đầu mức giá!", bg="#4CAF50")
                return 
            if msg_text == "LEAVE_SUCCESS":
                self.show_dashboard()
                self.show_toast("Đã rời phòng thành công", duration=1500)
                self.root.after(100, lambda: self.backend.send_command({"type": 201}))
                return
            
            self.show_toast(msg_text, bg="#2E7D32")
            if "REGISTER" in str(msg_text).upper(): self.show_login_screen()

        elif msg_type == 810: # Room Update
            if hasattr(self, 'dashboard_frame'): self.dashboard_frame.update_room_list(data.get("rooms", []))

        elif msg_type == 811: self.show_search_results(data.get("results", []))

        elif msg_type == 812: # History
            history = data.get("history", [])
            h_str = "\n".join([f"• {h['item']}: {h['price']:,} VND" for h in history])
            messagebox.showinfo("Lịch sử", h_str if h_str else "Trống")

        # --- NHÓM 9xx (Room Controls) ---

        elif msg_type == 901: # Join Room
            self.show_auction_room(data.get("room_id"))
            self.room_frame.update_auction_state(data)

        elif msg_type == 903: # Auction Started
            if hasattr(self, 'room_frame'): self.room_frame.update_auction_state(data)

        elif msg_type == 904: # New Bid
            if hasattr(self, 'room_frame'): self.room_frame.update_auction_state(data)

        elif msg_type == 905: # Time Alert
            if hasattr(self, 'room_frame'): self.room_frame.update_timer(data.get("time_left"))

        elif msg_type == 902: # New Item
            if hasattr(self, 'room_frame'): self.room_frame.update_auction_state(data)

        elif msg_type == 906: # Item Ended
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)
                winner = data.get("winner", "Không có")
                price = data.get("final_price", 0)
                self.show_toast(f"🏆 {winner} thắng ({price:,} VND)", duration=5000, bg="#B71C1C")

        elif msg_type == 907: # Queue Update
            if hasattr(self, 'room_frame'): self.room_frame.update_queue_list(data.get("queue"))

        elif msg_type == 908: # Chat
            if hasattr(self, 'room_frame'): self.room_frame.display_chat_message(data.get("username"), data.get("message"))

if __name__ == "__main__":
    app_root = tk.Tk()
    app = AuctionApp(app_root)
    app_root.protocol("WM_DELETE_WINDOW", lambda: (app.backend.stop(), app_root.destroy()))
    app_root.mainloop()