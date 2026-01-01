#ifndef HANDLERS_H
#define HANDLERS_H

#include "protocol.h"
#include "../../third_party/cJSON/cJSON.h"

// Phan loai request tu Client
char *handle_client_message(int fd, cJSON *json);

#endif
