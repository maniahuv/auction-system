#include "user_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"

// Khoi tao trang thai nguoi dung moi khi ket noi
void init_user(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == 0) {
            users[i].fd = fd;
            memset(users[i].username, 0, 50);
            users[i].is_logged_in = 0;
            users[i].current_room_id = -1;
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