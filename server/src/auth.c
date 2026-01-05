#include "auth.h"
#include "db_manager.h"
#include "user_manager.h"
#include "logger.h"
#include "json_util.h"
#include "server.h" // Thêm để sử dụng hàm send_message
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

// Ham xu ly yeu cau dang nhap (Co logic xu ly dang nhap trung lap)
char *handle_login(int fd, cJSON *json) {
    const char *user = get_json_string(json, "user");
    const char *pass = get_json_string(json, "pass");
    int role_val = 1;

    if (!user || !pass) return create_error_response(ERR_INVALID_MESSAGE, "Missing user or pass");

    // Kiem tra thong tin xac thuc tu Database
    if (db_login_user(user, pass, &role_val)) {
        
        // --- LOGIC XU LY DANG NHAP TRUNG LAP (KICK OLD SESSION) ---
        // Tim kiem xem username nay co dang online o socket khac khong
        UserState *old_session = get_user_by_username(user);
        if (old_session && old_session->fd != fd) {
            printf("[Auth] Tai khoan '%s' dang nhap moi tai fd %d. Dang kick phien cu tai fd %d...\n", 
                   user, fd, old_session->fd);

            // 1. Gui thong bao cho Client cu biet bi Kick
            char *kick_msg = create_error_response(ERR_UNKNOWN, "Tai khoan da dang nhap o noi khac!");
            send_message(old_session->fd, kick_msg);
            free(kick_msg);

            // 2. Xoa trang thai cua socket cu trong RAM
            old_session->is_logged_in = 0;
            old_session->current_room_id = -1;
            memset(old_session->username, 0, sizeof(old_session->username));
        }
        // ---------------------------------------------------------

        // Thiet lap trang thai dang nhap cho socket hien tai
        UserState *u_ptr = get_user_by_fd(fd);
        if (u_ptr) {
            strncpy(u_ptr->username, user, sizeof(u_ptr->username) - 1);
            u_ptr->is_logged_in = 1;
            u_ptr->role = role_val;
        }
        
        log_activity(user, "LOGGED IN");
        
        // Tra ve phan hoi dang nhap thanh cong kem theo Role
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
        
        // Reset trang thai nguoi dung ve mac dinh (phien lam viec vang lai)
        memset(u->username, 0, sizeof(u->username));
        u->is_logged_in = 0;
        u->current_room_id = -1;
        u->role = 0;
        
        // Tra ve thong bao thanh cong de GUI Python co the chuyen man hinh
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "type", S2C_GENERIC_OK);
        cJSON_AddStringToObject(resp, "message", "LOGOUT_SUCCESS");
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }
    return create_error_response(ERR_UNKNOWN, "User not logged in");
}