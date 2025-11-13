import socket
import json
import struct
import sys

# --- Cấu hình ---
HOST = 'localhost'
PORT = 8080  # Phải khớp với SERVER_PORT trong server.h

# Tin nhắn payload chúng ta muốn gửi (dựa trên login.json)
payload_dict = {
    "type": 102,
    "user": "test_user",
    "pass": "12345"
}

def run_test():
    try:
        # 1. Chuẩn bị payload
        # Chuyển từ dict Python -> chuỗi JSON -> bytes (dùng utf-8)
        payload_str = json.dumps(payload_dict)
        payload_bytes = payload_str.encode('utf-8')

        # 2. ĐÓNG GÓI (Framing)
        # Tính độ dài (len)
        msg_len = len(payload_bytes)
        # '!' nghĩa là network order (Big-Endian)
        # 'I' nghĩa là 4-byte unsigned integer
        # Đây chính là hàm "htonl()" và 4-byte length-prefix
        packed_len = struct.pack('!I', msg_len)

        # Gói tin hoàn chỉnh = [ 4-byte length ][ payload ]
        message_to_send = packed_len + payload_bytes

        print(f"--- Client ---")
        print(f"Đã chuẩn bị {msg_len} byte payload: {payload_str}")
        print(f"Đã đóng gói 4-byte length: {packed_len.hex()}")
        print(f"Tổng kích thước gửi: {len(message_to_send)} bytes")

        # 3. Kết nối và Gửi
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            
            # Gửi toàn bộ gói tin (tương đương send_all)
            s.sendall(message_to_send)
            print(f"Đã gửi tin nhắn đến {HOST}:{PORT}")

            # 4. Nhận phản hồi (MỞ GÓI - Unframing)
            # Bước 4a: Nhận 4-byte length-prefix
            packed_len_recv = s.recv(4)
            if not packed_len_recv:
                print("Server đã đóng kết nối (nhận 0 byte).")
                return

            # Giải nén 4-byte length
            msg_len_recv = struct.unpack('!I', packed_len_recv)[0]
            print(f"\nĐã nhận 4-byte length: {packed_len_recv.hex()} (Giá trị = {msg_len_recv} bytes)")

            # Bước 4b: Nhận chính xác số byte của payload
            payload_bytes_recv = s.recv(msg_len_recv)
            
            # Chuyển bytes -> chuỗi JSON
            payload_str_recv = payload_bytes_recv.decode('utf-8')

            print(f"Đã nhận {len(payload_bytes_recv)} byte payload:")
            
            # In đẹp JSON
            try:
                print(json.dumps(json.loads(payload_str_recv), indent=2))
            except json.JSONDecodeError:
                print(f"Lỗi: Không thể decode JSON: {payload_str_recv}")

    except ConnectionRefusedError:
        print(f"Lỗi: Không thể kết nối đến {HOST}:{PORT}.")
        print("Bạn đã chạy server (./../bin/server) chưa?")
    except Exception as e:
        print(f"Đã xảy ra lỗi: {e}")

if __name__ == "__main__":
    run_test()