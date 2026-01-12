#include "user_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#define HEARTBEAT_TIMEOUT 30 // Thoi gian timeout (giay) quá 30s mà không gửi tin là server ngắt kết nối
// Khoi tao trang thai nguoi dung moi khi ket noi
void init_user(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == 0) {
            users[i].fd = fd;
            memset(users[i].username, 0, 50);
            users[i].is_logged_in = 0;
            users[i].current_room_id = -1;
            users[i].last_heartbeat = time(NULL);
            printf("[UserMgr] Init state for fd %d at slot %d\n", fd, i);
            return;
        }
    }
    printf("[Warning] Server full, cannot init user for fd %d\n", fd);
}

// Xoa trang thai nguoi dung khi ngat ket noi
void clear_user(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == fd) {
            printf("[UserMgr] Clearing state for User '%s' (fd %d)\n", users[i].username, fd);
            users[i].fd = 0;
            users[i].is_logged_in = 0;
            users[i].current_room_id = -1;
            return;
        }
    }
}

// Lay con tro den trang thai nguoi dung theo fd
UserState *get_user_by_fd(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == fd) return &users[i];
    }
    return NULL;
}

// Gui thong diep den tat ca nguoi dung trong phong
void broadcast_to_room(int room_id, char *json_message) {
    printf("[Broadcast] Room %d: %s\n", room_id, json_message);
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd != 0 && users[i].is_logged_in && users[i].current_room_id == room_id) {
            // Tao ban sao rieng cho moi user de tranh loi bo nho
            char *msg_copy = strdup(json_message);
            if (msg_copy) {
                send_message(users[i].fd, msg_copy);
                free(msg_copy);
            }
        }
    }
}

// Lấy con trỏ đến UserState dựa trên username (dùng để kiểm tra đăng nhập trùng)
UserState *get_user_by_username(const char *username) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].is_logged_in && strcmp(users[i].username, username) == 0) {
            return &users[i];
        }
    }
    return NULL;
}
//  Quét và ngắt kết nối các user "im lặng" quá lâu


// Sửa lại hàm này
void check_connection_timeouts(fd_set *master_set) { // Thêm tham số
    time_t now = time(NULL);
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd != 0) {
            double inactive_time = difftime(now, users[i].last_heartbeat);
            if (inactive_time > HEARTBEAT_TIMEOUT) {
                printf("[Timeout] Disconnecting fd %d due to inactivity (%.0fs)\n", users[i].fd, inactive_time);

                int fd_to_close = users[i].fd; // Lưu lại FD trước khi xóa

                // 1. Đóng socket
                close(fd_to_close);

                // 2. QUAN TRỌNG: Xóa khỏi danh sách theo dõi của Select
                FD_CLR(fd_to_close, master_set);

                // 3. Xóa dữ liệu user
                clear_user(fd_to_close);
            }
        }
    }
}
void update_heartbeat(int fd) {
    UserState *u = get_user_by_fd(fd);
    if (u) u->last_heartbeat = time(NULL);
}
