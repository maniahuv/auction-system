#include "auction_engine.h"
#include "user_manager.h"
#include "db_manager.h"
#include "logger.h"
#include "json_util.h"
#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Hàm gửi số lượng người trong phòng (Mã 909)
void broadcast_user_count(int room_id) {
    int count = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        // Kiểm tra user đang online và đang ở đúng room_id này
        if (users[i].is_logged_in && users[i].current_room_id == room_id) {
            count++;
        }
    }
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg, "type", 909);
    cJSON_AddNumberToObject(msg, "count", count);
    char *out = cJSON_PrintUnformatted(msg);
    broadcast_to_room(room_id, out);
    free(out);
    cJSON_Delete(msg);
}

// Ham kiem tra va cap nhat trang thai cac phien dau gia
void check_auctions() {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_ROOMS; i++) {
        RoomState *r = &rooms[i];
        if (!r->is_active || !r->is_started || r->end_time <= 0) continue;

        double diff = difftime(r->end_time, now);
        
        // 1. Gửi cảnh báo khi còn 30 giây
        if (diff <= 30.0 && diff > 0 && r->sent_warning == 0) {
            r->sent_warning = 1;
            cJSON *alert = cJSON_CreateObject();
            cJSON_AddNumberToObject(alert, "type", S2C_TIME_ALERT);
            cJSON_AddNumberToObject(alert, "room_id", r->room_id);
            cJSON_AddStringToObject(alert, "message", "CON 30 GIAY!");
            cJSON_AddNumberToObject(alert, "time_left", (int)diff);
            char *s = cJSON_PrintUnformatted(alert);
            broadcast_to_room(r->room_id, s);
            free(s); 
            cJSON_Delete(alert);
        }

        // 2. Kết thúc phiên khi hết thời gian
        if (diff <= 0) {
            char winner[50] = "Khong co";
            UserState *w = get_user_by_fd(r->highest_bidder_id);
            if (w) strcpy(winner, w->username);

            // Lưu kết quả vào DB
            if (r->highest_bidder_id != -1) {
                db_save_auction_result(winner, r->queue[r->current_item_idx].title, r->current_price, r->owner_username);
            }

            // Thông báo kết thúc món hàng
            cJSON *msg = cJSON_CreateObject();
            cJSON_AddNumberToObject(msg, "type", S2C_AUCTION_ENDED);
            cJSON_AddNumberToObject(msg, "room_id", r->room_id);
            cJSON_AddStringToObject(msg, "item_title", r->queue[r->current_item_idx].title);
            cJSON_AddStringToObject(msg, "winner", winner);
            cJSON_AddNumberToObject(msg, "final_price", r->current_price);
            char *s = cJSON_PrintUnformatted(msg);
            broadcast_to_room(r->room_id, s);
            free(s); 
            cJSON_Delete(msg);

            // KIỂM TRA CHUYỂN MÓN TIẾP THEO
            if (r->current_item_idx + 1 < r->total_items) {
                r->current_item_idx++;
                // SỬA LỖI: Gán giá hiện tại bằng giá khởi điểm món mới thay vì 0
                r->current_price = r->queue[r->current_item_idx].start_price;
                r->highest_bidder_id = -1;
                r->end_time = now + 60; // 60 giây cho món tiếp theo
                r->sent_warning = 0;    // Reset cờ cảnh báo
                r->is_started = 1;

                db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);

                cJSON *next = cJSON_CreateObject();
                cJSON_AddNumberToObject(next, "type", S2C_NEW_ITEM_PENDING);
                cJSON_AddStringToObject(next, "title", r->queue[r->current_item_idx].title);
                // Đảm bảo gửi giá hiện tại (giá khởi điểm món mới) về Client để không bị hiển thị 0
                cJSON_AddNumberToObject(next, "current_price", r->current_price); 
                cJSON_AddNumberToObject(next, "buy_now", r->queue[r->current_item_idx].buy_now_price); 
                cJSON_AddNumberToObject(next, "is_started", 1); 
                cJSON_AddNumberToObject(next, "time_left", 60);
                
                char *s_next = cJSON_PrintUnformatted(next);
                broadcast_to_room(r->room_id, s_next);
                free(s_next); 
                cJSON_Delete(next);

                extern void broadcast_queue_update(int room_id);
                broadcast_queue_update(r->room_id);
                broadcast_user_count(r->room_id); // Cập nhật số người xem cho món mới
            } else {
                // TRƯỜNG HỢP KẾT THÚC VẬT PHẨM CUỐI CÙNG
                r->is_active = 0; 
                r->is_started = 0;
                db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);
                
                // Gửi cập nhật hàng chờ rỗng để Client xóa vật phẩm cuối cùng trên UI (Mã 907)
                cJSON *empty_q = cJSON_CreateObject();
                cJSON_AddNumberToObject(empty_q, "type", 907); 
                cJSON_AddArrayToObject(empty_q, "queue");
                char *s_empty = cJSON_PrintUnformatted(empty_q);
                broadcast_to_room(r->room_id, s_empty);
                free(s_empty); 
                cJSON_Delete(empty_q);

                r->room_id = 0; // Giải phóng slot phòng
            }
        }
    }
}

// Ham xu ly yeu cau dat gia
char *handle_bid(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || u->current_room_id == -1) return create_error_response(ERR_UNKNOWN, "Not in room");
    if (u->role != ROLE_BIDDER) return create_error_response(ERR_UNKNOWN, "Bidders only");

    int price = 0;
    get_json_int(json, "price", &price);
    RoomState *r = &rooms[u->current_room_id - 1];

    if (!r->is_started) return create_error_response(ERR_UNKNOWN, "Phien dau gia chua bat dau!");

    if (price < (r->current_price + MIN_BID_STEP)) return create_error_response(ERR_BID_TOO_LOW, "Gia dat phai lon hon gia hien tai it nhat 10k");

    r->current_price = price;
    r->highest_bidder_id = fd;

    time_t now = time(NULL);
    if (difftime(r->end_time, now) < 30.0) {
        r->end_time = now + 30;
        r->sent_warning = 0;
    }

    db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);

    log_activity(u->username, "Placed a bid");
    cJSON *bc = cJSON_CreateObject();
    cJSON_AddNumberToObject(bc, "type", S2C_NEW_BID);
    cJSON_AddStringToObject(bc, "bidder", u->username);
    cJSON_AddNumberToObject(bc, "current_price", price);
    // BỔ SUNG: Gửi kèm time_left để Client đồng bộ lại đồng hồ khi reset về 30s
    cJSON_AddNumberToObject(bc, "time_left", (int)difftime(r->end_time, now));

    char *s_bc = cJSON_PrintUnformatted(bc);
    broadcast_to_room(r->room_id, s_bc);
    free(s_bc); cJSON_Delete(bc);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
    cJSON_AddStringToObject(resp, "message", "BID_SUCCESS");
    char *s_resp = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s_resp;
}

// Ham xu ly yeu cau mua ngay
char *handle_buy_now(int fd, cJSON *json) {
    (void)json;
    UserState *u = get_user_by_fd(fd);
    if (!u || u->current_room_id == -1) return create_error_response(ERR_UNKNOWN, "Not in room");
    if (u->role != ROLE_BIDDER) return create_error_response(ERR_UNKNOWN, "Only Bidders can use Buy Now");

    RoomState *r = &rooms[u->current_room_id - 1];
    if (!r->is_started) return create_error_response(ERR_UNKNOWN, "Phien dau gia chua bat dau!");

    int bn_price = r->queue[r->current_item_idx].buy_now_price;
    if (bn_price <= 0) return create_error_response(ERR_UNKNOWN, "Buy Now disabled for this item");

    r->current_price = bn_price;
    r->highest_bidder_id = fd;
    r->end_time = time(NULL); // Kết thúc ngay món hàng

    db_update_room_state(r->room_id, r->current_item_idx, r->current_price, r->highest_bidder_id, (long)r->end_time);
    
    log_activity(u->username, "Used Buy Now option");
    return create_ok_response();
}