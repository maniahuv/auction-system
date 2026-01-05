import subprocess
import threading
import json
import os
import sys

class AuctionBackend:
    def __init__(self, binary_path="../bin/clientd"):
        """
        Khởi tạo cầu nối giữa Python và Client Daemon viết bằng C.
        """
        self.binary_path = binary_path
        self.process = None
        self.callback = None
        self._is_running = False

        # Tự động tải cấu hình mạng
        self.host, self.port = self._load_config()

        # 1. Kiểm tra sự tồn tại của file thực thi C
        if not os.path.exists(self.binary_path):
            print(f"CRITICAL ERROR: Không tìm thấy file thực thi tại '{self.binary_path}'")
            print("Vui lòng chạy 'make' trong thư mục clientd trước khi khởi chạy GUI.")
            sys.exit(1)

        try:
            # 2. Khởi chạy tiến trình C (clientd)
            self.process = subprocess.Popen(
                [self.binary_path, self.host, str(self.port)],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            self._is_running = True
            
            # 3. Khởi chạy luồng đọc lỗi (stderr) từ C để debug
            threading.Thread(target=self._read_stderr, daemon=True).start()
            print(f"Hệ thống: Đã khởi chạy Daemon kết nối tới {self.host}:{self.port}")
            
        except Exception as e:
            print(f"LỖI khi khởi chạy Backend Daemon: {e}")
            sys.exit(1)

    def _load_config(self):
        """Đọc file config.json. Nếu không có, dùng mặc định localhost."""
        config_path = os.path.join(os.path.dirname(__file__), 'config.json')
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    config = json.load(f)
                    return config.get("server_host", "127.0.0.1"), config.get("server_port", 8080)
            except Exception as e:
                print(f"Cảnh báo: Lỗi đọc file config: {e}")
        return "127.0.0.1", 8080

    def _read_stderr(self):
        """Đọc và in các lỗi từ phía C ra terminal của Python."""
        while self._is_running and self.process:
            line = self.process.stderr.readline()
            if not line: break
            print(f"[C-Daemon Log]: {line.strip()}")

    def send_command(self, cmd_dict):
        """Gửi một dictionary Python dưới dạng JSON sang phía C qua stdin."""
        if not self.process or self.process.poll() is not None:
            print("LỖI: Tiến trình Client Daemon đã ngừng hoạt động.")
            return False
        try:
            json_str = json.dumps(cmd_dict)
            self.process.stdin.write(json_str + "\n")
            self.process.stdin.flush()
            return True
        except (BrokenPipeError, IOError):
            print("LỖI: Broken Pipe - Client Daemon đã bị ngắt kết nối.")
            self._is_running = False
            return False

    def start_listening(self, callback):
        """Bắt đầu lắng nghe dữ liệu từ stdout của C."""
        self.callback = callback
        threading.Thread(target=self._listen, daemon=True).start()

    def _listen(self):
        """Vòng lặp đọc dữ liệu JSON từ phía C."""
        while self._is_running and self.process:
            line = self.process.stdout.readline()
            if not line: break
            try:
                data = json.loads(line.strip())
                if self.callback: self.callback(data)
            except:
                if line.strip(): print(f"Hệ thống: Dữ liệu thô: {line.strip()}")

    def stop(self):
        """Dừng tiến trình C an toàn."""
        self._is_running = False
        if self.process:
            self.process.terminate()
            print("Hệ thống: Đã dừng Backend Daemon.")

    # --- NHÓM 1: XÁC THỰC (AUTH) ---

    def register(self, username, password, role):
        return self.send_command({"type": 101, "user": username, "pass": password, "role": role})

    def login(self, username, password):
        return self.send_command({"type": 102, "user": username, "pass": password})

    def logout(self):
        """Gửi yêu cầu đăng xuất (Mã 103)"""
        return self.send_command({"type": 103})

    # --- NHÓM 2: QUẢN LÝ PHÒNG (ROOM) ---

    def list_rooms(self):
        return self.send_command({"type": 201})

    def create_room(self, room_name, first_item_title, start_price, buy_now):
        return self.send_command({
            "type": 202, "room_name": room_name, "title": first_item_title,
            "start_price": int(start_price), "buy_now": int(buy_now)
        })

    def join_room(self, room_id):
        return self.send_command({"type": 203, "room_id": int(room_id)})

    def leave_room(self):
        return self.send_command({"type": 204})

    def search_item(self, keyword):
        return self.send_command({"type": 205, "keyword": keyword})

    def start_auction(self):
        return self.send_command({"type": 206})

    def send_chat(self, message):
        return self.send_command({"type": 207, "message": message})

    # --- NHÓM 3: QUẢN LÝ VẬT PHẨM (ITEM) ---

    def add_item(self, room_id, title, start_price, buy_now):
        return self.send_command({
            "type": 301, "room_id": int(room_id), "title": title,
            "start_price": int(start_price), "buy_now": int(buy_now)
        })

    def delete_item(self, room_id, item_index):
        return self.send_command({"type": 302, "room_id": int(room_id), "item_index": int(item_index)})

    def update_item(self, room_id, item_index, title, start_price, buy_now):
        """Gửi yêu cầu cập nhật vật phẩm (Mã 304)"""
        return self.send_command({
            "type": 304, "room_id": int(room_id), "item_index": int(item_index),
            "title": title, "start_price": int(start_price), "buy_now": int(buy_now)
        })

    # --- NHÓM 4: ĐẤU GIÁ (BIDDING) ---

    def bid(self, price):
        return self.send_command({"type": 401, "price": int(price)})

    def buy_now(self):
        return self.send_command({"type": 402})

    def get_history(self):
        return self.send_command({"type": 501})

    # --- NHÓM 6: QUẢN LÝ ADMIN (ADMIN MGMT) ---

    def admin_list_users(self):
        """Admin: Lấy danh sách tất cả người dùng (Mã 601)"""
        return self.send_command({"type": 601})

    def admin_delete_user(self, user_id):
        """Admin: Xóa một người dùng (Mã 602)"""
        return self.send_command({"type": 602, "user_id": int(user_id)})

    def admin_update_role(self, user_id, new_role):
        """Admin: Thay đổi quyền người dùng (Mã 603)"""
        return self.send_command({"type": 603, "user_id": int(user_id), "new_role": int(new_role)})