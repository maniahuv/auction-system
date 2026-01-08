#include "db_manager.h"
#include "json_util.h"
#include "user_manager.h"
#include "logger.h"

char *handle_admin_list_users(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (u->role != ROLE_ADMIN) return create_error_response(ERR_UNKNOWN, "Quyen Admin yeu cau!");
    
    return db_get_all_users_json();
}

char *handle_admin_delete_user(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (u->role != ROLE_ADMIN) return create_error_response(ERR_UNKNOWN, "Quyen Admin yeu cau!");

    int target_id;
    get_json_int(json, "user_id", &target_id);
    
    if (db_delete_user_by_id(target_id) == 0) {
        log_activity(u->username, "ADMIN: Da xoa user ID");
        return create_ok_response();
    }
    return create_error_response(ERR_UNKNOWN, "Loi khi xoa user");
}

char *handle_admin_update_role(int fd, cJSON *json) {
    UserState *u = get_user_by_fd(fd);
    if (u->role != ROLE_ADMIN) return create_error_response(ERR_UNKNOWN, "Quyen Admin yeu cau!");

    int target_id, new_role;
    get_json_int(json, "user_id", &target_id);
    get_json_int(json, "new_role", &new_role);
    
    if (db_update_user_role(target_id, new_role) == 0) {
        return create_ok_response();
    }
    return create_error_response(ERR_UNKNOWN, "Loi khi cap nhat quyen");
}