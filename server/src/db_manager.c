#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "db_manager.h"
#include "cJSON.h"
#include "state.h"

sqlite3 *db;

// Khoi tao cac bang trong database
int db_init(const char *db_name) {
    int rc = sqlite3_open(db_name, &db);
    if (rc) return rc;

    // 1. Tao bang USERS
    const char *sql_users = "CREATE TABLE IF NOT EXISTS users ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "username TEXT UNIQUE,"
                            "password TEXT,"
                            "role INTEGER);";
    
    // 2. Tao bang HISTORY (Luu lich su dau gia kem nguoi ban)
    const char *sql_history = "CREATE TABLE IF NOT EXISTS history ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                              "winner TEXT,"
                              "item TEXT,"
                              "price INTEGER,"
                              "timestamp INTEGER,"
                              "owner TEXT);";

    // 3. Tao bang ACTIVITY_LOGS
    const char *sql_logs = "CREATE TABLE IF NOT EXISTS activity_logs ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "username TEXT,"
                           "action TEXT,"
                           "timestamp INTEGER);";

    // 4. Tao bang ACTIVE_ROOMS (Luu trang thai tuc thoi cua phong)
    const char *sql_active_rooms = "CREATE TABLE IF NOT EXISTS active_rooms ("
                                   "id INTEGER PRIMARY KEY,"
                                   "owner TEXT,"
                                   "current_idx INTEGER,"
                                   "price INTEGER,"
                                   "bidder_fd INTEGER,"
                                   "end_time INTEGER,"
                                   "is_active INTEGER,"
                                   "is_started INTEGER," 
                                   "total_items INTEGER);";

    // 5. Tao bang ROOM_ITEMS (Luu danh sach vat pham trong hang cho cua phong)
    const char *sql_room_items = "CREATE TABLE IF NOT EXISTS room_items ("
                                 "room_id INTEGER,"
                                 "item_idx INTEGER,"
                                 "title TEXT,"
                                 "start_price INTEGER,"
                                 "buy_now INTEGER,"
                                 "PRIMARY KEY (room_id, item_idx));";

    sqlite3_exec(db, sql_users, 0, 0, 0);
    sqlite3_exec(db, sql_history, 0, 0, 0);
    sqlite3_exec(db, sql_logs, 0, 0, 0);
    sqlite3_exec(db, sql_active_rooms, 0, 0, 0);
    sqlite3_exec(db, sql_room_items, 0, 0, 0);
    
    return SQLITE_OK;
}

// Luu tai khoan nguoi dung moi
int db_register_user(const char *user, const char *pass, int role) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO users (username, password, role) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pass, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, role);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Kiem tra tai khoan dang nhap nguoi dung
int db_login_user(const char *user, const char *pass, int *role) {
    sqlite3_stmt *res;
    const char *sql = "SELECT role FROM users WHERE username = ? AND password = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return 0;
    
    sqlite3_bind_text(res, 1, user, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 2, pass, -1, SQLITE_STATIC);

    int step = sqlite3_step(res);
    if (step == SQLITE_ROW) {
        *role = sqlite3_column_int(res, 0);
        sqlite3_finalize(res);
        return 1; 
    }
    sqlite3_finalize(res);
    return 0; 
}

// Luu ket qua dau gia vao bang HISTORY
int db_save_auction_result(const char *winner, const char *item, int price, const char *owner) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO history (winner, item, price, timestamp, owner) VALUES (?, ?, ?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, winner, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, item, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, price);
    sqlite3_bind_int(stmt, 4, (int)time(NULL));
    sqlite3_bind_text(stmt, 5, owner, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Luu nhat ky hoat dong cua nguoi dung
void db_log_activity(const char *username, const char *action) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO activity_logs (username, action, timestamp) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username ? username : "UNKNOWN", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, action, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, (int)time(NULL));
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

// --- NÂNG CẤP: Lay lich su dau gia day du thong tin (Winner, Item, Price, Time, Owner) ---
char* db_get_history_json(const char *username, int role) {
    sqlite3_stmt *res;
    const char *sql;

    // Phan quyen truy van dua tren role
    if (role == ROLE_ADMIN) {
        sql = "SELECT winner, item, price, timestamp, owner FROM history ORDER BY timestamp DESC;";
    } else if (role == ROLE_AUCTIONEER) {
        sql = "SELECT winner, item, price, timestamp, owner FROM history WHERE owner = ? ORDER BY timestamp DESC;";
    } else {
        sql = "SELECT winner, item, price, timestamp, owner FROM history WHERE winner = ? ORDER BY timestamp DESC;";
    }

    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return NULL;
    
    if (role != ROLE_ADMIN) {
        sqlite3_bind_text(res, 1, username, -1, SQLITE_STATIC);
    }
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_HISTORY_LIST);
    cJSON *arr = cJSON_CreateArray();

    while (sqlite3_step(res) == SQLITE_ROW) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "winner", (const char*)sqlite3_column_text(res, 0));
        cJSON_AddStringToObject(obj, "item", (const char*)sqlite3_column_text(res, 1));
        cJSON_AddNumberToObject(obj, "price", sqlite3_column_int(res, 2));
        cJSON_AddNumberToObject(obj, "time", sqlite3_column_int(res, 3));
        // MỚI: Đóng gói thêm tên người bán (owner) vào JSON để hiển thị cho Bidder
        const char *owner_val = (const char*)sqlite3_column_text(res, 4);
        cJSON_AddStringToObject(obj, "owner", owner_val ? owner_val : "---");
        cJSON_AddItemToArray(arr, obj);
    }

    cJSON_AddItemToObject(resp, "history", arr);
    sqlite3_finalize(res);
    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

// --- QUẢN LÝ TRẠNG THÁI PHÒNG ĐỂ CHỐNG MẤT DỮ LIỆU KHI SERVER SẬP ---

int db_update_room_state(int room_id, int current_item_idx, int current_price, int highest_bidder_id, long end_time) {
    sqlite3_stmt *stmt;
    RoomState *r = &rooms[room_id - 1];

    // 1. Cap nhat thong tin co ban cua phong vao active_rooms
    const char *sql_room = "INSERT OR REPLACE INTO active_rooms (id, owner, current_idx, price, bidder_fd, end_time, is_active, is_started, total_items) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql_room, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_text(stmt, 2, r->owner_username, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, current_item_idx);
    sqlite3_bind_int(stmt, 4, current_price);
    sqlite3_bind_int(stmt, 5, highest_bidder_id);
    sqlite3_bind_int(stmt, 6, (int)end_time);
    sqlite3_bind_int(stmt, 7, r->is_active);
    sqlite3_bind_int(stmt, 8, r->is_started); 
    sqlite3_bind_int(stmt, 9, r->total_items);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // 2. Dong bo danh sach vat pham trong hang cho (room_items)
    const char *del_items = "DELETE FROM room_items WHERE room_id = ?;";
    sqlite3_prepare_v2(db, del_items, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    const char *ins_item = "INSERT INTO room_items (room_id, item_idx, title, start_price, buy_now) VALUES (?, ?, ?, ?, ?);";
    for (int i = 0; i < r->total_items; i++) {
        sqlite3_prepare_v2(db, ins_item, -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, room_id);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_text(stmt, 3, r->queue[i].title, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, r->queue[i].start_price);
        sqlite3_bind_int(stmt, 5, r->queue[i].buy_now_price);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return 0;
}

int db_load_active_auctions(void *rooms_array) {
    RoomState *rooms_ptr = (RoomState *)rooms_array;
    sqlite3_stmt *res_rooms, *res_items;
    
    // Truy van cac phong dang active
    const char *sql_rooms = "SELECT id, owner, current_idx, price, end_time, is_active, is_started, total_items FROM active_rooms WHERE is_active = 1;";
    if (sqlite3_prepare_v2(db, sql_rooms, -1, &res_rooms, 0) != SQLITE_OK) return -1;
    
    while (sqlite3_step(res_rooms) == SQLITE_ROW) {
        int r_id = sqlite3_column_int(res_rooms, 0);
        int idx = r_id - 1;
        if (idx < 0 || idx >= MAX_ROOMS) continue;
        
        RoomState *r = &rooms_ptr[idx];
        r->room_id = r_id;
        strcpy(r->owner_username, (const char*)sqlite3_column_text(res_rooms, 1));
        r->current_item_idx = sqlite3_column_int(res_rooms, 2);
        r->current_price = sqlite3_column_int(res_rooms, 3);
        r->highest_bidder_id = -1; // Reset FD vi socket fd cu khong con gia tri sau khi server sập
        r->end_time = (time_t)sqlite3_column_int(res_rooms, 4);
        r->is_active = sqlite3_column_int(res_rooms, 5);
        r->is_started = sqlite3_column_int(res_rooms, 6); 
        r->total_items = sqlite3_column_int(res_rooms, 7);
        r->sent_warning = 0;

        // Truy van va nap lai hang cho vat pham cua phong
        const char *sql_items = "SELECT item_idx, title, start_price, buy_now FROM room_items WHERE room_id = ?;";
        sqlite3_prepare_v2(db, sql_items, -1, &res_items, 0);
        sqlite3_bind_int(res_items, 1, r_id);
        while (sqlite3_step(res_items) == SQLITE_ROW) {
            int i_idx = sqlite3_column_int(res_items, 0);
            if (i_idx >= 0 && i_idx < MAX_ITEMS_PER_ROOM) {
                strcpy(r->queue[i_idx].title, (const char*)sqlite3_column_text(res_items, 1));
                r->queue[i_idx].start_price = sqlite3_column_int(res_items, 2);
                r->queue[i_idx].buy_now_price = sqlite3_column_int(res_items, 3);
            }
        }
        sqlite3_finalize(res_items);
    }
    sqlite3_finalize(res_rooms);
    return 0;
}

// --- QUẢN LÝ ADMIN ---

// Lấy danh sách toàn bộ người dùng dưới dạng JSON
char* db_get_all_users_json() {
    sqlite3_stmt *res;
    const char *sql = "SELECT id, username, role FROM users;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return NULL;
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "type", S2C_USER_LIST);
    cJSON *arr = cJSON_CreateArray();

    while (sqlite3_step(res) == SQLITE_ROW) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "id", sqlite3_column_int(res, 0));
        cJSON_AddStringToObject(obj, "username", (const char*)sqlite3_column_text(res, 1));
        cJSON_AddNumberToObject(obj, "role", sqlite3_column_int(res, 2));
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON_AddItemToObject(resp, "users", arr);
    sqlite3_finalize(res);
    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

// Xóa người dùng theo ID
int db_delete_user_by_id(int user_id) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM users WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Cập nhật Role cho người dùng
int db_update_user_role(int user_id, int new_role) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE users SET role = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, new_role);
    sqlite3_bind_int(stmt, 2, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Dong ket noi database
void db_close() {
    if (db) sqlite3_close(db);
}