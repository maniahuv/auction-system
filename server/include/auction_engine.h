#ifndef AUCTION_ENGINE_H
#define AUCTION_ENGINE_H

#include "cJSON.h"

// Kiem tra trang thai tat ca phong dau gia theo moi giay
void check_auctions();

// Xu ly dat gia va kiem tra buoc gia
char *handle_bid(int fd, cJSON *json);

// Xu ly lenh mua ngay
char *handle_buy_now(int fd, cJSON *json);

#endif