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

        # 1. Kiểm tra sự tồn tại của file thực thi C
        if not os.path.exists(self.binary_path):
            print(f"CRITICAL ERROR: Không tìm thấy file thực thi tại '{self.binary_path}'")
            print("Vui lòng chạy 'make' trong thư mục clientd trước khi khởi chạy GUI.")
            sys.exit(1)

        try:
            # 2. Khởi chạy tiến trình C (clientd)
            # stdin=PIPE: Để Python gửi JSON sang C
            # stdout=PIPE: Để Python nhận JSON từ C
            # stderr=PIPE: Để bắt các lỗi runtime của C (như mất kết nối server)
            self.process = subprocess.Popen(
                [self.binary_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1  # Line buffered để nhận dữ liệu ngay khi C in ra
            )
            self._is_running = True
            
            # 3. Khởi chạy luồng đọc lỗi (stderr) từ C để debug
            threading.Thread(target=self._read_stderr, daemon=True).start()
            
        except Exception as e:
            print(f"LỖI khi khởi chạy Backend Daemon: {e}")
            sys.exit(1)

    def _read_stderr(self):
        """Đọc và in các lỗi từ phía C ra terminal của Python."""
        while self._is_running and self.process:
            line = self.process.stderr.readline()
            if not line:
                break
            print(f"[C-Daemon Log]: {line.strip()}")

    def send_command(self, cmd_dict):
        """
        Gửi một dictionary Python dưới dạng JSON sang phía C qua stdin.
        """
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
        """
        Bắt đầu lắng nghe dữ liệu từ stdout của C trong một luồng riêng.
        """
        self.callback = callback
        thread = threading.Thread(target=self._listen, daemon=True)
        thread.start()

    def _listen(self):
        """
        Vòng lặp đọc dữ liệu JSON từ phía C và đẩy vào hàm callback.
        """
        while self._is_running and self.process:
            line = self.process.stdout.readline()
            if not line:
                print("Hệ thống: Luồng đọc kết thúc (Stdout closed).")
                break
            
            try:
                # Chuyển chuỗi JSON nhận được từ C thành Dictionary
                data = json.loads(line.strip())
                if self.callback:
                    # Gửi data về cho Main App xử lý
                    self.callback(data)
            except json.JSONDecodeError:
                # Bỏ qua nếu dòng nhận được không phải là JSON hợp lệ
                if line.strip():
                    print(f"Hệ thống: Nhận dữ liệu không phải JSON: {line.strip()}")
            except Exception as e:
                print(f"Lỗi khi xử lý dữ liệu từ C: {e}")

    def stop(self):
        """
        Dừng tiến trình C một cách an toàn.
        """
        self._is_running = False
        if self.process:
            print("Hệ thống: Đang đóng kết nối Backend...")
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
            print("Hệ thống: Đã dừng Backend Daemon.")