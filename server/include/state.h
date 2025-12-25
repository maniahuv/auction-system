#ifndef STATE_H
#define STATE_H

#include "protocol.h"

#define MAX_USERS 50
#define MAX_ROOMS 10

typedef struct {
  int fd; // Socket file descriptor
  char username[50];
  int is_logged_in;
  int current_room_id; // -1 neu chua vo phong
} UserState;

typedef struct {
  int room_id;
  char title[100];
  int current_price;
  int highest_bidder_id; // ID (fd) cua nguoi tra gia cao nhat
  int is_active;
  time_t end_time;
} RoomState;

// Khai bao bien toan cuc (dinh nghia trong main.c)
extern UserState users[MAX_USERS];
extern RoomState rooms[MAX_ROOMS];

#endif