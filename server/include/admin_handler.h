#ifndef ADMIN_HANDLER_H
#define ADMIN_HANDLER_H

#include <cJSON.h>

char *handle_admin_list_users(int fd);
char *handle_admin_delete_user(int fd, cJSON *json);
char *handle_admin_update_role(int fd, cJSON *json);

#endif