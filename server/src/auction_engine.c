#include "auction_engine.h"
#include "user_manager.h"
#include "db_manager.h"
#include "logger.h"
#include "json_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// Ham kiem tra va cap nhat trang thai cac phien dau gia
void check_auctions() {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_ROOMS; i++) {
        RoomState *r = &rooms[i];
        if (r->is_active && r->end_time > 0) {
            double diff = difftime(r->end_time, now);
            
            // Canh bao 30s
            if (diff <= 30.0 && diff > 0 && r->sent_warning == 0) {
                r->sent_warning = 1;
                cJSON *alert = cJSON_CreateObject();
                cJSON_AddNumberToObject(alert, "type", S2C_TIME_ALERT);
                cJSON_AddNumberToObject(alert, "room_id", r->room_id);
                cJSON_AddStringToObject(alert, "message", "CON 30 GIAY!");
                cJSON_AddNumberToObject(alert, "time_left", (int)diff);
                char *s = cJSON_PrintUnformatted(alert);
                broadcast_to_room(r->room_id, s);
                free(s); cJSON_Delete(alert);
            }

            // Ket thuc phien
            if (diff <= 0) {
                char winner[50] = "Khong co";
                UserState *w = get_user_by_fd(r->highest_bidder_id);
                if (w) strcpy(winner, w->username);

                if (r->highest_bidder_id != -1) {
                    db_save_auction_result(winner, r->queue[r->current_item_idx].title, r->current_price, r->owner_username);
                }

                cJSON *msg = cJSON_CreateObject();
                cJSON_AddNumberToObject(msg, "type", S2C_AUCTION_ENDED);
                cJSON_AddNumberToObject(msg, "room_id", r->room_id);
                cJSON_AddStringToObject(msg, "item_title", r->queue[r->current_item_idx].title);
                cJSON_AddStringToObject(msg, "winner", winner);
                cJSON_AddNumberToObject(msg, "final_price", r->current_price);
                char *s = cJSON_PrintUnformatted(msg);
                broadcast_to_room(r->room_id, s);
                free(s); cJSON_Delete(msg);

                // Chuyen mon
                if (r->current_item_idx + 1 < r->total_items) {
                    r->current_item_idx++;
                    r->current_price = r->queue[r->current_item_idx].start_price;
                    r->highest_bidder_id = -1;
                    r->end_time = now + 60; // Reset 60 giay cho vat pham tiep theo
                    r->sent_warning = 0;

                    // PHÁT TIN 902 ĐỂ CLIENT MỞ LẠI NÚT BẤM VÀ CẬP NHẬT THÔNG TIN MÓN MỚI
                    cJSON *next = cJSON_CreateObject();
                    cJSON_AddNumberToObject(next, "type", S2C_NEW_ITEM_PENDING);
                    cJSON_AddStringToObject(next, "title", r->queue[r->current_item_idx].title);
                    cJSON_AddNumberToObject(next, "start_price", r->current_price);
                    char *s_next = cJSON_PrintUnformatted(next);
                    broadcast_to_room(r->room_id, s_next);
                    free(s_next); cJSON_Delete(next);

                    // Cập nhật lại hàng chờ cho cả phòng để món vừa đấu biến mất khỏi danh sách chờ trên UI
                    extern void broadcast_queue_update(int room_id);
                    broadcast_queue_update(r->room_id);
                } else {
                    r->is_active = 0; r->room_id = 0;
                }
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

    if (price < (r->current_price + MIN_BID_STEP)) return create_error_response(ERR_BID_TOO_LOW, "Bid too low");

    r->current_price = price;
    r->highest_bidder_id = fd;

    // Reset timer neu con < 30s (Gia han them thoi gian dau gia)
    time_t now = time(NULL);
    if (difftime(r->end_time, now) < 30.0) {
        r->end_time = now + 30;
        r->sent_warning = 0;
    }

    log_activity(u->username, "Placed a bid");
    cJSON *bc = cJSON_CreateObject();
    cJSON_AddNumberToObject(bc, "type", S2C_NEW_BID);
    cJSON_AddStringToObject(bc, "bidder", u->username);
    cJSON_AddNumberToObject(bc, "current_price", price);
    char *s_bc = cJSON_PrintUnformatted(bc);
    broadcast_to_room(r->room_id, s_bc);
    free(s_bc); cJSON_Delete(bc);

    // PHẢN HỒI RIÊNG cho người đặt giá: Gửi thông báo cụ thể để tránh GUI tự động quay về Dashboard
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
    
    // 1. Kiem tra trang thai dang nhap va trong phong
    if (!u || u->current_room_id == -1) 
        return create_error_response(ERR_UNKNOWN, "Not in room");

    // 2. Kiem tra quyen han
    if (u->role != ROLE_BIDDER) 
        return create_error_response(ERR_UNKNOWN, "Only Bidders can use Buy Now");

    RoomState *r = &rooms[u->current_room_id - 1];
    int bn_price = r->queue[r->current_item_idx].buy_now_price;

    // 3. Kiem tra tinh hop le cua Buy Now
    if (bn_price <= 0) 
        return create_error_response(ERR_UNKNOWN, "Buy Now disabled for this item");

    // Thuc hien mua ngay
    r->current_price = bn_price;
    r->highest_bidder_id = fd;
    r->end_time = time(NULL); // Ket thuc phien ngay lap tuc, check_auctions se xu ly tiep theo
    
    log_activity(u->username, "Used Buy Now option");
    return create_ok_response();
}