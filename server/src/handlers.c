#include "server.h"
#include "state.h"


// Tính năng ghi log hoạt động của user
void log_activity(const char *username, const char *action) {
    FILE *f = fopen("auction_server.log", "a");
    if (f == NULL) return;

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Xóa ký tự xuống dòng

    fprintf(f, "[%s] User: %-10s | Action: %s\n", timestamp, username ? username : "UNKNOWN", action);
    fclose(f);
}

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

// Hàm xử lý ĐĂNG KÝ
char *handle_register(int fd, cJSON *json) {
    (void)fd; // Unused parameter
    const char *user = get_json_string(json, "user");
    const char *pass = get_json_string(json, "pass");

    if (!user || !pass) return create_error_response(ERR_INVALID_MESSAGE, "Missing user or pass");

    // Kiểm tra xem user đã tồn tại chưa
    FILE *f = fopen("accounts.txt", "r");
    char line[100], existing_user[50], existing_pass[50];
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%[^:]:%s", existing_user, existing_pass) == 2) {
                if (strcmp(existing_user, user) == 0) {
                    fclose(f);
                    return create_error_response(ERR_USER_EXISTS, "Username already exists");
                }
            }
        }
        fclose(f);
    }

    // Ghi tài khoản mới vào file
    f = fopen("accounts.txt", "a");
    if (!f) return create_error_response(ERR_UNKNOWN, "Internal server error (file)");
    fprintf(f, "%s:%s\n", user, pass);
    fclose(f);

    log_activity(user, "REGISTERED new account");
    return create_ok_response();
}

char *handle_login(int fd, cJSON *json) {
    const char *user = get_json_string(json, "user");
    const char *pass = get_json_string(json, "pass");

    FILE *f = fopen("accounts.txt", "r");
    if (!f) return create_error_response(ERR_USER_NOT_FOUND, "No accounts found. Please register first.");

    char line[100], u[50], p[50];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%[^:]:%s", u, p) == 2) {
            if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
                found = 1; break;
            }
        }
    }
    fclose(f);

    if (found) {
        UserState *u_ptr = get_user_by_fd(fd);
        if (u_ptr) {
            strcpy(u_ptr->username, user);
            u_ptr->is_logged_in = 1;
        }
        log_activity(user, "LOGGED IN");
        
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "type", S2C_LOGIN_SUCCESS);
        return cJSON_PrintUnformatted(resp);
    }
    return create_error_response(ERR_WRONG_PASSWORD, "Invalid username or password");
}

char *handle_bid(int fd, cJSON *json) {
  UserState *u = get_user_by_fd(fd);
  if (!u || !u->is_logged_in)
    return create_error_response(ERR_UNKNOWN, "Login required");

  if (u->current_room_id == -1)
    return create_error_response(ERR_UNKNOWN, "You are not in any room");

  int price = 0;
  get_json_int(json, "price", &price); // Client gửi lên giá muốn đặt

  // Tìm phòng user đang ở (ID phòng là index + 1)
  RoomState *room = &rooms[u->current_room_id - 1];

  if (!room || !room->is_active)
    return create_error_response(ERR_UNKNOWN, "Auction is not active");

  // LOGIC KIỂM TRA GIÁ: Phải cao hơn giá hiện tại
  if (price <= room->current_price) {
    return create_error_response(ERR_BID_TOO_LOW,
                                 "Price must be higher than current price");
  }

  // CẬP NHẬT TRẠNG THÁI
  room->current_price = price;
  room->highest_bidder_id = u->fd; 

  time_t now = time(NULL);
  room->end_time = now + 30;
  room->sent_warning = 0; // Reset cờ cảnh báo cho lượt bid mới

  log_activity(u->username, "Placed a bid"); 

  // BROADCAST cho cả phòng
  cJSON *bc = cJSON_CreateObject();
  cJSON_AddNumberToObject(bc, "type", S2C_NEW_BID); 
  cJSON_AddNumberToObject(bc, "room_id", room->room_id);
  cJSON_AddNumberToObject(bc, "current_price", room->current_price);
  cJSON_AddStringToObject(bc, "bidder", u->username);

  char *s_bc = cJSON_PrintUnformatted(bc);
  broadcast_to_room(room->room_id, s_bc);
  free(s_bc);
  cJSON_Delete(bc);

  return create_ok_response();
}

// Hàm xử lý: TẠO PHÒNG
char *handle_create_room(int fd, cJSON *json) {
  UserState *u = get_user_by_fd(fd);
  if (!u || !u->is_logged_in)
    return create_error_response(ERR_UNKNOWN, "Login required");

  const char *title = get_json_string(json, "title");
  int start_price = 0;
  get_json_int(json, "start_price", &start_price);
  int buy_now_price = 0;
  get_json_int(json, "buy_now", &buy_now_price);

  if (!title || start_price <= 0)
    return create_error_response(ERR_INVALID_MESSAGE, "Invalid title or price");

  for (int i = 0; i < MAX_ROOMS; i++) {
    if (rooms[i].room_id == 0) { 
      rooms[i].room_id = i + 1;
      rooms[i].is_active = 1;    
      
      // --- LOGIC HÀNG CHỜ ---
      strncpy(rooms[i].queue[0].title, title, 99);
      rooms[i].queue[0].start_price = start_price;
      rooms[i].queue[0].buy_now_price = buy_now_price;
      
      rooms[i].total_items = 1;      
      rooms[i].current_item_idx = 0; 

      rooms[i].current_price = start_price;
      rooms[i].highest_bidder_id = -1;
      rooms[i].end_time = 0; 
      rooms[i].sent_warning = 0; 

      u->current_room_id = rooms[i].room_id;
      log_activity(u->username, "Created a room with queue management"); 

      cJSON *resp = cJSON_CreateObject();
      cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
      cJSON_AddStringToObject(resp, "message", "Room created successfully");
      cJSON_AddNumberToObject(resp, "room_id", rooms[i].room_id);
      
      char *s = cJSON_PrintUnformatted(resp);
      cJSON_Delete(resp);
      return s;
    }
  }
  return create_error_response(ERR_UNKNOWN, "Server full (max rooms reached)");
}

char *handle_list_rooms(int fd) {
    (void)fd; 
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_ROOM_LIST);
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id != 0 && rooms[i].is_active) {
            cJSON *item_room = cJSON_CreateObject();
            cJSON_AddNumberToObject(item_room, "id", rooms[i].room_id);
            cJSON_AddNumberToObject(item_room, "current_price", rooms[i].current_price);
            cJSON_AddNumberToObject(item_room, "current_idx", rooms[i].current_item_idx);

            // Thêm danh sách vật phẩm trong hàng chờ của phòng này
            cJSON *queue_arr = cJSON_CreateArray();
            for (int j = 0; j < rooms[i].total_items; j++) {
                cJSON *obj = cJSON_CreateObject();
                cJSON_AddStringToObject(obj, "title", rooms[i].queue[j].title);
                cJSON_AddNumberToObject(obj, "start_price", rooms[i].queue[j].start_price);
                cJSON_AddNumberToObject(obj, "buy_now", rooms[i].queue[j].buy_now_price);
                cJSON_AddItemToArray(queue_arr, obj);
            }
            cJSON_AddItemToObject(item_room, "queue", queue_arr);
            cJSON_AddItemToArray(arr, item_room);
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

  if (room_id <= 0 || room_id > MAX_ROOMS || !rooms[room_id - 1].is_active)
    return create_error_response(ERR_ROOM_NOT_FOUND, "Room not found or inactive");

  u->current_room_id = room_id;
  log_activity(u->username, "Joined a room"); 

  cJSON *notif = cJSON_CreateObject();
  cJSON_AddNumberToObject(notif, "type", S2C_GENERIC_OK);
  char msg[100];
  snprintf(msg, sizeof(msg),"User %s joined the room", u->username);
  cJSON_AddStringToObject(notif, "message", msg);

  char *s_notif = cJSON_PrintUnformatted(notif);
  broadcast_to_room(room_id, s_notif); 
  free(s_notif);
  cJSON_Delete(notif);

  cJSON *resp = cJSON_CreateObject();
  cJSON_AddNumberToObject(resp, "type", S2C_JOIN_ROOM_SUCCESS);
  cJSON_AddNumberToObject(resp, "room_id", room_id);
  char *s = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);
  return s;
}

// Hàm xử lý: MUA NGAY (Buy Now)
char *handle_buy_now(int fd, cJSON *json) {
    (void)json; 
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in || u->current_room_id == -1)
        return create_error_response(ERR_UNKNOWN, "Action not allowed");

    RoomState *room = &rooms[u->current_room_id - 1];

    // SỬA LỖI: Lấy buy_now_price từ vật phẩm hiện tại trong hàng chờ
    int current_buy_now = room->queue[room->current_item_idx].buy_now_price;

    if (!room->is_active || current_buy_now <= 0)
        return create_error_response(ERR_UNKNOWN, "Buy Now option not available");

    // Thực hiện kết thúc đấu giá ngay lập tức
    room->current_price = current_buy_now;
    room->highest_bidder_id = u->fd;
    room->end_time = time(NULL); 

    log_activity(u->username, "Used BUY NOW"); 

    return create_ok_response();
}

// Hàm xử lý: THÊM VẬT PHẨM VÀO PHÒNG CHỈ ĐỊNH
char *handle_add_item(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in)
        return create_error_response(ERR_UNKNOWN, "Login required");

    int room_id = 0;
    get_json_int(json, "room_id", &room_id); // Lấy room_id từ client

    if (room_id <= 0 || room_id > MAX_ROOMS || !rooms[room_id - 1].is_active)
        return create_error_response(ERR_ROOM_NOT_FOUND, "Room not found or inactive");

    RoomState *r = &rooms[room_id - 1];
    if (r->total_items >= MAX_ITEMS_PER_ROOM)
        return create_error_response(ERR_UNKNOWN, "Queue full for this room");

    const char *title = get_json_string(json, "title");
    int start_p = 0, buy_n = 0;
    get_json_int(json, "start_price", &start_p);
    get_json_int(json, "buy_now", &buy_n);

    if (!title || start_p <= 0)
        return create_error_response(ERR_INVALID_MESSAGE, "Invalid item info");

    // Thêm vào cuối hàng chờ của phòng r
    int idx = r->total_items;
    strncpy(r->queue[idx].title, title, 99);
    r->queue[idx].start_price = start_p;
    r->queue[idx].buy_now_price = buy_n;
    r->total_items++;

    log_activity(u->username, "Added item to a room queue");
    
    // Thông báo cho mọi người đang ở trong phòng đó
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "type", S2C_GENERIC_OK);
    cJSON_AddStringToObject(notif, "message", "A new item was added to the queue");
    broadcast_to_room(r->room_id, cJSON_PrintUnformatted(notif));

    return create_ok_response();
}