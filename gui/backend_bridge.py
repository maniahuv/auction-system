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

        # --- MỚI: Tự động tải cấu hình mạng ---
        self.host, self.port = self._load_config()

        # 1. Kiểm tra sự tồn tại của file thực thi C
        if not os.path.exists(self.binary_path):
            print(f"CRITICAL ERROR: Không tìm thấy file thực thi tại '{self.binary_path}'")
            print("Vui lòng chạy 'make' trong thư mục clientd trước khi khởi chạy GUI.")
            sys.exit(1)

        try:
            # 2. Khởi chạy tiến trình C (clientd)
            # TRUYỀN THAM SỐ: Chuyền host và port vào argv của chương trình C
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
                    return config.get("server_host"), config.get("server_port")
            except Exception as e:
                print(f"Cảnh báo: Lỗi đọc file config: {e}")
        
        # Mặc định nếu lỗi hoặc không thấy file
        return "127.0.0.1", 8080

    def _read_stderr(self):
        """Đọc và in các lỗi từ phía C ra terminal của Python."""
        while self._is_running and self.process:
            line = self.process.stderr.readline()
            if not line:
                break
            # In các log từ C để dễ dàng debug lỗi kết nối mạng
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
        """Bắt đầu lắng nghe dữ liệu từ stdout của C trong một luồng riêng."""
        self.callback = callback
        thread = threading.Thread(target=self._listen, daemon=True)
        thread.start()

    def _listen(self):
        """Vòng lặp đọc dữ liệu JSON từ phía C và đẩy vào hàm callback."""
        while self._is_running and self.process:
            line = self.process.stdout.readline()
            if not line:
                break
            
            try:
                data = json.loads(line.strip())
                if self.callback:
                    self.callback(data)
            except json.JSONDecodeError:
                if line.strip():
                    print(f"Hệ thống: Dữ liệu nhận được: {line.strip()}")
            except Exception as e:
                print(f"Lỗi khi xử lý dữ liệu từ C: {e}")

    def stop(self):
        """Dừng tiến trình C một cách an toàn."""
        self._is_running = False
        if self.process:
            print("Hệ thống: Đang đóng kết nối Backend...")
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
            print("Hệ thống: Đã dừng Backend Daemon.")
            
    # Thêm vào class BackendBridge trong gui/backend_bridge.py

    def logout(self):
        """Gửi yêu cầu đăng xuất (Mã 103)"""
        payload = {"type": 103}
        return self.send_command(payload)

    def update_item(self, room_id, item_index, title, start_price, buy_now):
        """Gửi yêu cầu cập nhật vật phẩm (Mã 304)"""
        payload = {
            "type": 304,
            "room_id": room_id,
            "item_index": item_index,
            "title": title,
            "start_price": int(start_price),
            "buy_now": int(buy_now)
        }
        return self.send_command(payload)