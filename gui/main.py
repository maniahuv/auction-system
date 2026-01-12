import tkinter as tk
from tkinter import ttk, messagebox
import json
import os
from datetime import datetime

# Import các thành phần giao diện và logic
from backend_bridge import AuctionBackend
from screens.dashboard_frame import DashboardFrame
from screens.auction_room_frame import AuctionRoomFrame

class AuctionApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hệ thống Đấu giá Trực tuyến")
        self.root.geometry("1000x750")

        # Trạng thái ứng dụng
        self.user_role = 0  # 1: Bidder, 2: Auctioneer, 3: Admin
        self.username = ""
        self.current_screen_name = ""
        self.current_frame = None

        # 1. Khởi tạo Backend Bridge
        self.backend = AuctionBackend()
        self.backend.start_listening(self.queue_server_response)

        # 2. Container chính
        self.container = tk.Frame(self.root)
        self.container.pack(fill="both", expand=True)

        # 3. Màn hình đầu tiên
        self.show_login_screen()

    # --- TIỆN ÍCH ĐIỀU HƯỚNG & QUẢN LÝ GIAO DIỆN ---

    def queue_server_response(self, data):
        """Đưa phản hồi từ thread backend vào hàng đợi xử lý của Main Thread Tkinter."""
        try:
            self.root.after(0, self.handle_server_response, data)
        except Exception:
            pass

    def clear_frame(self):
        """Xóa màn hình hiện tại một cách an toàn."""
        if self.current_frame is not None:
            try:
                # 1. Hủy timer của AuctionRoom nếu có
                if hasattr(self, 'room_frame') and self.current_frame == self.room_frame:
                    if hasattr(self.room_frame, 'timer_job') and self.room_frame.timer_job:
                        self.root.after_cancel(self.room_frame.timer_job)
                        self.room_frame.timer_job = None
                
                # 2. Destroy frame
                self.current_frame.destroy()
            except Exception as e:
                print(f"Dọn dẹp Frame cũ có cảnh báo: {e}")
            finally:
                self.current_frame = None

    def show_toast(self, message, duration=2500, bg="#333333"):
        """Hiển thị thông báo dạng Toast tự tắt."""
        toast = tk.Toplevel(self.root)
        toast.overrideredirect(True)
        toast.attributes("-topmost", True)

        lbl = tk.Label(toast, text=message, bg=bg, fg="white",
                       padx=25, pady=12, font=("Arial", 11, "bold"))
        lbl.pack()

        toast.update_idletasks()
        # Căn giữa theo cửa sổ chính
        x = self.root.winfo_x() + (self.root.winfo_width() // 2) - (toast.winfo_width() // 2)
        y = self.root.winfo_y() + 100
        toast.geometry(f"+{x}+{y}")
        
        self.root.after(duration, toast.destroy)

    # --- ĐIỀU HƯỚNG MÀN HÌNH ---

    def show_login_screen(self):
        if self.current_screen_name == "login": return
        self.clear_frame()
        self.current_screen_name = "login"
        
        self.current_frame = tk.Frame(self.container)
        self.current_frame.pack(expand=True)

        tk.Label(self.current_frame, text="HỆ THỐNG ĐẤU GIÁ", font=("Arial", 22, "bold"), fg="#1976D2").pack(pady=(0, 30))
        tk.Label(self.current_frame, text="ĐĂNG NHẬP", font=("Arial", 16, "bold")).pack(pady=10)

        tk.Label(self.current_frame, text="Tên đăng nhập:").pack(anchor="w")
        self.ent_user = tk.Entry(self.current_frame, width=35, font=("Arial", 11))
        self.ent_user.pack(pady=5)
        self.ent_user.focus_set()

        tk.Label(self.current_frame, text="Mật khẩu:").pack(anchor="w", pady=(10, 0))
        self.ent_pass = tk.Entry(self.current_frame, show="*", width=35, font=("Arial", 11))
        self.ent_pass.pack(pady=5)
        
        # Bắt sự kiện Enter để đăng nhập
        self.root.bind('<Return>', lambda e: self.login())

        tk.Button(self.current_frame, text="ĐĂNG NHẬP", bg="#2196F3", fg="white",
                  font=("Arial", 11, "bold"), width=25, pady=8, command=self.login).pack(pady=25)

        tk.Button(self.current_frame, text="Chưa có tài khoản? Đăng ký ngay",
                  relief="flat", fg="#666", command=self.show_register_screen).pack()

    def show_register_screen(self):
        self.clear_frame()
        self.current_screen_name = "register"
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
        tk.Label(self.current_frame, text="Loại tài khoản:").pack(anchor="w", pady=(10, 0))
        tk.Radiobutton(self.current_frame, text="Người tham gia đấu giá (Bidder)", variable=self.role_var, value=1).pack(anchor="w")
        tk.Radiobutton(self.current_frame, text="Người tổ chức bán hàng (Auctioneer)", variable=self.role_var, value=2).pack(anchor="w")

        tk.Button(self.current_frame, text="ĐĂNG KÝ NGAY", bg="#4CAF50", fg="white",
                  font=("Arial", 11, "bold"), width=25, pady=8, command=self.register).pack(pady=25)

        tk.Button(self.current_frame, text="Quay lại đăng nhập", relief="flat", fg="#666", command=self.show_login_screen).pack()

    def show_dashboard(self):
        self.clear_frame()
        self.current_screen_name = "dashboard"
        self.root.unbind('<Return>') # Hủy bind phím Enter login
        
        self.dashboard_frame = DashboardFrame(self.container, self)
        self.dashboard_frame.pack(fill="both", expand=True)
        self.current_frame = self.dashboard_frame

    def show_auction_room(self, room_id):
        self.clear_frame()
        self.current_screen_name = "auction_room"
        
        self.room_frame = AuctionRoomFrame(self.container, self)
        self.room_frame.set_room_info(room_id)
        self.room_frame.pack(fill="both", expand=True)
        self.current_frame = self.room_frame

    # --- CỬA SỔ POPUP ---

    def show_search_results(self, results):
        search_win = tk.Toplevel(self.root)
        search_win.title("Kết quả tìm kiếm")
        search_win.geometry("700x450")
        search_win.grab_set()

        tk.Label(search_win, text=f"Tìm thấy {len(results)} vật phẩm phù hợp", font=("Arial", 12, "bold"), pady=15).pack()
        
        cols = ("room", "title", "price", "status")
        tree = ttk.Treeview(search_win, columns=cols, show="headings")
        tree.heading("room", text="ID Phòng"); tree.heading("title", text="Tên vật phẩm")
        tree.heading("price", text="Giá hiện tại"); tree.heading("status", text="Trạng thái")
        
        tree.column("room", width=80, anchor="center")
        tree.column("status", width=120, anchor="center")

        for r in results:
            tree.insert("", "end", values=(r.get('room_id'), r.get('title'), f"{r.get('start_price', 0):,} VND", r.get('status')))
        
        tree.pack(fill="both", expand=True, padx=20, pady=10)
        tk.Button(search_win, text="Đóng", width=15, command=search_win.destroy).pack(pady=10)

    def show_history_window(self, history):
        history_win = tk.Toplevel(self.root)
        history_win.title("Lịch sử phiên đấu giá")
        history_win.geometry("850x500")
        history_win.grab_set()

        tk.Label(history_win, text="LỊCH SỬ GIAO DỊCH", font=("Arial", 14, "bold"), pady=15).pack()

        cols = ("time", "item", "owner", "winner", "price")
        tree = ttk.Treeview(history_win, columns=cols, show="headings")
        tree.heading("time", text="Thời gian"); tree.heading("item", text="Vật phẩm")
        tree.heading("owner", text="Người bán"); tree.heading("winner", text="Người thắng")
        tree.heading("price", text="Giá chốt")

        for h in history:
            dt = datetime.fromtimestamp(h.get('time', 0)).strftime('%Y-%m-%d %H:%M')
            tree.insert("", "end", values=(dt, h.get('item'), h.get('owner', '---'), h.get('winner'), f"{h.get('price', 0):,} VND"))

        tree.pack(fill="both", expand=True, padx=20, pady=10)
        tk.Button(history_win, text="Đóng", width=15, command=history_win.destroy).pack(pady=10)

    def show_admin_user_management(self, users):
        from screens.admin_dashboard_frame import AdminDashboardFrame
        # Sử dụng Admin Frame đã tách thay vì tạo code Inline để dễ bảo trì
        self.clear_frame()
        self.current_screen_name = "admin"
        self.current_frame = AdminDashboardFrame(self.container, self, users)
        self.current_frame.pack(fill="both", expand=True)

    # --- LOGIC GỬI LỆNH ---

    def login(self):
        u, p = self.ent_user.get().strip(), self.ent_pass.get().strip()
        if not u or not p:
            messagebox.showwarning("Thông báo", "Vui lòng nhập đầy đủ thông tin")
            return
        self.username = u
        self.backend.send_command({"type": 102, "user": u, "pass": p})

    def register(self):
        u, p, r = self.ent_reg_user.get().strip(), self.ent_reg_pass.get().strip(), self.role_var.get()
        if not u or not p:
            messagebox.showwarning("Thông báo", "Vui lòng nhập đầy đủ thông tin")
            return
        self.backend.send_command({"type": 101, "user": u, "pass": p, "role": r})

    # --- BỘ NÃO XỬ LÝ PHẢN HỒI (PROTOCOL HANDLER) ---

    def handle_server_response(self, data):
        msg_type = data.get("type")
        
        # 1. Quản lý trạng thái kết nối
        if msg_type == 998: # LOST_CONNECTION
            self.show_toast("⚠️ Mất kết nối! Đang thử lại...", duration=3000, bg="#FFA000")
            self.root.title("Hệ thống Đấu giá (Đang mất kết nối...)")
            return

        if msg_type == 999: # RECONNECTED
            self.show_toast("✅ Đã kết nối lại Server!", duration=2000, bg="#388E3C")
            self.root.title("Hệ thống Đấu giá Trực tuyến")
            # Khi server C restart, toàn bộ Session biến mất, bắt buộc phải login lại
            self.show_login_screen()
            messagebox.showinfo("Hệ thống", "Đã khôi phục kết nối.\nVui lòng đăng nhập lại để tiếp tục.")
            return

        # 2. Xử lý phản hồi đăng nhập/đăng ký
        if msg_type == 802: # LOGIN SUCCESS
            self.user_role = data.get("role")
            self.show_dashboard()
            self.show_toast(f"Chào mừng {self.username}!", bg="#2196F3")
            # Lấy danh sách phòng ngay sau khi vào dashboard
            self.root.after(300, lambda: self.backend.send_command({"type": 201}))

        elif msg_type == 801: # ERROR
            err = data.get("message", "Thao tác thất bại")
            if "dang nhap o noi khac" in err:
                messagebox.showwarning("An toàn tài khoản", err)
                self.show_login_screen()
            else:
                messagebox.showerror("Lỗi hệ thống", err)

        # 3. Xử lý các phản hồi Generic OK (800)
        elif msg_type == 800:
            msg = data.get("message", "")
            if msg == "LEAVE_SUCCESS":
                self.show_dashboard()
                self.root.after(100, lambda: self.backend.send_command({"type": 201}))
                return
            if msg == "LOGOUT_SUCCESS":
                self.show_login_screen()
                return
            self.show_toast(msg, bg="#2E7D32")

        # 4. Cập nhật dữ liệu Dashboard
        elif msg_type == 810: # ROOM LIST
            if hasattr(self, 'dashboard_frame'):
                self.dashboard_frame.update_room_list(data.get("rooms", []))
        
        elif msg_type == 811: # SEARCH RESULT
            self.show_search_results(data.get("results", []))
            
        elif msg_type == 812: # HISTORY LIST
            self.show_history_window(data.get("history", []))

        elif msg_type == 820: # ADMIN: USER LIST
            self.show_admin_user_management(data.get("users", []))

        # 5. Cập nhật dữ liệu trong Phòng đấu giá (Broadcast)
        elif msg_type == 901: # JOIN SUCCESS (Broadcast)
            self.show_auction_room(data.get("room_id"))
            self.room_frame.update_auction_state(data)

        elif msg_type in [902, 903, 904, 905, 906, 907, 908, 909]:
            if hasattr(self, 'room_frame') and self.current_screen_name == "auction_room":
                self.room_frame.update_auction_state(data)
                
                # Hiệu ứng Toast đặc biệt cho người thắng cuộc
                if msg_type == 906:
                    winner = data.get("winner", "Không rõ")
                    price = data.get("final_price", 0)
                    self.show_toast(f"🏆 {winner} THẮNG PHIÊN ({price:,} VND)", duration=6000, bg="#D32F2F")

if __name__ == "__main__":
    app_root = tk.Tk()
    app = AuctionApp(app_root)
    
    # Xử lý đóng ứng dụng an toàn
    def on_closing():
        if messagebox.askokcancel("Thoát", "Bạn có chắc chắn muốn thoát hệ thống?"):
            app.backend.stop() # Dừng backend daemon
            app_root.destroy()
            
    app_root.protocol("WM_DELETE_WINDOW", on_closing)
    app_root.mainloop()