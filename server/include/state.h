#ifndef STATE_H
#define STATE_H

#include "protocol.h"
#include <time.h>

#define MAX_USERS 50
#define MAX_ROOMS 10
#define MAX_ITEMS_PER_ROOM 10 // So luong vat pham toi da auctioner co the tao trong 1 phong

typedef struct {
  int fd;              // Socket file descriptor
  char username[50];
  int role;
  int is_logged_in;
  int current_room_id; // -1 neu chua vao phong
  time_t last_activity;
} UserState;

// Cau truc luu thong tin vat pham trong hang gio
typedef struct {
  char title[100];     // Ten vat pham
  int start_price;     // Gia khoi diem
  int buy_now_price;   // Gia mua ngay
} AuctionItem;

typedef struct {
  int room_id;
  int is_active;
  int is_started;
  char owner_username[50];
  
  // --- QUAN LY HANG CHO ---
  AuctionItem queue[MAX_ITEMS_PER_ROOM]; // Danh sach cac vat pham dang cho dau gia
  int total_items;      // Tong so vat pham hien co
  int current_item_idx; // Chi so vat pham dang duoc dau gia (0 -> total_items-1)
  
  // Trang thai dau gia hien tai (ap dung cho cac vat pham tai current_item_idx)
  int current_price;
  int highest_bidder_id; // ID (fd) cua nguoi tra gia cao nhat hien tai
  time_t end_time;
  int sent_warning;      // Co bao hieu gui canh bao 30s (0: chua, 1: roi)
} RoomState;

// Khai bao bien toan cuc
extern UserState users[MAX_USERS];
extern RoomState rooms[MAX_ROOMS];

#endif