# 🛒 Online Auction System (C/C++ Network Programming)

Hệ thống đấu giá trực tuyến được xây dựng bằng ngôn ngữ **C**, sử dụng giao thức **TCP/IP Socket**, cơ chế **I/O Multiplexing (select)** và định dạng dữ liệu **JSON**. Dự án giải quyết các vấn đề cốt lõi trong lập trình mạng như chống dính gói (Sticky Packets) và quản lý trạng thái phiên (Session Management).

## 🌟 Tính năng nổi bật

### 1. Core Logic (Hệ thống cốt lõi)

* **Sequential Auction Queue:** Quản lý hàng chờ vật phẩm trong mỗi phòng. Vật phẩm được đấu giá lần lượt, tự động chuyển món khi phiên kết thúc.
* **Framing Mechanism:** Sử dụng cơ chế Length-prefix (4-byte header) để đảm bảo toàn vẹn dữ liệu, chống dính gói/xẻ gói TCP.
* **Real-time Alerts:** Tự động gửi cảnh báo 30 giây cuối cùng cho tất cả người dùng trong phòng.
* **Data Persistence:** Lưu trữ tài khoản người dùng (`accounts.txt`) và lịch sử thắng cuộc (`history.txt`) bền vững.

### 2. Chức năng người dùng

* **Xác thực:** Đăng ký, Đăng nhập bảo mật.
* **Quản lý phòng:** Tạo phòng đấu giá mới, Tham gia/Rời phòng.
* **Quản lý hàng chờ:** Thêm vật phẩm vào hàng chờ của bất kỳ phòng nào, Xóa vật phẩm đang chờ (chưa lên sàn).
* **Đấu giá:** Đặt thầu (Bid) với cơ chế kiểm tra bước giá, Mua ngay (Buy Now).
* **Tiện ích:** Tìm kiếm vật phẩm theo từ khóa trên toàn hệ thống, Xem lịch sử cá nhân.

---

## 🛠 Kiến trúc kỹ thuật

* **Ngôn ngữ:** C (GCC Compiler).
* **Thư viện bên thứ ba:** [cJSON](https://github.com/DaveGamble/cJSON) (Xử lý dữ liệu JSON).
* **Cơ chế I/O:** `select()` Multiplexing (Hỗ trợ nhiều kết nối đồng thời).
* **Giao thức:** TCP/IP (Stream Socket).
* **Hệ điều hành:** Linux (Ubuntu/Debian).

---

## 📂 Cấu trúc dự án

```text
.
├── bin/                # File thực thi sau khi biên dịch (server, auction_cli)
├── build/              # File object (.o) tạm thời
├── server/
│   ├── include/        # File tiêu đề (.h)
│   └── src/            # Mã nguồn server (.c)
├── clientd/
│   ├── include/        # File tiêu đề (.h)
│   └── src/            # Mã nguồn client (.c)
├── third_party/        # Thư viện cJSON
├── Makefile            # File cấu hình biên dịch tự động
└── auction_server.log  # Nhật ký hoạt động hệ thống

```

---

## 🚀 Hướng dẫn cài đặt & Chạy

### 1. Biên dịch common

```bash
cd common
make clean
make

```

### 2. Chạy Server

```bash
cd ./server
make clean
make
cd ..

cd bin
./server

```

### 3. Chạy Client

Mở một terminal mới:

```bash
cd ./clientd
make clean
make
cd ..

cd bin
./auction_cli 127.0.0.1

```

---

## ⌨️ Hướng dẫn lệnh (CLI Guide)

Sau khi khởi chạy Client, bạn có thể sử dụng các lệnh sau:

| Nhóm | Lệnh | Mô tả |
| --- | --- | --- |
| **Auth** | `register <u] <p>` | Đăng ký tài khoản mới |
|  | `login <u] <p>` | Đăng nhập hệ thống |
| **Room** | `list` | Xem tất cả các phòng và hàng chờ vật phẩm |
|  | `create <title> <price> <buy_now>` | Tạo phòng mới và vật phẩm đầu tiên |
|  | `join <room_id>` | Tham gia vào một phòng đấu giá |
|  | `leave` | Rời khỏi phòng hiện tại |
| **Queue** | `additem <room_id> <title> <price> <buy_now>` | Thêm vật phẩm vào hàng chờ |
|  | `delete <room_id> <item_index>` | Xóa vật phẩm đang chờ khỏi phòng |
| **Auction** | `bid <price>` | Đặt giá thầu mới |
|  | `buynow` | Mua ngay vật phẩm hiện tại |
| **Utility** | `search <keyword>` | Tìm kiếm vật phẩm theo tên |
|  | `history` | Xem lịch sử các món đồ bạn đã thắng |

---

## 📊 Giao thức dữ liệu (Example JSON)

**Client gửi yêu cầu Đặt giá (Bid):**

```json
{
  "type": 401,
  "price": 500000
}

```

**Server Broadcast thông báo có giá mới:**

```json
{
  "type": 904,
  "room_id": 1,
  "current_price": 500000,
  "bidder": "userA"
}

```

---

## 📝 Nhật ký thay đổi (Roadmap)

* [x] Hoàn thiện xử lý Queue vật phẩm.
* [x] Cài đặt cơ chế tự động chuyển phiên đấu giá.
* [x] Hoàn thiện Search và Win History.
* [ ] Phát triển Giao diện đồ họa (GUI) bằng Python/Tkinter.
* [ ] Tích hợp hệ thống ví điện tử (Wallet).

---

**Author:** 

**Project:** 

---


