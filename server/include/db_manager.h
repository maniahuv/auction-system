#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "../../third_party/sqlite/sqlite3.h"
#include "protocol.h"

// Khoi tao database va tao cac bang neu chua co
int db_init(const char *db_name);

// Quan ly nguoi dung
int db_register_user(const char *user, const char *pass, int role);
int db_login_user(const char *user, const char *pass, int *role);

// Quan ly lich su (tra ve chuoi JSON de handler gui di)
char* db_get_history_json(const char *username, int role);

// Ghi lich su dau gia
int db_save_auction_result(const char *winner, const char *item, int price, const char *owner);

// Ghi log hoat dong
void db_log_activity(const char *username, const char *action);

// --- Chong mat du lieu khi server sap ---

// Cap nhat trang thai tuc thoi cua phong vao Database
// SỬA: Thay int highest_bidder_id thanh const char *bidder_name de khop voi db_manager.c
int db_update_room_state(int room_id, int current_item_idx, int current_price, const char *bidder_name, long end_time);

// Khoi phuc trang thai cac phong tu Database khi Server khoi dong lai
int db_load_active_auctions(void *rooms_array);
char* db_get_all_users_json();
int db_delete_user_by_id(int user_id);
int db_update_user_role(int user_id, int new_role);

void db_close();

#endif