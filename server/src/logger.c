#include "../include/logger.h"
#include "../include/db_manager.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

void log_activity(const char *username, const char *action) {
    // 1. Ghi vao file log truyen thong
    FILE *f = fopen("auction_server.log", "a");
    if (f != NULL) {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0'; // Xoa ky tu xuong dong
        fprintf(f, "[%s] User: %-10s | Action: %s\n", timestamp, username ? username : "UNKNOWN", action);
        fclose(f);
    }
    // 2. Ghi vao Database
    db_log_activity(username, action);
}