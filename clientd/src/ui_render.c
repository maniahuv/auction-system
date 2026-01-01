#include "../include/ui_render.h"
#include "../include/client_state.h"
#include "protocol.h"
#include "json_util.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void print_server_response(char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        printf("RAW: %s\n", json_str);
        return;
    }

    int type = 0;
    get_json_int(json, "type", &type);

    switch (type) {
        case S2C_GENERIC_OK: // Xử lý type 800
            printf("\n[SUCCESS] %s\n", get_json_string(json, "message"));
            break;

        case S2C_LOGIN_SUCCESS: {
            current_role = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "role"));
            printf("\n[SUCCESS] Dang nhap thanh cong với vai tro: %s!\n", 
                (current_role == ROLE_ADMIN) ? "ADMIN" : 
                (current_role == ROLE_AUCTIONEER) ? "AUCTIONEER" : "BIDDER");
            
            // In hướng dẫn lệnh ngay sau khi đăng nhập
            printf("Cac lenh hop le cho ban:\n");
            if (current_role == ROLE_ADMIN) {
                printf(" -> delete <room_id> <item_idx>, list, join <id>, leave, search <key>, history\n");
            } else if (current_role == ROLE_AUCTIONEER) {
                printf(" -> create <title> <p> <buy>, additem <room> <title> <p> <buy>, delete, list, join, leave, search, history\n");
            } else {
                printf(" -> list, join <id>, leave, bid <price>, buynow, search <key>, history\n");
            }
            break;
        }

        case S2C_ROOM_LIST:
            printf("\n======= DANH SACH PHONG & HANG CHO =======\n");
            cJSON *rooms_json = cJSON_GetObjectItem(json, "rooms");
            cJSON *r;
            cJSON_ArrayForEach(r, rooms_json) {
                int id = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "id"));
                printf("\n[PHONG #%d] owner: %s | Gia hien tai: %d\n", id, 
                       get_json_string(r, "owner"),
                       (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "current_price")));
                
                cJSON *queue = cJSON_GetObjectItem(r, "queue");
                cJSON *it;
                int count = 1;
                cJSON_ArrayForEach(it, queue) {
                    printf("  %d. %s | Gia BD: %d\n", count++, 
                           get_json_string(it, "title"),
                           (int)cJSON_GetNumberValue(cJSON_GetObjectItem(it, "start_price")));
                }
            }
            printf("\n===========================================\n");
            break;

        case S2C_HISTORY_LIST: { // Xử lý type 812
            printf("\n--- LICH SU DAU GIA ---\n");
            cJSON *h_arr = cJSON_GetObjectItem(json, "history");
            cJSON *h;
            cJSON_ArrayForEach(h, h_arr) {
                time_t t = (time_t)cJSON_GetNumberValue(cJSON_GetObjectItem(h, "time"));
                char *time_s = ctime(&t); time_s[strlen(time_s)-1] = '\0';
                printf("- [%s] Vat pham: %-15s | Gia: %-8d | Thang: %s\n", 
                    time_s,
                    get_json_string(h, "item"),
                    (int)cJSON_GetNumberValue(cJSON_GetObjectItem(h, "price")),
                    get_json_string(h, "winner"));
            }
            break;
        }

        case S2C_JOIN_ROOM_SUCCESS:
            current_room_id = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "room_id"));
            printf("\n[SUCCESS] Ban da vao phong #%d thanh cong!\n", current_room_id);
            break;

        case S2C_AUCTION_ENDED:
            printf("\n>>> [KET THUC] Phien dau gia vat pham '%s' da ket thuc!\n", get_json_string(json, "item_title"));
            printf("    Nguoi thang: %s | Gia cuoi: %d\n", 
                   get_json_string(json, "winner"), 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "final_price")));
            break;

        case S2C_NEW_BID:
            printf("\n>>> [BID] %s vua dat gia: %d\n", 
                   get_json_string(json, "bidder"), 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "current_price")));
            break;

        case S2C_TIME_ALERT:
            printf("\n>>> [THONG BAO] Phien dau gia con %d giay nua!\n", 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "time_left")));
            break;

        case S2C_GENERIC_ERROR:
            printf("\n[ERROR] %s\n", get_json_string(json, "message"));
            break;

        default:
            printf("\n[SERVER] %s\n", json_str);
    }
    cJSON_Delete(json);
    printf("YOU> "); 
    fflush(stdout);
}