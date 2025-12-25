#ifndef HANDLERS_H
#define HANDLERS_H

char *handle_login(int fd, cJSON *json);
char *handle_bid(int fd, cJSON *json);
char *handle_create_room(int fd, cJSON *json);
char *handle_list_rooms(int fd);
char *handle_join_room(int fd, cJSON *json);
void broadcast_to_room(int room_id, char *json_message);

#endif
