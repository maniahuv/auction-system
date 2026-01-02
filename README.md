# 🛒 Online Auction System (Hybrid C/Python Architecture)

Hệ thống đấu giá trực tuyến hiệu năng cao, kết hợp sức mạnh xử lý mạng của **C** và giao diện đồ họa linh hoạt của **Python**. Dự án sử dụng mô hình **Client Daemon**, trong đó một tiến trình C chạy ngầm xử lý giao thức mạng và giao tiếp với giao diện Python thông qua đường ống dữ liệu (Pipes).

## 🌟 Tính năng nổi bật

### 1. Hệ thống cốt lõi (C Server)

* **Sequential Auction Queue:** Quản lý hàng chờ vật phẩm tự động chuyển phiên.
* **Framing Mechanism:** Cơ chế Length-prefix (4-byte header) chống dính/xẻ gói TCP.
* **I/O Multiplexing:** Sử dụng `select()` để quản lý hàng trăm kết nối đồng thời.

### 2. Giao diện người dùng (Python GUI)

* **Real-time Dashboard:** Danh sách phòng đấu giá cập nhật trực tiếp không cần tải lại trang.
* **Live Auction Room:** Hiển thị đồng hồ đếm ngược thời gian thực, đồng bộ tuyệt đối với Server.
* **Role-based UI:** Giao diện tự động thay đổi dựa trên vai trò (Bidder/Auctioneer/Admin).

---

## 🛠 Kiến trúc kỹ thuật (Hybrid Model)

Hệ thống hoạt động dựa trên 3 thành phần chính:

1. **Backend Server (C):** Quản lý logic đấu giá, Database và Broadcast thông tin.
2. **Client Daemon (C):** Một tiến trình chạy ngầm làm cầu nối, xử lý đóng gói JSON và giao thức mạng TCP.
3. **Frontend GUI (Python/Tkinter):** Hiển thị giao diện người dùng và gửi lệnh JSON tới Daemon qua `stdin/stdout`.

---

## 📂 Cấu trúc dự án

```text
.
├── bin/                # File thực thi (server, clientd, auction_cli)
├── common/             # Thư viện dùng chung (Giao thức, Framing)
├── server/             # Mã nguồn Server (C)
├── clientd/            # Mã nguồn Client Daemon (C) làm cầu nối cho GUI
├── gui/                # Giao diện đồ họa (Python)
│   ├── main.py         # File chạy chính
│   ├── backend_bridge.py # Logic kết nối Python - C
│   └── screens/        # Các màn hình (Login, Dashboard, Room)
└── third_party/        # Thư viện cJSON

```

---

## 🚀 Hướng dẫn cài đặt & Chạy

### 1. Yêu cầu hệ thống

* **Hệ điều hành:** Linux (Ubuntu/Debian được khuyến nghị).
* **Compiler:** GCC.
* **Python:** Phiên bản 3.x và thư viện Tkinter.

Cài đặt Tkinter trên Ubuntu:

```bash
sudo apt update
sudo apt install python3-tk

```

### 2. Biên dịch hệ thống (C)

Sử dụng Makefile để biên dịch toàn bộ các thành phần lõi:

```bash
# Tại thư mục gốc của dự án
make clean
make

```

*Sau khi chạy, các file thực thi `server`, `clientd`, và `auction_cli` sẽ xuất hiện trong thư mục `bin/`.*

### 3. Vận hành hệ thống

**Bước 1: Chạy Server**

```bash
cd bin
./server

```

**Bước 2: Chạy Giao diện GUI (Python)**

```bash
cd gui
python3 main.py

```

*(Lưu ý: Bạn cũng có thể chạy bản **Client CLI** truyền thống bằng lệnh `./bin/auction_cli` để thực hiện test nhanh qua terminal).*

---

## ⌨️ Hướng dẫn lệnh & Giao thức

### Các mã hiệu Protocol (protocol.h)

Hệ thống sử dụng các mã định danh để giao tiếp giữa các thành phần:

| Mã (Type) | Ý nghĩa | Hướng đi |
| --- | --- | --- |
| **102** | Yêu cầu Đăng nhập | Client -> Server |
| **802** | Đăng nhập Thành công | Server -> Client |
| **201** | Lấy danh sách phòng | Client -> Server |
| **905** | Cảnh báo thời gian (Time Alert) | Server -> Broadcast |
| **401** | Đặt giá thầu (Bid) | Client -> Server |

### Ví dụ dữ liệu JSON (Đặt giá thầu)

Khi người dùng bấm nút "Bid" trên GUI, Python gửi tới Client Daemon:

```json
{ "type": 401, "price": 1000000 }

```

Daemon sẽ thêm Header độ dài và gửi gói tin thô qua Socket tới Server.

---

## 📝 Nhật ký thay đổi (Roadmap)

* [x] Refactor mã nguồn C sang dạng mô đun hóa (State, Utils, Parser).
* [x] Phát triển cơ chế Client Daemon để tách biệt logic mạng và UI.
* [x] Hoàn thiện GUI bằng Python/Tkinter (Dashboard, Room, Timer).
* [ ] Tích hợp cơ sở dữ liệu SQL (SQLite/MySQL) thay cho file text.
* [ ] Hỗ trợ mã hóa SSL/TLS cho kết nối Socket.

---

**Author:** [Tên của bạn]
**Project:** Hệ thống Đấu giá Trực tuyến - Đồ án lập trình mạng.

---
