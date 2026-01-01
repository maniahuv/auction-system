#include "cmd_parser.h"
#include "client_state.h"
#include "protocol.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* convert_command_to_json(char *input) {
    char tmp[4096];
    strncpy(tmp, input, 4096);
    
    char *cmd = strtok(tmp, " ");
    if (!cmd) return NULL;

    cJSON *req = cJSON_CreateObject();
    
    // Auth: register <u] <p> <role], login <u] <p>
    if (strcmp(cmd, "register") == 0) {
        char *user = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        char *role_str = strtok(NULL, " ");
        if (user && pass && role_str) {
            cJSON_AddNumberToObject(req, "type", C2S_REGISTER);
            cJSON_AddStringToObject(req, "user", user);
            cJSON_AddStringToObject(req, "pass", pass);
            cJSON_AddNumberToObject(req, "role", atoi(role_str));
        } else {
            printf(">> Dung: register <u] <p> <role: 1-Bidder, 2-Auctioneer>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    else if (strcmp(cmd, "login") == 0) {
        char *user = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        if (user && pass) {
            cJSON_AddNumberToObject(req, "type", C2S_LOGIN);
            cJSON_AddStringToObject(req, "user", user);
            cJSON_AddStringToObject(req, "pass", pass);
        } else {
            printf(">> Dung: login <u] <p>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // Room: list, create <title] <p] <buy], join <id], leave
    else if (strcmp(cmd, "create") == 0) {
        char *title = strtok(NULL, " ");
        char *price = strtok(NULL, " ");
        char *buy = strtok(NULL, " ");
        if (title && price && buy) {
            cJSON_AddNumberToObject(req, "type", C2S_CREATE_ROOM);
            cJSON_AddStringToObject(req, "title", title);
            cJSON_AddNumberToObject(req, "start_price", atoi(price));
            cJSON_AddNumberToObject(req, "buy_now", atoi(buy));
        } else {
            printf(">> Dung: create <title> <start_price> <buy_now>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    else if (strcmp(cmd, "additem") == 0) {
        char *rid = strtok(NULL, " ");
        char *title = strtok(NULL, " ");
        char *price = strtok(NULL, " ");
        char *buy = strtok(NULL, " ");
        if (rid && title && price && buy) {
            cJSON_AddNumberToObject(req, "type", C2S_CREATE_ITEM);
            cJSON_AddNumberToObject(req, "room_id", atoi(rid));
            cJSON_AddStringToObject(req, "title", title);
            cJSON_AddNumberToObject(req, "start_price", atoi(price));
            cJSON_AddNumberToObject(req, "buy_now", atoi(buy));
        } else {
            printf(">> Dung: additem <room_id> <title> <start_price> <buy_now>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    else if (strcmp(cmd, "list") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_LIST_ROOMS);
    }
    else if (strcmp(cmd, "join") == 0) {
        char *id = strtok(NULL, " ");
        if (id) {
            cJSON_AddNumberToObject(req, "type", C2S_JOIN_ROOM);
            cJSON_AddNumberToObject(req, "room_id", atoi(id));
        } else {
            printf(">> Dung: join <room_id>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    else if (strcmp(cmd, "leave") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_LEAVE_ROOM);
    }
    // Auction: bid <p], buynow, search <kw], history
    else if (strcmp(cmd, "bid") == 0) {
        char *p = strtok(NULL, " ");
        if (p) {
            cJSON_AddNumberToObject(req, "type", C2S_BID);
            cJSON_AddNumberToObject(req, "price", atoi(p));
        } else {
            printf(">> Dung: bid <price>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    else if (strcmp(cmd, "buynow") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_BUY_NOW);
    }
    else if (strcmp(cmd, "history") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_GET_HISTORY);
    }
    else if (strcmp(cmd, "search") == 0) {
        char *kw = strtok(NULL, " ");
        if (kw) {
            cJSON_AddNumberToObject(req, "type", C2S_SEARCH_ITEM);
            cJSON_AddStringToObject(req, "keyword", kw);
        } else {
            printf(">> Dung: search <keyword>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    else if (cmd[0] == '{') { // Debug: gui JSON truc tiep
        cJSON_Delete(req);
        return strdup(input);
    }
    else {
        printf(">> Lenh khong hop le cho vai tro %d!\n", current_role);
        cJSON_Delete(req); return NULL;
    }

    // --- XU LY LOI NHAP SAI ---
    if (cJSON_GetArraySize(req) == 0) { 
        if (current_role == 0) {
            printf(">> Ban chua dang nhap! Hay register hoac login\n");
        } else {
            char *role_name = (current_role == 1) ? "BIDDER" : (current_role == 2) ? "AUCTIONEER" : "ADMIN";
            printf(">> Lenh khong hop le cho vai tro %s!\n", role_name);
            printf(">> Cac lenh co san: list, join, leave, search, history");
            
            if (current_role == 2) printf(", create, additem, delete\n");
            else if (current_role == 1) printf(", bid, buynow\n");
            else printf("\n");
        }
        cJSON_Delete(req);
        return NULL;
    }

    char *json_str = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);
    return json_str;
}