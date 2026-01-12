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
        self.root.geometry("950x750")

        # Trạng thái ứng dụng
        self.user_role = 0  # 1: Bidder, 2: Auctioneer, 3: Admin
        self.current_room_name = ""  # Lưu tên phòng hiện tại để hiển thị trong AuctionRoomFrame
        self.current_frame = None

        # 1. Khởi tạo Backend Bridge
        self.backend = AuctionBackend()
        self.backend.start_listening(self.queue_server_response)

        # 2. Container chính
        self.container = tk.Frame(self.root)
        self.container.pack(fill="both", expand=True)

        # 3. Màn hình đầu tiên
        self.show_login_screen()

    # --- TIỆN ÍCH ĐIỀU HƯỚNG (NAVIGATION UTILS) ---

    def show_frame(self, frame_name):
        """Phương thức tiện ích để chuyển đổi màn hình từ các Frame con."""
        if frame_name == "LoginFrame":
            self.show_login_screen()
        elif frame_name == "DashboardFrame":
            self.show_dashboard()

    def show_toast(self, message, duration=2500, bg="#333333"):
        """Hiển thị thông báo tự tắt sau một khoảng thời gian."""
        toast = tk.Toplevel(self.root)
        toast.overrideredirect(True)
        toast.attributes("-topmost", True)

        lbl = tk.Label(toast, text=message, bg=bg, fg="white",
                       padx=25, pady=12, font=("Arial", 11, "bold"))
        lbl.pack()

        toast.update_idletasks()
        main_x = self.root.winfo_x()
        main_y = self.root.winfo_y()
        main_width = self.root.winfo_width()

        x = main_x + (main_width // 2) - (toast.winfo_width() // 2)
        y = main_y + 100

        toast.geometry(f"+{x}+{y}")
        self.root.after(duration, toast.destroy)

    def queue_server_response(self, data):
        self.root.after(0, self.handle_server_response, data)

    def clear_frame(self):
        # Đặc biệt: Nếu đang ở phòng đấu giá, cần hủy timer trước khi destroy
        if hasattr(self, 'room_frame') and self.current_frame == self.room_frame:
            if self.room_frame.timer_job:
                self.root.after_cancel(self.room_frame.timer_job)
                self.room_frame.timer_job = None

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

    # --- LỊCH SỬ DẠNG BẢNG ---

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
        tree.heading("price", text="Giá trúng")

        for h in history:
            dt = datetime.fromtimestamp(h.get('time', 0)).strftime('%Y-%m-%d %H:%M')
            tree.insert("", "end", values=(dt, h.get('item'), h.get('owner', '---'), h.get('winner'), f"{h.get('price', 0):,} VND"))

        tree.pack(fill="both", expand=True, padx=20, pady=10)
        tk.Button(history_win, text="Đóng", width=15, command=history_win.destroy).pack(pady=10)

    # --- QUẢN LÝ ADMIN ---

    def show_admin_user_management(self, users):
        admin_win = tk.Toplevel(self.root)
        admin_win.title("Quản lý người dùng (Admin)")
        admin_win.geometry("650x550")
        admin_win.grab_set()

        cols = ("id", "username", "role")
        tree = ttk.Treeview(admin_win, columns=cols, show="headings")
        tree.heading("id", text="ID"); tree.heading("username", text="Username"); tree.heading("role", text="Role")

        role_map = {1: "Bidder", 2: "Auctioneer", 3: "Admin"}
        for u in users: tree.insert("", "end", values=(u.get('id'), u.get('username'), role_map.get(u.get('role'), "N/A")))
        tree.pack(fill="both", expand=True, padx=20, pady=10)

        btn_frame = tk.Frame(admin_win); btn_frame.pack(pady=10)

        def update_role():
            selected = tree.selection()
            if not selected: return
            uid = tree.item(selected[0])['values'][0]
            role_win = tk.Toplevel(admin_win); role_win.title("Sửa quyền")
            new_role_var = tk.IntVar(value=1)
            tk.Radiobutton(role_win, text="Bidder", variable=new_role_var, value=1).pack()
            tk.Radiobutton(role_win, text="Auctioneer", variable=new_role_var, value=2).pack()
            tk.Radiobutton(role_win, text="Admin", variable=new_role_var, value=3).pack()
            def confirm():
                self.backend.send_command({"type": 603, "user_id": int(uid), "new_role": new_role_var.get()})
                role_win.destroy(); admin_win.destroy(); self.root.after(500, lambda: self.backend.send_command({"type": 601}))
            tk.Button(role_win, text="Xác nhận", command=confirm).pack(pady=10)

        def delete_user():
            selected = tree.selection()
            if not selected: return
            uid = tree.item(selected[0])['values'][0]
            if messagebox.askyesno("Xác nhận", f"Xóa ID {uid}?"):
                self.backend.send_command({"type": 602, "user_id": int(uid)})
                admin_win.destroy(); self.root.after(500, lambda: self.backend.send_command({"type": 601}))

        tk.Button(btn_frame, text="Sửa quyền", bg="#2196F3", fg="white", command=update_role, width=15).pack(side="left", padx=10)
        tk.Button(btn_frame, text="Xóa người dùng", bg="#f44336", fg="white", command=delete_user, width=15).pack(side="left", padx=10)
        tk.Button(btn_frame, text="Đóng", command=admin_win.destroy, width=15).pack(side="left", padx=10)

    # --- LỆNH BACKEND ---

    def login(self):
        u, p = self.ent_user.get().strip(), self.ent_pass.get().strip()
        if u and p: self.backend.send_command({"type": 102, "user": u, "pass": p})

    def register(self):
        u, p, r = self.ent_reg_user.get().strip(), self.ent_reg_pass.get().strip(), self.role_var.get()
        if u and p: self.backend.send_command({"type": 101, "user": u, "pass": p, "role": r})

    # --- XỬ LÝ PHẢN HỒI ---

    def handle_server_response(self, data):
        msg_type = data.get("type")
        # --- XỬ LÝ KẾT NỐI MẠNG ---
        if msg_type == 998: # LOST_CONNECTION
            self.show_toast("⚠️ Mất kết nối! Đang thử lại...", duration=3000, bg="#FFA000")
            self.root.title("Hệ thống Đấu giá (Đang mất kết nối...)")
            return

        if msg_type == 999: # RECONNECTED
            self.show_toast("✅ Đã kết nối lại Server!", duration=2000, bg="#388E3C")
            self.root.title("Hệ thống Đấu giá Trực tuyến")
            # Vì Server C mới khởi động lại nên Session bị mất.
            # Ta cần đưa người dùng về màn hình đăng nhập để họ vào lại.
            self.show_login_screen()
            messagebox.showinfo("Thông báo", "Kết nối đã được khôi phục.\nVui lòng đăng nhập lại.")
            return
        if msg_type == 802: # Login Success
            self.user_role = data.get("role")
            self.show_dashboard()
            self.show_toast(f"Chào mừng quay trở lại!", bg="#2196F3")
            self.root.after(200, lambda: self.backend.send_command({"type": 201}))

        elif msg_type == 801: # Error
            error_msg = data.get("message", "")

            # LOGIC KICK OUT: Khi tài khoản đăng nhập ở nơi khác
            if error_msg == "Tai khoan da dang nhap o noi khac!":
                messagebox.showwarning("Cảnh báo hệ thống", error_msg)
                self.show_login_screen()
                return

            messagebox.showerror("Lỗi", error_msg or "Thao tác thất bại")

        elif msg_type == 800: # Generic OK
            msg_text = data.get("message", "")
            if msg_text == "LEAVE_SUCCESS":
                self.show_dashboard()
                self.root.after(100, lambda: self.backend.send_command({"type": 201}))
                return
            if msg_text == "LOGOUT_SUCCESS":
                self.show_login_screen()
                return
            self.show_toast(msg_text, bg="#2E7D32")

        elif msg_type == 810:
            if hasattr(self, 'dashboard_frame'): self.dashboard_frame.update_room_list(data.get("rooms", []))
        elif msg_type == 811: self.show_search_results(data.get("results", []))
        elif msg_type == 812:
            history = data.get("history", [])
            if history: self.show_history_window(history)
            else: messagebox.showinfo("Thông báo", "Chưa có lịch sử giao dịch.")
        elif msg_type == 820: self.show_admin_user_management(data.get("users", []))
        elif msg_type == 901:
            self.show_auction_room(data.get("room_id"))
            self.room_frame.update_auction_state(data)
        elif msg_type in [902, 903, 904, 905, 906, 907]:
            if hasattr(self, 'room_frame'):
                self.room_frame.update_auction_state(data)
                if msg_type == 906:
                    w, pr = data.get("winner", "N/A"), data.get("final_price", 0)
                    self.show_toast(f"🏆 {w} thắng ({pr:,} VND)", duration=5000, bg="#B71C1C")
        elif msg_type == 908:
            if hasattr(self, 'room_frame'): self.room_frame.display_chat_message(data.get("username"), data.get("message"))

if __name__ == "__main__":
    app_root = tk.Tk()
    app = AuctionApp(app_root)
    app_root.protocol("WM_DELETE_WINDOW", lambda: (app.backend.stop(), app_root.destroy()))
    app_root.mainloop()
