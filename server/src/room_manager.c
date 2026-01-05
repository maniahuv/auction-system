#include "room_manager.h"
#include "user_manager.h"
#include "logger.h"
#include "db_manager.h"
#include "framing.h"
#include "json_util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Truy cập mảng người dùng toàn cục
extern UserState users[];

// Helper để gửi cập nhật hàng chờ cho cả phòng khi có thay đổi (Thêm/Xóa vật phẩm)
void broadcast_queue_update(int room_id) {
    RoomState *r = &rooms[room_id - 1];
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", 907); // S2C_QUEUE_UPDATE
    cJSON *q_arr = cJSON_CreateArray();
    
    // Gửi danh sách các vật phẩm từ vị trí hiện tại trở đi
    for (int j = r->current_item_idx; j < r->total_items; j++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "title", r->queue[j].title);
        cJSON_AddNumberToObject(obj, "start_price", r->queue[j].start_price);
        cJSON_AddItemToArray(q_arr, obj);
    }
    
    cJSON_AddItemToObject(resp, "queue", q_arr);
    char *s = cJSON_PrintUnformatted(resp);
    broadcast_to_room(room_id, s);
    free(s); 
    cJSON_Delete(resp);
}

// Xu ly cac logic lien quan phong dau gia
char *handle_create_room(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) return create_error_response(ERR_UNKNOWN, "Yeu cau dang nhap!");
    if (u->role != ROLE_AUCTIONEER) return create_error_response(ERR_UNKNOWN, "Chi nguoi dau gia moi co the tao phong!");

    for (int j = 0; j < MAX_ROOMS; j++) {
        if (rooms[j].is_active && strcmp(rooms[j].owner_username, u->username) == 0) {
            return create_error_response(ERR_UNKNOWN, "Ban da co mot phong dang hoat dong!");
        }
    }

    const char *title = get_json_string(json, "title");
    int start_price = 0, buy_now = 0;
    get_json_int(json, "start_price", &start_price);
    get_json_int(json, "buy_now", &buy_now);

    if (!title || start_price <= 0) return create_error_response(ERR_INVALID_MESSAGE, "Thong tin vat pham khong hop le!");

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == 0) {
            rooms[i].room_id = i + 1;
            rooms[i].is_active = 1;
            rooms[i].is_started = 0; // KHOI TAO: Chua bat dau dau gia
            strncpy(rooms[i].owner_username, u->username, 49);
            strncpy(rooms[i].queue[0].title, title, 99);
            rooms[i].queue[0].start_price = start_price;
            rooms[i].queue[0].buy_now_price = buy_now;
            rooms[i].total_items = 1;
            rooms[i].current_item_idx = 0;
            rooms[i].current_price = start_price;
            rooms[i].highest_bidder_id = -1;
            rooms[i].end_time = 0; // KHOI TAO: Dong ho chua chay
            rooms[i].sent_warning = 0;
            u->current_room_id = rooms[i].room_id;

            // ĐỒNG BỘ PHÒNG MỚI TẠO VÀO DATABASE
            db_update_room_state(rooms[i].room_id, rooms[i].current_item_idx, rooms[i].current_price, rooms[i].highest_bidder_id, (long)rooms[i].end_time);

            log_activity(u->username, "Da tao phong dau gia moi");
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
            cJSON_AddStringToObject(resp, "message", "Tao phong thanh cong!");
            cJSON_AddNumberToObject(resp, "room_id", rooms[i].room_id);
            char *s = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return s;
        }
    }
    return create_error_response(ERR_UNKNOWN, "May chu da day phong!");
}

// Ham bat dau phien dau gia (Chi chu phong)
char *handle_start_auction(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (!u || u->current_room_id == -1) return create_error_response(ERR_UNKNOWN, "Ban phai o trong phong!");
    
    RoomState *r = &rooms[u->current_room_id - 1];
    if (strcmp(r->owner_username, u->username) != 0) 
        return create_error_response(ERR_UNKNOWN, "Chi chu phong moi co quyen bat dau!");
    
    if (r->is_started) 
        return create_error_response(ERR_UNKNOWN, "Phien dau gia nay da bat dau tu truoc!");

    r->is_started = 1;
    r->end_time = time(NULL) + 60; // Bat dau 60 giay cho vat pham dau tien
    r->sent_warning = 0;

    // Dong bo vao Database
    db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);

    // Thong bao cho ca phong biet phien da bat dau (Mã 903) để Client mở khóa nút Bid
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "type", S2C_AUCTION_STARTED);
    cJSON_AddStringToObject(notif, "message", "Phien dau gia chinh thuc BAT DAU!");
    cJSON_AddNumberToObject(notif, "time_left", 60);
    cJSON_AddNumberToObject(notif, "is_started", 1);
    char *s_notif = cJSON_PrintUnformatted(notif);
    broadcast_to_room(r->room_id, s_notif);
    free(s_notif);
    cJSON_Delete(notif);

    log_activity(u->username, "Da bat dau phien dau gia");
    return create_ok_response();
}

// Xu ly nguoi dung tham gia phong dau gia
char *handle_join_room(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) {
        return create_error_response(ERR_UNKNOWN, "Yeu cau dang nhap!");
    }

    int room_id = 0;
    get_json_int(json, "room_id", &room_id);
    
    if (room_id <= 0 || room_id > MAX_ROOMS || !rooms[room_id - 1].is_active) {
        return create_error_response(ERR_ROOM_NOT_FOUND, "Khong tim thay phong dau gia!");
    }

    u->current_room_id = room_id;
    RoomState *r = &rooms[room_id - 1];
    log_activity(u->username, "Tham gia vao phong");

    // 1. Thong bao cho NHUNG NGUOI KHAC trong phong
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "type", S2C_GENERIC_OK);
    char msg[100];
    snprintf(msg, sizeof(msg), "Nguoi dung %s da tham gia phong", u->username);
    cJSON_AddStringToObject(notif, "message", msg);
    char *s_notif = cJSON_PrintUnformatted(notif);
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd != -1 && users[i].fd != fd && users[i].current_room_id == room_id) {
            send_message(users[i].fd, s_notif);
        }
    }
    free(s_notif);
    cJSON_Delete(notif);

    // 2. Phan hoi cho nguoi join kem thong tin vat pham va hang cho
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_JOIN_ROOM_SUCCESS);
    cJSON_AddNumberToObject(resp, "room_id", room_id);
    cJSON_AddNumberToObject(resp, "is_started", r->is_started); // Gui kem trang thai bat dau
    
    if (r->current_item_idx < r->total_items) {
        cJSON_AddStringToObject(resp, "item_title", r->queue[r->current_item_idx].title);
        cJSON_AddNumberToObject(resp, "current_price", r->current_price);
        cJSON_AddNumberToObject(resp, "buy_now", r->queue[r->current_item_idx].buy_now_price);
        
        // Neu chua bat dau, gui time_left = 0
        int time_left = 0;
        if (r->is_started && r->end_time > 0) {
            time_left = (int)difftime(r->end_time, time(NULL));
        }
        cJSON_AddNumberToObject(resp, "time_left", time_left > 0 ? time_left : 0);

        cJSON *q_arr = cJSON_CreateArray();
        for (int j = r->current_item_idx; j < r->total_items; j++) {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "title", r->queue[j].title);
            cJSON_AddNumberToObject(obj, "start_price", r->queue[j].start_price);
            cJSON_AddItemToArray(q_arr, obj);
        }
        cJSON_AddItemToObject(resp, "queue", q_arr);
    }

    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

// Xu ly nguoi dung roi phong dau gia
char *handle_leave_room(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in || u->current_room_id == -1)
        return create_error_response(ERR_UNKNOWN, "Ban dang khong o trong phong nao!");

    int old_room = u->current_room_id;
    u->current_room_id = -1;
    log_activity(u->username, "Da roi khoi phong");

    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "type", S2C_GENERIC_OK);
    char msg[100];
    snprintf(msg, sizeof(msg), "Nguoi dung %s da roi phong", u->username);
    cJSON_AddStringToObject(notif, "message", msg);
    char *s_notif = cJSON_PrintUnformatted(notif);
    broadcast_to_room(old_room, s_notif);
    free(s_notif);
    cJSON_Delete(notif);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
    cJSON_AddStringToObject(resp, "message", "LEAVE_SUCCESS");
    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

// Xu ly danh sach phong dau gia
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
            cJSON_AddNumberToObject(item_room, "is_started", rooms[i].is_started);
            cJSON *q_arr = cJSON_CreateArray();
            for (int j = 0; j < rooms[i].total_items; j++) {
                cJSON *obj = cJSON_CreateObject();
                cJSON_AddStringToObject(obj, "title", rooms[i].queue[j].title);
                cJSON_AddNumberToObject(obj, "start_price", rooms[i].queue[j].start_price);
                cJSON_AddNumberToObject(obj, "buy_now", rooms[i].queue[j].buy_now_price);
                cJSON_AddItemToArray(q_arr, obj);
            }
            cJSON_AddItemToObject(item_room, "queue", q_arr);
            cJSON_AddItemToArray(arr, item_room);
        }
    }
    cJSON_AddItemToObject(resp, "rooms", arr);
    char *s = cJSON_PrintUnformatted(resp); 
    cJSON_Delete(resp);
    return s;
}

// Xu ly them vat pham vao hang cho dau gia
char *handle_add_item(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    int room_id = 0;
    get_json_int(json, "room_id", &room_id);
    if (room_id <= 0 || room_id > MAX_ROOMS || !rooms[room_id - 1].is_active)
        return create_error_response(ERR_ROOM_NOT_FOUND, "Phong khong ton tai!");

    RoomState *r = &rooms[room_id - 1];
    
    // CHAN: Neu phien dau gia da bat dau thi khong duoc them mon
    if (r->is_started) 
        return create_error_response(ERR_UNKNOWN, "Khong the them vat pham khi phien dau gia da bat dau!");

    if (u->role != ROLE_ADMIN && strcmp(r->owner_username, u->username) != 0)
        return create_error_response(ERR_UNKNOWN, "Ban khong phai chu phong!");

    if (r->total_items >= MAX_ITEMS_PER_ROOM) return create_error_response(ERR_UNKNOWN, "Hang cho da day!");

    const char *title = get_json_string(json, "title");
    int sp = 0, bn = 0;
    get_json_int(json, "start_price", &sp);
    get_json_int(json, "buy_now", &bn);

    strncpy(r->queue[r->total_items].title, title, 99);
    r->queue[r->total_items].start_price = sp;
    r->queue[r->total_items].buy_now_price = bn;
    r->total_items++;

    // ĐỒNG BỘ HÀNG CHỜ MỚI VÀO DATABASE
    db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);

    log_activity(u->username, "Da them vat pham moi vao hang cho");
    broadcast_queue_update(r->room_id);
    
    return create_ok_response();
}

// Xu ly xoa vat pham khoi hang cho dau gia
char *handle_delete_item(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    int rid = 0, idx = 0;
    get_json_int(json, "room_id", &rid);
    get_json_int(json, "item_index", &idx);
    RoomState *r = &rooms[rid - 1];

    // CHAN: Neu phien dau gia da bat dau thi khong duoc xoa mon
    if (r->is_started) 
        return create_error_response(ERR_UNKNOWN, "Khong the xoa vat pham khi phien dau gia da bat dau!");

    if (u->role != ROLE_ADMIN && strcmp(r->owner_username, u->username) != 0)
        return create_error_response(ERR_UNKNOWN, "Khong co quyen truy cap!");

    int real_idx = idx - 1;
    if (real_idx <= r->current_item_idx || real_idx >= r->total_items)
        return create_error_response(ERR_UNKNOWN, "Vat pham khong hop le hoac dang duoc dau gia!");

    for (int i = real_idx; i < r->total_items - 1; i++) r->queue[i] = r->queue[i+1];
    r->total_items--;

    // ĐỒNG BỘ HÀNG CHỜ SAU KHI XÓA VÀO DATABASE
    db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);

    log_activity(u->username, "Da xoa vat pham khoi hang cho");
    broadcast_queue_update(r->room_id);
    
    return create_ok_response();
}

// Xu ly tim kiem vat pham
char *handle_search_item(int fd, cJSON *json) {
    (void)fd;
    const char *kw = get_json_string(json, "keyword");
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_SEARCH_RESULT);
    cJSON *results = cJSON_CreateArray();
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active) {
            for (int j = 0; j < rooms[i].total_items; j++) {
                if (strcasestr(rooms[i].queue[j].title, kw)) {
                    cJSON *res_item = cJSON_CreateObject();
                    cJSON_AddNumberToObject(res_item, "room_id", rooms[i].room_id);
                    cJSON_AddStringToObject(res_item, "title", rooms[i].queue[j].title);
                    cJSON_AddNumberToObject(res_item, "start_price", rooms[i].queue[j].start_price);
                    cJSON_AddStringToObject(res_item, "status", (j == rooms[i].current_item_idx) ? "DANG DAU GIA" : (j < rooms[i].current_item_idx ? "Da xong" : "Dang cho"));
                    cJSON_AddItemToArray(results, res_item);
                }
            }
        }
    }
    cJSON_AddItemToObject(resp, "results", results);
    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

// Xu ly lich su dau gia
char *handle_get_history(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) return create_error_response(ERR_UNKNOWN, "Yeu cau dang nhap!");
    return db_get_history_json(u->username, u->role);
}