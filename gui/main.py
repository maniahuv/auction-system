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

        # 1. Khởi tạo Backend Bridge (Kết nối với tiến trình C Daemon)
        self.backend = AuctionBackend()
        self.backend.start_listening(self.queue_server_response)

        # 2. Container chính chứa các màn hình (Frame)
        self.container = tk.Frame(self.root)
        self.container.pack(fill="both", expand=True)

        # 3. Bắt đầu với màn hình đăng nhập
        self.show_login_screen()

    # --- ĐIỀU PHỐI GIAO DIỆN (UI COORDINATION) ---

    def queue_server_response(self, data):
        """Đảm bảo việc cập nhật giao diện luôn chạy trên luồng chính của Tkinter (Thread-safe)"""
        self.root.after(0, self.handle_server_response, data)

    def clear_frame(self):
        """Xóa màn hình hiện tại trước khi chuyển sang màn hình mới"""
        if self.current_frame:
            self.current_frame.destroy()

    def show_login_screen(self):
        self.clear_frame()
        self.current_frame = tk.Frame(self.container)
        self.current_frame.pack(expand=True)

        tk.Label(self.current_frame, text="HỆ THỐNG ĐẤU GIÁ", font=("Arial", 22, "bold"), fg="#1976D2").pack(pady=(0, 30))
        tk.Label(self.current_frame, text="ĐĂNG NHẬP", font=("Arial", 16, "bold")).pack(pady=10)
        
        tk.Label(self.current_frame, text="Tên đăng nhập:", font=("Arial", 10)).pack(anchor="w")
        self.ent_user = tk.Entry(self.current_frame, width=35, font=("Arial", 11))
        self.ent_user.pack(pady=5)

        tk.Label(self.current_frame, text="Mật khẩu:", font=("Arial", 10)).pack(anchor="w", pady=(10, 0))
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
        
        tk.Label(self.current_frame, text="Tên đăng nhập:", font=("Arial", 10)).pack(anchor="w")
        self.ent_reg_user = tk.Entry(self.current_frame, width=35, font=("Arial", 11))
        self.ent_reg_user.pack(pady=5)

        tk.Label(self.current_frame, text="Mật khẩu:", font=("Arial", 10)).pack(anchor="w", pady=(10, 0))
        self.ent_reg_pass = tk.Entry(self.current_frame, show="*", width=35, font=("Arial", 11))
        self.ent_reg_pass.pack(pady=5)

        tk.Label(self.current_frame, text="Vai trò người dùng:", font=("Arial", 10)).pack(anchor="w", pady=(15, 0))
        self.role_var = tk.IntVar(value=1)
        tk.Radiobutton(self.current_frame, text="Người mua (Bidder)", variable=self.role_var, value=1).pack(anchor="w")
        tk.Radiobutton(self.current_frame, text="Người bán (Auctioneer)", variable=self.role_var, value=2).pack(anchor="w")

        tk.Button(self.current_frame, text="ĐĂNG KÝ NGAY", bg="#4CAF50", fg="white",
                  font=("Arial", 11, "bold"), width=25, pady=8, command=self.register).pack(pady=25)
        
        tk.Button(self.current_frame, text="Quay lại đăng nhập", 
                  relief="flat", fg="#666", command=self.show_login_screen).pack()

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
        """Hiển thị kết quả tìm kiếm trong cửa sổ Pop-up chuyên nghiệp"""
        search_win = tk.Toplevel(self.root)
        search_win.title("Kết quả tìm kiếm hệ thống")
        search_win.geometry("700x450")
        search_win.grab_set()
        
        tk.Label(search_win, text=f"Tìm thấy {len(results)} vật phẩm phù hợp", 
                 font=("Arial", 12, "bold"), pady=15).pack()

        cols = ("room", "title", "price", "status")
        tree = ttk.Treeview(search_win, columns=cols, show="headings")
        tree.heading("room", text="ID Phòng")
        tree.heading("title", text="Tên vật phẩm")
        tree.heading("price", text="Giá khởi điểm")
        tree.heading("status", text="Trạng thái")
        
        tree.column("room", width=80, anchor="center")
        tree.column("title", width=250)
        tree.column("price", width=140, anchor="e")
        tree.column("status", width=150, anchor="center")
        
        tree.pack(fill="both", expand=True, padx=20, pady=10)
        
        for r in results:
            tree.insert("", "end", values=(
                r.get('room_id'), 
                r.get('title'), 
                f"{r.get('start_price', 0):,} VND", 
                r.get('status')
            ))
        
        tk.Button(search_win, text="Đóng", width=15, command=search_win.destroy).pack(pady=10)

    # --- XỬ LÝ LỆNH TỚI BACKEND ---

    def login(self):
        user = self.ent_user.get().strip()
        pw = self.ent_pass.get().strip()
        if user and pw:
            self.backend.send_command({"type": 102, "user": user, "pass": pw})

    def register(self):
        user = self.ent_reg_user.get().strip()
        pw = self.ent_reg_pass.get().strip()
        role = self.role_var.get()
        if user and pw:
            self.backend.send_command({"type": 101, "user": user, "pass": pw, "role": role})

    # --- XỬ LÝ PHẢN HỒI TỪ SERVER (PROTOCOL HANDLING) ---

    def handle_server_response(self, data):
        print(f"DEBUG: Server Protocol -> {data}") 
        msg_type = data.get("type")
        
        # --- NHÓM 8xx: Phản hồi hệ thống và Dashboard ---
        
        if msg_type == 802: # Đăng nhập thành công
            self.user_role = data.get("role")
            self.show_dashboard()
            self.root.after(200, lambda: self.backend.send_command({"type": 201}))

        elif msg_type == 801: # Lỗi từ máy chủ
            messagebox.showerror("Lỗi", data.get("message") or "Thao tác thất bại")

        elif msg_type == 800: # Thông báo OK
            msg_text = data.get("message", "")
            
            if msg_text == "BID_SUCCESS":
                messagebox.showinfo("Thành công", "Bạn đang dẫn đầu mức giá!")
                return 

            if msg_text == "LEAVE_SUCCESS":
                self.show_dashboard()
                self.root.after(100, lambda: self.backend.send_command({"type": 201}))
                return

            messagebox.showinfo("Hệ thống", msg_text or "Thành công")
            if "REGISTER" in str(msg_text).upper() or not self.user_role:
                self.show_login_screen()

        elif msg_type == 810: # Cập nhật danh sách phòng
            if hasattr(self, 'dashboard_frame'):
                self.dashboard_frame.update_room_list(data.get("rooms", []))

        elif msg_type == 811: # Kết quả tìm kiếm
            self.show_search_results(data.get("results", []))

        elif msg_type == 812: # Danh sách lịch sử
            history = data.get("history", [])
            h_str = "\n".join([f"• {h['item']}: {h['price']:,} VND (Thắng: {h['winner']})" for h in history])
            messagebox.showinfo("Lịch sử đấu giá", h_str if h_str else "Chưa có dữ liệu")

        # --- NHÓM 9xx: Điều khiển Phòng Đấu giá (Thời gian thực) ---

        elif msg_type == 901: # Vào phòng thành công (Dữ liệu khởi tạo)
            self.show_auction_room(data.get("room_id"))
            self.room_frame.update_auction_state(data)
            if "queue" in data:
                self.room_frame.update_queue_list(data.get("queue"))

        elif msg_type == 904: # Có giá thầu mới
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)

        elif msg_type == 905: # Cảnh báo thời gian
            if hasattr(self, 'room_frame'):
                self.room_frame.update_timer(data.get("time_left"))

        elif msg_type == 902: # CHUYỂN PHIÊN (Vật phẩm mới bắt đầu)
            if hasattr(self, 'room_frame'):
                # Quan trọng: Gọi update_auction_state để reset nút bấm và nhãn
                self.room_frame.update_auction_state(data)

        elif msg_type == 906: # Phiên của vật phẩm hiện tại kết thúc
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)
                winner = data.get("winner", "Không có")
                price = data.get("final_price", 0)
                messagebox.showinfo("Kết thúc phiên", f"Người thắng: {winner}\nGiá cuối: {price:,} VND")

        elif msg_type == 907: # Cập nhật danh sách hàng chờ
            if hasattr(self, 'room_frame'):
                self.room_frame.update_queue_list(data.get("queue"))

if __name__ == "__main__":
    app_root = tk.Tk()
    app = AuctionApp(app_root)
    
    def on_closing():
        app.backend.stop() # Tắt Client Daemon
        app_root.destroy()

    app_root.protocol("WM_DELETE_WINDOW", on_closing)
    app_root.mainloop()