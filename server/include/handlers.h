#ifndef HANDLERS_H
#define HANDLERS_H

#include "protocol.h"
#include "../../third_party/cJSON/cJSON.h"

char *handle_register(int fd, cJSON *json);
char *handle_login(int fd, cJSON *json);
char *handle_bid(int fd, cJSON *json);
char *handle_buy_now(int fd, cJSON *json);
char *handle_create_room(int fd, cJSON *json);
char *handle_delete_item(int fd, cJSON *json);
char *handle_add_item(int fd, cJSON *json);
char *handle_search_item(int fd, cJSON *json);
char *handle_list_rooms(int fd);
char *handle_join_room(int fd, cJSON *json);
char *handle_get_history(int fd);
void broadcast_to_room(int room_id, char *json_message);

#endif
