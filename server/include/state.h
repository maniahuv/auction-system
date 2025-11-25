#include "server.h"
#include "state.h"

// Helper: Tim user theo socket fd
UserState* get_user_by_fd(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == fd) return &users[i];
    }
    return NULL;
}

// Xu ly dang nhap
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

// Xu ly chat dau gia
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