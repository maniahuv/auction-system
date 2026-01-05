#include "auth.h"
#include "db_manager.h"
#include "user_manager.h"
#include "logger.h"
#include "json_util.h"
#include <string.h>

// Ham xu ly yeu cau dang ky tai khoan
char *handle_register(int fd, cJSON *json) {
    (void)fd;
    const char *user = get_json_string(json, "user");
    const char *pass = get_json_string(json, "pass");
    int role = 1;
    get_json_int(json, "role", &role);

    if (!user || !pass) return create_error_response(ERR_INVALID_MESSAGE, "Missing user or pass");

    if (db_register_user(user, pass, role) != 0) {
        return create_error_response(ERR_USER_EXISTS, "Username already exists or Database error");
    }

    log_activity(user, "REGISTERED new account");
    return create_ok_response();
}

// Ham xu ly yeu cau dang nhap
char *handle_login(int fd, cJSON *json) {
    const char *user = get_json_string(json, "user");
    const char *pass = get_json_string(json, "pass");
    int role_val = 1;

    if (db_login_user(user, pass, &role_val)) {
        UserState *u_ptr = get_user_by_fd(fd);
        if (u_ptr) {
            strcpy(u_ptr->username, user);
            u_ptr->is_logged_in = 1;
            u_ptr->role = role_val;
        }
        log_activity(user, "LOGGED IN");
        
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "type", S2C_LOGIN_SUCCESS);
        cJSON_AddNumberToObject(resp, "role", role_val);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }
    return create_error_response(ERR_WRONG_PASSWORD, "Invalid username or password");
}

// Ham xu ly yeu cau dang xuat
char *handle_logout(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (u && u->is_logged_in) {
        log_activity(u->username, "LOGGED OUT");
        
        // Reset trạng thái người dùng về mặc định
        memset(u->username, 0, sizeof(u->username));
        u->is_logged_in = 0;
        u->current_room_id = -1;
        u->role = 0;
        
        return create_ok_response(); // Trả về mã 800 để Client biết là OK
    }
    return create_error_response(ERR_UNKNOWN, "User not logged in");
}