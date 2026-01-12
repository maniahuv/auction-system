#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "cJSON.h"

// Quan ly phong dau gia
char *handle_create_room(int fd, cJSON *json);
char *handle_list_rooms(int fd);
char *handle_join_room(int fd, cJSON *json);
char *handle_leave_room(int fd);

// Quan ly hang cho vat pham dau gia
char *handle_add_item(int fd, cJSON *json);
char *handle_delete_item(int fd, cJSON *json);
char *handle_update_item(int fd, cJSON *json);

// Ham tim kiem vat pham tren toan bo phong dau gia va lich su dau gia
char *handle_search_item(int fd, cJSON *json);
char *handle_get_history(int fd);
char *handle_start_auction(int fd);

char *handle_chat(int fd, cJSON *json);
#endif 