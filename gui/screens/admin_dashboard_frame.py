import tkinter as tk
from tkinter import ttk, messagebox

class AdminDashboardFrame(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        
        tk.Label(self, text="HỆ THỐNG QUẢN TRỊ (ADMIN)", 
                 font=("Arial", 16, "bold"), fg="#C62828").pack(pady=15)

        # Toolbar
        toolbar = tk.Frame(self)
        toolbar.pack(fill="x", padx=20)
        
        tk.Button(toolbar, text="🔄 Tải danh sách User", command=self.refresh_users).pack(side="left", padx=5)
        tk.Button(toolbar, text="🚪 Đăng xuất", bg="#e74c3c", fg="white", 
                  command=self.controller.show_login_screen).pack(side="right", padx=5)

        # Bảng danh sách User
        columns = ("id", "username", "role")
        self.tree = ttk.Treeview(self, columns=columns, show="headings")
        self.tree.heading("id", text="ID")
        self.tree.heading("username", text="Tên đăng nhập")
        self.tree.heading("role", text="Quyền hạn")
        
        self.tree.pack(fill="both", expand=True, padx=20, pady=10)

        # Nút chức năng
        btn_frame = tk.Frame(self)
        btn_frame.pack(pady=10)

        tk.Button(btn_frame, text="XÓA USER", bg="#f44336", fg="white", 
                  command=self.delete_user).pack(side="left", padx=10)
        
        self.refresh_users()

    def refresh_users(self):
        # Gọi hàm admin_list_users (Mã 601) đã thêm vào backend_bridge
        self.controller.backend.admin_list_users()

    def update_user_list(self, users):
        for item in self.tree.get_children(): self.tree.delete(item)
        for u in users:
            role_text = {1: "Bidder", 2: "Auctioneer", 3: "Admin"}.get(u['role'], "Unknown")
            self.tree.insert("", "end", values=(u['id'], u['username'], role_text))

    def delete_user(self):
        selected = self.tree.selection()
        if not selected: return
        user_id = self.tree.item(selected[0])['values'][0]
        if messagebox.askyesno("Xác nhận", f"Xóa người dùng ID {user_id}?"):
            self.controller.backend.admin_delete_user(user_id)
            self.after(500, self.refresh_users)