#include "handlers.h"
#include "auth.h"
#include "auction_engine.h"
#include "room_manager.h"
#include "json_util.h"
#include "admin_handler.h"
#include "stdio.h"
char *handle_ping() {
    printf("[Heartbeat] Received PING from client.\n");
    return create_ok_response(); // Hoặc trả về message "PONG" tùy ý
}

// Xu ly tin nhan nhan tu client
char *handle_client_message(int fd, cJSON *json) {
    int type = 0;
    get_json_int(json, "type", &type);

    switch (type) {
        // 1. Nhom xác thực tài khoản
        case C2S_REGISTER:    return handle_register(fd, json);
        case C2S_LOGIN:       return handle_login(fd, json);
        case C2S_LOGOUT:      return handle_logout(fd);

        // 2. Nhom quản lý phòng đấu giá
        case C2S_LIST_ROOMS:  return handle_list_rooms(fd);
        case C2S_CREATE_ROOM: return handle_create_room(fd, json);
        case C2S_JOIN_ROOM:   return handle_join_room(fd, json);
        case C2S_LEAVE_ROOM:  return handle_leave_room(fd);
        case C2S_SEARCH_ITEM: return handle_search_item(fd, json);
        case C2S_START_AUCTION: return handle_start_auction(fd);
        case C2S_CHAT:        return handle_chat(fd, json);

        // 3. Nhom quản lý vật phẩm trong hàng chờ
        case C2S_CREATE_ITEM: return handle_add_item(fd, json);
        case C2S_DELETE_ITEM: return handle_delete_item(fd, json);
        case C2S_UPDATE_ITEM: return handle_update_item(fd, json);

        // 4. Nhom logic đấu giá (Bidding)
        case C2S_BID:         return handle_bid(fd, json);
        case C2S_BUY_NOW:     return handle_buy_now(fd, json);

        // 5. Nhom báo cáo & lịch sử
        case C2S_GET_HISTORY: return handle_get_history(fd);

        // 6. Nhom quản trị viên (Admin)
        case C2S_ADMIN_LIST_USERS:       return handle_admin_list_users(fd);
        case C2S_ADMIN_DELETE_USER:     return handle_admin_delete_user(fd, json);
        case C2S_ADMIN_UPDATE_USER_ROLE: return handle_admin_update_role(fd, json);
        case C2S_PING:              return handle_ping();
        default:
            return create_error_response(ERR_UNKNOWN, "Unknown command");
    }
}

