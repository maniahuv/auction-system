#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/db_manager.h"
#include "cJSON.h"

sqlite3 *db;

int db_init(const char *db_name) {
    int rc = sqlite3_open(db_name, &db);
    if (rc) return rc;

    // 1. Tạo bảng Users
    const char *sql_users = "CREATE TABLE IF NOT EXISTS users ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "username TEXT UNIQUE,"
                            "password TEXT,"
                            "role INTEGER);";
    
    // 2. Tạo bảng History (Lưu kết quả đấu giá)
    const char *sql_history = "CREATE TABLE IF NOT EXISTS history ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                              "winner TEXT,"
                              "item TEXT,"
                              "price INTEGER,"
                              "timestamp INTEGER,"
                              "owner TEXT);";

    // 3. Tạo bảng Activity Logs (Lưu nhật ký hoạt động)
    const char *sql_logs = "CREATE TABLE IF NOT EXISTS activity_logs ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "username TEXT,"
                           "action TEXT,"
                           "timestamp INTEGER);";

    sqlite3_exec(db, sql_users, 0, 0, 0);
    sqlite3_exec(db, sql_history, 0, 0, 0);
    sqlite3_exec(db, sql_logs, 0, 0, 0);
    
    return SQLITE_OK;
}

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

// ĐÂY LÀ HÀM BỊ THIẾU KHIẾN BẠN GẶP LỖI
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

char* db_get_history_json(const char *username, int role) {
    sqlite3_stmt *res;
    const char *sql;

    // Phân quyền truy vấn dựa trên Role
    if (role == ROLE_ADMIN) {
        sql = "SELECT winner, item, price, timestamp FROM history ORDER BY timestamp DESC;";
    } else if (role == ROLE_AUCTIONEER) {
        sql = "SELECT winner, item, price, timestamp FROM history WHERE owner = ? ORDER BY timestamp DESC;";
    } else {
        sql = "SELECT winner, item, price, timestamp FROM history WHERE winner = ? ORDER BY timestamp DESC;";
    }

    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return NULL;
    
    // Bind username cho các role không phải Admin
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
        cJSON_AddItemToArray(arr, obj);
    }

    cJSON_AddItemToObject(resp, "history", arr);
    sqlite3_finalize(res);
    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

void db_close() {
    if (db) sqlite3_close(db);
}