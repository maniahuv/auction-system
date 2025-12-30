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
    int role = 1; // Mặc định là Bidder (1) nếu không gửi role
    get_json_int(json, "role", &role);

    if (!user || !pass) return create_error_response(ERR_INVALID_MESSAGE, "Missing user or pass");

    // Kiểm tra xem user đã tồn tại chưa
    FILE *f = fopen("accounts.txt", "r");
    char line[100], existing_user[50], existing_pass[50];
    int r;
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%[^:]:%[^:]:%d", existing_user, existing_pass, &r) >= 2) {
                if (strcmp(existing_user, user) == 0) {
                    fclose(f);
                    return create_error_response(ERR_USER_EXISTS, "Username already exists");
                }
            }
        }
        fclose(f);
    }

    // Ghi tài khoản mới vào file theo định dạng user:pass:role
    f = fopen("accounts.txt", "a");
    if (!f) return create_error_response(ERR_UNKNOWN, "Internal server error (file)");
    fprintf(f, "%s:%s:%d\n", user, pass, role);
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
    int role_val = 1;
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        // Đọc thêm role từ file accounts.txt
        if (sscanf(line, "%[^:]:%[^:]:%d", u, p, &role_val) >= 2) {
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
            u_ptr->role = role_val; // Gán quyền cho session
        }
        log_activity(user, "LOGGED IN");
        
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "type", S2C_LOGIN_SUCCESS);
        cJSON_AddNumberToObject(resp, "role", role_val);
        return cJSON_PrintUnformatted(resp);
    }
    return create_error_response(ERR_WRONG_PASSWORD, "Invalid username or password");
}

// Logic dat gia bid
char *handle_bid(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in)
        return create_error_response(ERR_UNKNOWN, "Login required");

    // KIỂM TRA QUYỀN: Chỉ Bidder mới được đặt giá
    if (u->role != ROLE_BIDDER)
        return create_error_response(ERR_UNKNOWN, "Only Bidders can place bids");

    if (u->current_room_id == -1)
        return create_error_response(ERR_UNKNOWN, "You are not in any room");

    int price = 0;
    get_json_int(json, "price", &price);

    RoomState *room = &rooms[u->current_room_id - 1];

    if (!room || !room->is_active)
        return create_error_response(ERR_UNKNOWN, "Auction is not active");

    // 1. KIỂM TRA BƯỚC GIÁ: Phải cao hơn ít nhất MIN_BID_STEP (10,000)
    if (price < (room->current_price + MIN_BID_STEP)) {
        char err_msg[100];
        snprintf(err_msg, sizeof(err_msg), "Gia phai cao hon gia hien tai it nhat %d VND", MIN_BID_STEP);
        return create_error_response(ERR_BID_TOO_LOW, err_msg);
    }

    // 2. CẬP NHẬT TRẠNG THÁI
    room->current_price = price;
    room->highest_bidder_id = u->fd; 

    // 3. LOGIC RESET THỜI GIAN: 
    // Nếu có giá thầu mới trong 30 giây cuối, đặt lại đồng hồ về 30 giây
    time_t now = time(NULL);
    double time_left = difftime(room->end_time, now);
    
    if (time_left < 30.0) {
        room->end_time = now + 30; // Reset về 30 giây
        room->sent_warning = 0;    // Reset cờ để hệ thống có thể gửi lại cảnh báo 30s sau đó
        printf("[Timer] Room %d: Timer reset to 30s due to new bid\n", room->room_id);
    }

    log_activity(u->username, "Placed a valid bid"); 

    // 4. BROADCAST cho cả phòng (Giữ nguyên logic cũ của bạn)
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

  // KIỂM TRA QUYỀN: Chỉ Auctioneer mới được tạo phòng
  if (u->role != ROLE_AUCTIONEER)
    return create_error_response(ERR_UNKNOWN, "Only Auctioneers can create rooms");

  // GIỚI HẠN: Mỗi Auctioneer chỉ được tạo 1 phòng đang hoạt động
  for (int j = 0; j < MAX_ROOMS; j++) {
      if (rooms[j].is_active && strcmp(rooms[j].owner_username, u->username) == 0) {
          return create_error_response(ERR_UNKNOWN, "You already have an active room");
      }
  }

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
      strncpy(rooms[i].owner_username, u->username, 49); // Lưu chủ phòng
      
      // --- LOGIC HÀNG CHỜ ---
      strncpy(rooms[i].queue[0].title, title, 99);
      rooms[i].queue[0].start_price = start_price;
      rooms[i].queue[0].buy_now_price = buy_now_price;
      
      rooms[i].total_items = 1;      
      rooms[i].current_item_idx = 0; 

      rooms[i].current_price = start_price;
      rooms[i].highest_bidder_id = -1;
      rooms[i].end_time = time(NULL) + 60; // Mặc định phiên đầu tiên 60s
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
            cJSON_AddStringToObject(item_room, "owner", rooms[i].owner_username);

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

  // KIỂM TRA QUYỀN: Bidder mới cần join phòng để đấu giá
  // Auctioneer và Admin có thể join để xem/quản lý

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

    // KIỂM TRA QUYỀN: Chỉ Bidder mới được mua ngay
    if (u->role != ROLE_BIDDER)
        return create_error_response(ERR_UNKNOWN, "Only Bidders can use Buy Now");

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

    // KIỂM TRA QUYỀN: Chỉ chủ phòng (Auctioneer) mới được thêm món vào hàng chờ
    if (u->role != ROLE_ADMIN && strcmp(r->owner_username, u->username) != 0) {
        return create_error_response(ERR_UNKNOWN, "You are not the owner of this room");
    }

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
    char *s_notif = cJSON_PrintUnformatted(notif);
    broadcast_to_room(r->room_id, s_notif);
    free(s_notif);
    cJSON_Delete(notif);

    return create_ok_response();
}

// Hàm xử lý: XÓA VẬT PHẨM TRONG HÀNG CHỜ
char *handle_delete_item(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in)
        return create_error_response(ERR_UNKNOWN, "Login required");

    int room_id = 0, item_idx = 0;
    get_json_int(json, "room_id", &room_id);
    get_json_int(json, "item_index", &item_idx);

    // Kiểm tra phòng có tồn tại không
    if (room_id <= 0 || room_id > MAX_ROOMS || !rooms[room_id - 1].is_active) {
        return create_error_response(ERR_ROOM_NOT_FOUND, "Room not found");
    }

    RoomState *r = &rooms[room_id - 1];

    // KIỂM TRA QUYỀN: Admin hoặc Chủ phòng mới có quyền xóa
    if (u->role != ROLE_ADMIN && strcmp(r->owner_username, u->username) != 0) {
        return create_error_response(ERR_UNKNOWN, "Unauthorized to delete items in this room");
    }

    int real_idx = item_idx - 1;

    // Kiểm tra tính hợp lệ: không được xóa món đang đấu giá hoặc đã đấu giá xong
    if (real_idx <= r->current_item_idx || real_idx >= r->total_items) {
        return create_error_response(ERR_UNKNOWN, "Cannot delete live, finished, or invalid item");
    }

    // Ghi log (Tăng buffer lên 256 để tránh warning)
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Deleted item '%s' from Room %d queue", 
             r->queue[real_idx].title, room_id);
    log_activity(u->username, log_msg);

    // Dịch chuyển mảng để xóa phần tử
    for (int i = real_idx; i < r->total_items - 1; i++) {
        r->queue[i] = r->queue[i + 1];
    }
    r->total_items--;

    // Broadcast thông báo cho mọi người trong phòng đó biết danh sách hàng chờ đã thay đổi
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "type", S2C_GENERIC_OK);
    cJSON_AddStringToObject(notif, "message", "An item was removed from the queue");
    char *s_notif = cJSON_PrintUnformatted(notif);
    broadcast_to_room(room_id, s_notif);
    free(s_notif);
    cJSON_Delete(notif);

    return create_ok_response();
}

// Hàm xem lịch sử đấu giá (Phân quyền Admin/Auctioneer/Bidder)
char *handle_get_history(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) return create_error_response(ERR_UNKNOWN, "Login required");

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_HISTORY_LIST);
    cJSON *h_list = cJSON_CreateArray();

    FILE *f = fopen("history.txt", "r");
    if (f) {
        char line[256], winner[50], item[100], owner[50];
        int price; long ts;
        while (fgets(line, sizeof(line), f)) {
            // Đọc định dạng mới: winner:item:price:timestamp:owner
            int res = sscanf(line, "%[^:]:%[^:]:%d:%ld:%s", winner, item, &price, &ts, owner);
            if (res >= 4) {
                int should_add = 0;
                // Admin: Thấy tất cả giao dịch
                if (u->role == ROLE_ADMIN) should_add = 1;
                // Bidder: Thấy món mình thắng
                else if (u->role == ROLE_BIDDER && strcmp(winner, u->username) == 0) should_add = 1;
                // Auctioneer: Thấy món mình đã bán (cần res == 5 để đảm bảo có field owner)
                else if (u->role == ROLE_AUCTIONEER && res == 5 && strcmp(owner, u->username) == 0) should_add = 1;

                if (should_add) {
                    cJSON *h_item = cJSON_CreateObject();
                    cJSON_AddStringToObject(h_item, "item", item);
                    cJSON_AddNumberToObject(h_item, "price", price);
                    cJSON_AddNumberToObject(h_item, "time", ts);
                    cJSON_AddStringToObject(h_item, "winner", winner); 
                    cJSON_AddItemToArray(h_list, h_item);
                }
            }
        }
        fclose(f);
    }
    cJSON_AddItemToObject(resp, "history", h_list);
    return cJSON_PrintUnformatted(resp);
}

char *handle_search_item(int fd, cJSON *json) {
    (void)fd;
    const char *keyword = get_json_string(json, "keyword");
    if (!keyword) return create_error_response(ERR_INVALID_MESSAGE, "Missing keyword");

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_SEARCH_RESULT);
    cJSON *results = cJSON_CreateArray();

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id != 0 && rooms[i].is_active) {
            for (int j = 0; j < rooms[i].total_items; j++) {
                if (strcasestr(rooms[i].queue[j].title, keyword)) {
                    cJSON *res_item = cJSON_CreateObject();
                    cJSON_AddNumberToObject(res_item, "room_id", rooms[i].room_id);
                    cJSON_AddStringToObject(res_item, "title", rooms[i].queue[j].title);
                    cJSON_AddNumberToObject(res_item, "current_price", 
                        (j == rooms[i].current_item_idx) ? rooms[i].current_price : rooms[i].queue[j].start_price);
                    
                    char *status = (j == rooms[i].current_item_idx) ? "DANG DAU GIA" : 
                                   (j < rooms[i].current_item_idx ? "Da xong" : "Dang cho");
                    cJSON_AddStringToObject(res_item, "status", status);
                    cJSON_AddItemToArray(results, res_item);
                }
            }
        }
    }
    cJSON_AddItemToObject(resp, "results", results);
    return cJSON_PrintUnformatted(resp);
}

// Hàm xử lý: RỜI PHÒNG
char *handle_leave_room(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in)
        return create_error_response(ERR_UNKNOWN, "Login required");

    if (u->current_room_id == -1)
        return create_error_response(ERR_UNKNOWN, "You are not in any room");

    int room_id_cu = u->current_room_id;
    u->current_room_id = -1; // Đặt lại trạng thái không ở trong phòng nào

    log_activity(u->username, "Left the room");

    // 1. Thông báo cho những người khác trong phòng (Broadcast)
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "type", S2C_GENERIC_OK);
    char msg[100];
    snprintf(msg, sizeof(msg), "User %s da roi khoi phong", u->username);
    cJSON_AddStringToObject(notif, "message", msg);

    char *s_notif = cJSON_PrintUnformatted(notif);
    broadcast_to_room(room_id_cu, s_notif);
    free(s_notif);
    cJSON_Delete(notif);

    // 2. Phản hồi thành công cho chính người vừa rời phòng
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
    cJSON_AddStringToObject(resp, "message", "Ban da roi phong thanh cong");
    return cJSON_PrintUnformatted(resp);
}

// Liệt kê vật phẩm đã thêm bởi user hiện tại
char *handle_list_my_items(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) return create_error_response(ERR_UNKNOWN, "Login required");

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK); // Hoặc tạo mã S2C_MY_ITEMS_LIST
    cJSON *arr = cJSON_CreateArray();

    // Duyệt qua tất cả các phòng để tìm vật phẩm user này đã thêm (nếu cần logic này)
    // Hoặc đơn giản là liệt kê trong phòng hiện tại
    if (u->current_room_id != -1) {
        RoomState *r = &rooms[u->current_room_id - 1];
        for (int j = 0; j < r->total_items; j++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "title", r->queue[j].title);
            cJSON_AddNumberToObject(item, "status", (j == r->current_item_idx) ? 1 : (j < r->current_item_idx ? 0 : 2));
            cJSON_AddItemToArray(arr, item);
        }
    }
    
    cJSON_AddItemToObject(resp, "my_items", arr);
    return cJSON_PrintUnformatted(resp);
}