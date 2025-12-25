#ifndef HANDLERS.H
#define SHANDLERS.H

char *handle_login(int fd, cJSON *json);
char *handle_bid(int fd, cJSON *json);
char *handle_create_room(int fd, cJSON *json);
char *handle_list_rooms(int fd);
char *handle_join_room(int fd, cJSON *json);


#endif
