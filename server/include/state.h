#ifndef STATE_H
#define STATE_H

#include "protocol.h"
#include <time.h>

#define MAX_USERS 50
#define MAX_ROOMS 10
#define MAX_ITEMS_PER_ROOM 10 // Số lượng vật phẩm tối đa trong hàng chờ của mỗi phòng

typedef struct {
  int fd;              // Socket file descriptor
  char username[50];
  int role;
  int is_logged_in;
  int current_room_id; // -1 nếu chưa vào phòng
} UserState;

// Cấu trúc cho một vật phẩm trong hàng chờ
typedef struct {
  char title[100];     // Tên vật phẩm
  int start_price;     // Giá khởi điểm
  int buy_now_price;   // Giá mua ngay
} AuctionItem;

typedef struct {
  int room_id;
  int is_active;
  char owner_username[50];
  
  // --- QUẢN LÝ HÀNG CHỜ ---
  AuctionItem queue[MAX_ITEMS_PER_ROOM]; // Danh sách các vật phẩm chờ đấu giá
  int total_items;      // Tổng số vật phẩm hiện có trong hàng chờ
  int current_item_idx; // Chỉ số của vật phẩm đang được đấu giá (0 -> total_items-1)
  
  // Trạng thái đấu giá hiện tại (áp dụng cho vật phẩm tại current_item_idx)
  int current_price;
  int highest_bidder_id; // ID (fd) của người trả giá cao nhất cho món hiện tại
  time_t end_time;
  int sent_warning;      // Cờ báo hiệu đã gửi cảnh báo 30 giây (0: chưa, 1: rồi)
} RoomState;

// Khai báo biến toàn cục (định nghĩa thực tế nằm trong main.c)
extern UserState users[MAX_USERS];
extern RoomState rooms[MAX_ROOMS];

#endif