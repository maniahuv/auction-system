#include "server.h"
#include "state.h"


UserState* get_user_by_fd(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == fd) return &users[i];
    }
    return NULL;
}

char* handle_login(int fd, cJSON *json) {
    char *user = (char*)get_json_string(json, "user");
    char *pass = (char*)get_json_string(json, "pass");

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

char* handle_bid(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) {
        return create_error_response(ERR_UNKNOWN, "You must login first");
    }

    int amount = 0;
    if (!get_json_int(json, "amount", &amount)) {
         return create_error_response(ERR_INVALID_MESSAGE, "Missing amount");
    }

    return create_ok_response();
}

// Hàm xử lý: TẠO PHÒNG
char* handle_create_room(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) return create_error_response(ERR_UNKNOWN, "Login required");

    char *title = get_json_string(json, "title");
    int start_price = 0;
    get_json_int(json, "start_price", &start_price);

    if (!title || start_price <= 0) return create_error_response(ERR_INVALID_MESSAGE, "Invalid title or price");

    // Tìm slot phòng trống
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == 0) { // Slot trống
            rooms[i].room_id = i + 1; // ID bắt đầu từ 1
            strncpy(rooms[i].title, title, 99);
            rooms[i].current_price = start_price;
            rooms[i].highest_bidder_id = -1;
            rooms[i].is_active = 1;

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
char* handle_list_rooms(int fd) {
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
char* handle_join_room(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (!u || !u->is_logged_in) return create_error_response(ERR_UNKNOWN, "Login required");

    int room_id = 0;
    get_json_int(json, "room_id", &room_id);

    // Tìm phòng
    int found = 0;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == room_id && rooms[i].is_active) {
            found = 1;
            u->current_room_id = room_id; // Gán user vào phòng này

            // TODO: Ở bước sau ta sẽ làm Broadcast "User A đã vào phòng"
            break;
        }
    }

    if (!found) return create_error_response(ERR_ROOM_NOT_FOUND, "Room not found");

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_JOIN_ROOM_SUCCESS);
    cJSON_AddNumberToObject(resp, "room_id", room_id);
    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}