#include "server.h"
#include "state.h"

UserState *get_user_by_fd(int fd) {
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].fd == fd)
      return &users[i];
  }
  return NULL;
}

// Broadcast cho tat ca user trong room_id (tru sender)
void broadcast_to_room(int room_id, char *json_message) {
  printf("[Broadcast] Room %d: %s\n", room_id, json_message);

  for (int i = 0; i < MAX_USERS; i++) {
    // Chi gui cho user dang dang nhap trong phong do
    if (users[i].fd != 0 && users[i].is_logged_in &&
        users[i].current_room_id == room_id) {

      // ===== FIX USE-AFTER-FREE =====
      // Tao ban sao rieng cho moi user
      char *msg_copy = strdup(json_message);
      if (msg_copy) {
        send_message(users[i].fd, msg_copy);
        free(msg_copy);
      }
    }
  }
}

char *handle_login(int fd, cJSON *json) {
  char *user = (char *)get_json_string(json, "user");
  char *pass = (char *)get_json_string(json, "pass");

  if (!user || !pass) {
    return create_error_response(ERR_INVALID_MESSAGE, "Missing user or pass");
  }

  if (strcmp(pass, "123") == 0) {
    UserState *u = get_user_by_fd(fd);
    if (u) {
      strcpy(u->username, user);
      u->is_logged_in = 1;
      printf("User %s logged in on fd %d\n", user, fd);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_LOGIN_SUCCESS);
    cJSON_AddStringToObject(resp, "message", "Login success");
    char *str = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return str;
  } else {
    return create_error_response(ERR_WRONG_PASSWORD, "Wrong password");
  }
}

char *handle_bid(int fd, cJSON *json) {
  UserState *u = get_user_by_fd(fd);
  if (!u || !u->is_logged_in)
    return create_error_response(ERR_UNKNOWN, "Login required");

  if (u->current_room_id == -1)
    return create_error_response(ERR_UNKNOWN, "You are not in any room");

  int price = 0;
  get_json_int(json, "price", &price); // Client gửi lên giá muốn đặt

  // Tìm phòng user đang ở
  RoomState *room = NULL;
  for (int i = 0; i < MAX_ROOMS; i++) {
    if (rooms[i].room_id == u->current_room_id) {
      room = &rooms[i];
      break;
    }
  }

  if (!room)
    return create_error_response(ERR_UNKNOWN, "Room error");

  // LOGIC KIỂM TRA GIÁ: Phải cao hơn giá hiện tại ít nhất 1 bước giá (hoặc chỉ
  // cần > hiện tại)
  if (price <= room->current_price) {
    return create_error_response(ERR_BID_TOO_LOW,
                                 "Price must be higher than current price");
  }

  // CẬP NHẬT TRẠNG THÁI
  room->current_price = price;
  room->highest_bidder_id = u->fd; // Lưu người giữ giá cao nhất

  time_t now = time(NULL);
  room->end_time = now + 30;

  // BROADCAST: Báo tin vui cho cả làng
  cJSON *bc = cJSON_CreateObject();
  cJSON_AddNumberToObject(bc, "type", S2C_NEW_BID); // 904
  cJSON_AddNumberToObject(bc, "room_id", room->room_id);
  cJSON_AddNumberToObject(bc, "current_price", room->current_price);
  cJSON_AddStringToObject(bc, "bidder", u->username); // Gửi tên người vừa bid

  char *s_bc = cJSON_PrintUnformatted(bc);
  broadcast_to_room(room->room_id, s_bc);
  free(s_bc);
  cJSON_Delete(bc);

  // Trả về OK cho người vừa bid
  return create_ok_response();
}

// Hàm xử lý: TẠO PHÒNG
char *handle_create_room(int fd, cJSON *json) {
  UserState *u = get_user_by_fd(fd);
  if (!u || !u->is_logged_in)
    return create_error_response(ERR_UNKNOWN, "Login required");

  char *title = get_json_string(json, "title");
  int start_price = 0;
  get_json_int(json, "start_price", &start_price);

  if (!title || start_price <= 0)
    return create_error_response(ERR_INVALID_MESSAGE, "Invalid title or price");

  // Tìm slot phòng trống
  for (int i = 0; i < MAX_ROOMS; i++) {
    if (rooms[i].room_id == 0) { // Slot trống
      rooms[i].room_id = i + 1;  // ID bắt đầu từ 1
      strncpy(rooms[i].title, title, 99);
      rooms[i].current_price = start_price;
      rooms[i].highest_bidder_id = -1;
      rooms[i].is_active = 1;
      rooms[i].end_time = 0;
      // Set user hiện tại là chủ phòng (hoặc cho join luôn)
      u->current_room_id = rooms[i].room_id;

      // Trả về OK kèm room_id
      cJSON *resp = cJSON_CreateObject();
      cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
      cJSON_AddStringToObject(resp, "message", "Room created");
      cJSON_AddNumberToObject(resp, "room_id", rooms[i].room_id);
      char *s = cJSON_PrintUnformatted(resp);
      cJSON_Delete(resp);
      return s;
    }
  }
  return create_error_response(ERR_UNKNOWN, "Server full (max rooms reached)");
}

// Hàm xử lý: LIST PHÒNG
char *handle_list_rooms(int fd) {
  cJSON *resp = cJSON_CreateObject();
  cJSON_AddNumberToObject(resp, "type", S2C_ROOM_LIST);
  cJSON *arr = cJSON_CreateArray();

  for (int i = 0; i < MAX_ROOMS; i++) {
    if (rooms[i].room_id != 0 && rooms[i].is_active) {
      cJSON *item = cJSON_CreateObject();
      cJSON_AddNumberToObject(item, "id", rooms[i].room_id);
      cJSON_AddStringToObject(item, "title", rooms[i].title);
      cJSON_AddNumberToObject(item, "price", rooms[i].current_price);
      cJSON_AddItemToArray(arr, item);
    }
  }
  cJSON_AddItemToObject(resp, "rooms", arr);

  char *s = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);
  return s;
}

// Hàm xử lý: JOIN PHÒNG
char *handle_join_room(int fd, cJSON *json) {
  UserState *u = get_user_by_fd(fd);
  if (!u || !u->is_logged_in)
    return create_error_response(ERR_UNKNOWN, "Login required");

  int room_id = 0;
  get_json_int(json, "room_id", &room_id);

  // Tìm phòng
  int found = 0;
  for (int i = 0; i < MAX_ROOMS; i++) {
    if (rooms[i].room_id == room_id && rooms[i].is_active) {
      found = 1;
      u->current_room_id = room_id; // Gán user vào phòng này

      cJSON *notif = cJSON_CreateObject();
      cJSON_AddNumberToObject(notif, "type",
                              S2C_GENERIC_OK); // Hoặc loại tin riêng nếu muốn
      char msg[100];
      snprintf(msg, sizeof(msg),"User %s joined the room", u->username);
      cJSON_AddStringToObject(notif, "message", msg);

      char *s_notif = cJSON_PrintUnformatted(notif);
      broadcast_to_room(room_id, s_notif); // Gửi cho mọi người
      free(s_notif);
      cJSON_Delete(notif);

      break;
    }
  }

  if (!found)
    return create_error_response(ERR_ROOM_NOT_FOUND, "Room not found");

  cJSON *resp = cJSON_CreateObject();
  cJSON_AddNumberToObject(resp, "type", S2C_JOIN_ROOM_SUCCESS);
  cJSON_AddNumberToObject(resp, "room_id", room_id);
  char *s = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);
  return s;
}