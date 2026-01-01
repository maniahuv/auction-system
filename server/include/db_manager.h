#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "../../third_party/sqlite/sqlite3.h"
#include "protocol.h"

// Khởi tạo database và tạo các bảng nếu chưa có
int db_init(const char *db_name);

// Quản lý người dùng
int db_register_user(const char *user, const char *pass, int role);
int db_login_user(const char *user, const char *pass, int *role);

// Quản lý lịch sử (trả về chuỗi JSON để handler gửi đi)
char* db_get_history_json(const char *username, int role);

// Ghi lịch sử đấu giá
int db_save_auction_result(const char *winner, const char *item, int price, const char *owner);

// Ghi log hoạt động
void db_log_activity(const char *username, const char *action);

void db_close();

#endif