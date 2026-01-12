#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "state.h"

// Khoi tao slot cho user moi ket noi
void init_user(int fd);

// Xoa khi user ngat ket noi
void clear_user(int fd);

// Tim UserState theo fd
UserState *get_user_by_fd(int fd);

// Broadcast cho tat ca user trong room_id
void broadcast_to_room(int room_id, char *json_message);

UserState *get_user_by_username(const char *username);

#include <sys/select.h> // Thêm thư viện này nếu chưa có
void check_connection_timeouts(fd_set *master_set);

void update_heartbeat(int fd);
#endif
