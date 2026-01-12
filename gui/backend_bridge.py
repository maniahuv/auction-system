import subprocess
import threading
import json
import os
import sys
import time

class AuctionBackend:
    def __init__(self, binary_path="../bin/clientd"):
        """
        Khởi tạo cầu nối giữa Python và Client Daemon viết bằng C.
        """
        self.binary_path = binary_path
        self.process = None
        self.callback = None
        self._is_running = True  # Cờ tổng kiểm soát ứng dụng
        self.reconnecting = False # Cờ kiểm tra trạng thái reconnect

        # Tự động tải cấu hình mạng
        self.host, self.port = self._load_config()

        # 1. Kiểm tra sự tồn tại của file thực thi C
        if not os.path.exists(self.binary_path):
            print(f"CRITICAL ERROR: Không tìm thấy file thực thi tại '{self.binary_path}'")
            print("Vui lòng chạy 'make' trong thư mục clientd trước khi khởi chạy GUI.")
            sys.exit(1)

        # 2. Bắt đầu chạy tiến trình lần đầu (Thay vì viết trực tiếp, ta gọi hàm start_process)
        self.start_process()

        # 3. Khởi chạy luồng Heartbeat (Chạy độc lập, tự kiểm tra process)
        threading.Thread(target=self._heartbeat_loop, daemon=True).start()

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

    def start_process(self):
            """Hàm khởi tạo tiến trình C mới (Được dùng cho cả Init và Reconnect)."""
            try:
                self.process = subprocess.Popen(
                    [self.binary_path, self.host, str(self.port)],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    bufsize=1
                )

                # --- ĐOẠN FIX QUAN TRỌNG ---
                # Chờ 1 giây để xem tiến trình C có bị chết ngay (do không kết nối được) không
                time.sleep(1.5)
                if self.process.poll() is not None:
                    # Nếu process đã dừng, nghĩa là kết nối thất bại
                    raise Exception("Kết nối thất bại (Process exited)")
                # ---------------------------

                print(f"Hệ thống: Đã kết nối tới {self.host}:{self.port}")

                # Khởi động lại các luồng đọc dữ liệu cho tiến trình mới
                threading.Thread(target=self._listen, daemon=True).start()
                threading.Thread(target=self._read_stderr, daemon=True).start()

                # Nếu đang ở chế độ reconnect, báo cho UI biết đã thành công
                if self.reconnecting and self.callback:
                    self.callback({"type": 999, "message": "RECONNECTED"})

                self.reconnecting = False

            except Exception as e:
                print(f"[RETRY] Lỗi khởi chạy ({e}). Thử lại sau 3s...")
                # Gọi lại attempt_reconnect để tiếp tục vòng lặp thử kết nối
                threading.Thread(target=self._attempt_reconnect, daemon=True).start()

    def _read_stderr(self):
        """Đọc và in các lỗi từ phía C ra terminal của Python."""
        current_proc = self.process
        while self._is_running and current_proc == self.process:
            try:
                line = current_proc.stderr.readline()
                if not line: break
                print(f"[C-Daemon Log]: {line.strip()}")
            except: break

    def send_command(self, cmd_dict):
        """Gửi một dictionary Python dưới dạng JSON sang phía C qua stdin."""
        if self.reconnecting or not self.process:
            return False
        try:
            if self.process.poll() is not None: return False
            json_str = json.dumps(cmd_dict)
            self.process.stdin.write(json_str + "\n")
            self.process.stdin.flush()
            return True
        except (BrokenPipeError, IOError):
            # Lỗi gửi tin cũng kích hoạt reconnect
            if not self.reconnecting:
                self._attempt_reconnect()
            return False

    def start_listening(self, callback):
        """Bắt đầu lắng nghe dữ liệu từ stdout của C."""
        self.callback = callback
        # _listen đã được gọi trong start_process, không cần gọi lại ở đây
        # nhưng để tương thích code cũ, ta có thể bỏ qua hoặc check process
        if not self.process:
             pass

    def _listen(self):
        """Đọc dữ liệu từ C. Nếu luồng này kết thúc => Process đã chết."""
        current_proc = self.process
        while self._is_running:
            try:
                line = current_proc.stdout.readline()
                if not line: break # EOF: Process chết
                try:
                    data = json.loads(line.strip())
                    if self.callback: self.callback(data)
                except: pass
            except: break

        # Khi vòng lặp vỡ (do lỗi hoặc EOF), kích hoạt Reconnect
        if self._is_running and not self.reconnecting:
            # Chỉ reconnect nếu đây là process hiện tại (tránh xung đột luồng cũ)
            if current_proc == self.process:
                self._attempt_reconnect()

    def _attempt_reconnect(self):
            """Logic thử kết nối lại sau khi mất mạng."""
            if not self._is_running: return

            # Chỉ set flag và báo UI 1 lần đầu tiên khi mới phát hiện mất mạng
            if not self.reconnecting:
                self.reconnecting = True
                print("Mất kết nối! Đang thử kết nối lại...")
                if self.callback:
                    try: self.callback({"type": 998, "message": "LOST_CONNECTION"})
                    except: pass

            # Luôn luôn sleep và thử lại (bất kể flag là gì, vì hàm này được gọi từ except của start_process)
            time.sleep(3)
            self.start_process()

    def _heartbeat_loop(self):
        """Tự động gửi Ping lên Server mỗi 10 giây để giữ kết nối."""
        while self._is_running:
            time.sleep(10) # Ngủ 10 giây
            if not self.reconnecting and self.process and self.process.poll() is None:
                try:
                    self.send_command({"type": 701})
                except: pass

    def stop(self):
        """Dừng tiến trình C an toàn."""
        self._is_running = False
        if self.process:
            self.process.terminate()
            print("Hệ thống: Đã dừng Backend Daemon.")

    # --- CÁC HÀM GỬI LỆNH (GIỮ NGUYÊN) ---

    def register(self, u, p, r): return self.send_command({"type": 101, "user": u, "pass": p, "role": r})
    def login(self, u, p): return self.send_command({"type": 102, "user": u, "pass": p})
    def logout(self): return self.send_command({"type": 103})
    def list_rooms(self): return self.send_command({"type": 201})
    def create_room(self, n, t, s, b): return self.send_command({"type": 202, "room_name": n, "title": t, "start_price": int(s), "buy_now": int(b)})
    def join_room(self, rid): return self.send_command({"type": 203, "room_id": int(rid)})
    def leave_room(self): return self.send_command({"type": 204})
    def search_item(self, k): return self.send_command({"type": 205, "keyword": k})
    def start_auction(self): return self.send_command({"type": 206})
    def send_chat(self, m): return self.send_command({"type": 207, "message": m})
    def add_item(self, rid, t, s, b): return self.send_command({"type": 301, "room_id": int(rid), "title": t, "start_price": int(s), "buy_now": int(b)})
    def delete_item(self, rid, idx): return self.send_command({"type": 302, "room_id": int(rid), "item_index": int(idx)})
    def update_item(self, rid, idx, t, s, b): return self.send_command({"type": 304, "room_id": int(rid), "item_index": int(idx), "title": t, "start_price": int(s), "buy_now": int(b)})
    def bid(self, p): return self.send_command({"type": 401, "price": int(p)})
    def buy_now(self): return self.send_command({"type": 402})
    def get_history(self): return self.send_command({"type": 501})
    def admin_list_users(self): return self.send_command({"type": 601})
    def admin_delete_user(self, uid): return self.send_command({"type": 602, "user_id": int(uid)})
    def admin_update_role(self, uid, r): return self.send_command({"type": 603, "user_id": int(uid), "new_role": int(r)})
