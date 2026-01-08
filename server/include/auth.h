#ifndef AUTH_H
#define AUTH_H
#include "cJSON.h"

// Xu ly yeu cau dang ky tai khoan moi
char *handle_register(int fd, cJSON *json);

// Xu ly yeu cau dang nhap va khoi tao quyen
char *handle_login(int fd, cJSON *json);

char *handle_logout(int fd);
#endif