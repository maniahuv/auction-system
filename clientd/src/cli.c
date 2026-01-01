#include "server.h" 
#include <ctype.h>
#include "state.h"

#define BUFFER_SIZE 4096

// Bien luu vai tro hien tai de in huong dan lenh
static int current_role = 0;

// Ham cat bo khoang trang dau/cuoi chuoi
void trim(char *s) {
    char *p = s;
    int l = strlen(p);
    while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
    while(*p && isspace(*p)) ++p, --l;
    memmove(s, p, l + 1);
}

// Ham chuyen doi lenh nguoi dung nhap thanh cau truc JSON de gui den Server
char* convert_command_to_json(char *input) {
    // Su dung ban sao de tranh thay doi chuoi goc
    char tmp[BUFFER_SIZE];
    strncpy(tmp, input, BUFFER_SIZE);
    
    char *cmd = strtok(tmp, " ");
    if (!cmd) return NULL;

    cJSON *req = cJSON_CreateObject();
    
    // --- LENH: REGISTER user pass role ---
    if (strcmp(cmd, "register") == 0) {
        char *user = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        char *role_str = strtok(NULL, " ");
        if (user && pass && role_str) {
            cJSON_AddNumberToObject(req, "type", C2S_REGISTER); // 101
            cJSON_AddStringToObject(req, "user", user);
            cJSON_AddStringToObject(req, "pass", pass);
            cJSON_AddNumberToObject(req, "role", atoi(role_str));
        } else {
            printf(">> Sai cu phap! Dung: register <user> <pass> <role: 1-Bidder, 2-Auctioneer, 3-Admin>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LENH: LOGIN user pass ---
    else if (strcmp(cmd, "login") == 0) {
        char *user = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        if (user && pass) {
            cJSON_AddNumberToObject(req, "type", C2S_LOGIN); // 102
            cJSON_AddStringToObject(req, "user", user);
            cJSON_AddStringToObject(req, "pass", pass);
        } else {
            printf(">> Sai cu phap! Dung: login <user> <pass>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LENH: CREATE title start_price buy_now_price ---
    else if (strcmp(cmd, "create") == 0) {
        char *title = strtok(NULL, " ");
        char *price_str = strtok(NULL, " ");
        char *buy_now_str = strtok(NULL, " ");
        if (title && price_str && buy_now_str) {
            cJSON_AddNumberToObject(req, "type", C2S_CREATE_ROOM); // 202
            cJSON_AddStringToObject(req, "title", title);
            cJSON_AddNumberToObject(req, "start_price", atoi(price_str));
            cJSON_AddNumberToObject(req, "buy_now", atoi(buy_now_str));
        } else {
            printf(">> Sai cu phap! Dung: create <title> <start_price> <buy_now_price>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LENH: ADDITEM room_id title start_price buy_now_price ---
    else if (strcmp(cmd, "additem") == 0) {
        char *room_id_str = strtok(NULL, " ");
        char *title = strtok(NULL, " ");
        char *price_str = strtok(NULL, " ");
        char *buy_now_str = strtok(NULL, " ");
        if (room_id_str && title && price_str && buy_now_str) {
            cJSON_AddNumberToObject(req, "type", C2S_CREATE_ITEM); // 301
            cJSON_AddNumberToObject(req, "room_id", atoi(room_id_str));
            cJSON_AddStringToObject(req, "title", title);
            cJSON_AddNumberToObject(req, "start_price", atoi(price_str));
            cJSON_AddNumberToObject(req, "buy_now", atoi(buy_now_str));
        } else {
            printf(">> Sai cu phap! Dung: additem <room_id> <title> <start_price> <buy_now_price>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // Lenh xoa vat pham khoi hang cho dau gia
    else if (strcmp(cmd, "delete") == 0) {
        char *room_id_str = strtok(NULL, " ");
        char *idx_str = strtok(NULL, " ");
        
        if (room_id_str && idx_str) {
            cJSON_AddNumberToObject(req, "type", C2S_DELETE_ITEM); // 302
            cJSON_AddNumberToObject(req, "room_id", atoi(room_id_str));
            cJSON_AddNumberToObject(req, "item_index", atoi(idx_str));
        } else {
            printf(">> Sai cu phap! Dung: delete <room_id> <item_index>\n");
            cJSON_Delete(req); return NULL;
        }
    }

    // --- LENH: JOIN room_id ---
    else if (strcmp(cmd, "join") == 0) {
        char *id_str = strtok(NULL, " ");
        if (id_str) {
            cJSON_AddNumberToObject(req, "type", C2S_JOIN_ROOM); // 203
            cJSON_AddNumberToObject(req, "room_id", atoi(id_str));
        } else {
            printf(">> Sai cu phap! Dung: join <room_id>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LENH: LEAVE ---
    else if (strcmp(cmd, "leave") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_LEAVE_ROOM); // 204
    }
    // --- LENH: BID price ---
    else if (strcmp(cmd, "bid") == 0) {
        char *price_str = strtok(NULL, " ");
        if (price_str) {
            cJSON_AddNumberToObject(req, "type", C2S_BID); // 401
            cJSON_AddNumberToObject(req, "price", atoi(price_str));
        } else {
            printf(">> Sai cu phap! Dung: bid <amount>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LENH: BUYNOW ---
    else if (strcmp(cmd, "buynow") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_BUY_NOW); // 402
    }
    // --- LENH: LIST ---
    else if (strcmp(cmd, "list") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_LIST_ROOMS); // 201
    }

    else if (strcmp(cmd, "search") == 0) {
        char *keyword = strtok(NULL, " ");
        if (keyword) {
            cJSON_AddNumberToObject(req, "type", C2S_SEARCH_ITEM); // 205
            cJSON_AddStringToObject(req, "keyword", keyword);
        } else {
            printf(">> Sai cu phap! Dung: search <tu_khoa>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LENH: HISTORY ---
    else if (strcmp(cmd, "history") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_GET_HISTORY); // 501
    }
    // --- Ho tro nhap JSON tho ---
    else if (cmd[0] == '{') {
        cJSON_Delete(req);
        return strdup(input);
    }
    else {
        // Hien thi thong bao loi theo vai tro hien tai
        if (current_role == 0) {
            printf(">> Lenh khong hop le! Cac lenh co san: register, login\n");
        } else if (current_role == ROLE_ADMIN) {
            printf(">> Lenh khong hop le! (Admin: delete, list, join, leave, search, history)\n");
        } else if (current_role == ROLE_AUCTIONEER) {
            printf(">> Lenh khong hop le! (Auctioneer: create, additem, delete, list, join, leave, search, history)\n");
        } else {
            printf(">> Lenh khong hop le! (Bidder: list, join, leave, bid, buynow, search, history)\n");
        }
        cJSON_Delete(req);
        return NULL;
    }

    char *json_str = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);
    return json_str;
}


// Ham in phan hoi tu Server
void print_server_response(char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        printf("RAW: %s\n", json_str);
        return;
    }

    int type = 0;
    get_json_int(json, "type", &type);

    switch (type) {
        case S2C_GENERIC_OK: // Phan hoi thanh cong
            printf("\n[SUCCESS] %s\n", get_json_string(json, "message"));
            break;

        case S2C_LOGIN_SUCCESS: {
            current_role = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "role"));
            printf("\n[SUCCESS] Dang nhap thanh cong với vai tro: %s!\n", 
                (current_role == ROLE_ADMIN) ? "ADMIN" : 
                (current_role == ROLE_AUCTIONEER) ? "AUCTIONEER" : "BIDDER");
            
            // Hien thi huong dan lenh theo vai tro
            printf("Cac lenh hop le cho ban:\n");
            if (current_role == ROLE_ADMIN) {
                printf(" -> delete <room_id> <item_idx>, list, join <id>, leave, search <key>, history\n");
            } else if (current_role == ROLE_AUCTIONEER) {
                printf(" -> create <title> <p> <buy>, additem <room> <title> <p> <buy>, delete, list, join, leave, search, history\n");
            } else {
                printf(" -> list, join <id>, leave, bid <price>, buynow, search <key>, history\n");
            }
            break;
        }

        case S2C_ROOM_LIST:
            printf("\n======= DANH SACH PHONG & HANG CHO =======\n");
            cJSON *rooms_json = cJSON_GetObjectItem(json, "rooms");
            cJSON *r;
            cJSON_ArrayForEach(r, rooms_json) {
                int id = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "id"));
                int cur_p = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "current_price"));
                int cur_idx = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "current_idx"));
                char *owner = cJSON_GetStringValue(cJSON_GetObjectItem(r, "owner"));
                
                printf("\n[PHONG #%d] - Chu phong: %s - Gia hien tai: %d\n", id, owner ? owner : "Unknown", cur_p);
                printf("  Danh sach hang cho:\n");
                
                cJSON *queue = cJSON_GetObjectItem(r, "queue");
                cJSON *it;
                int count = 0;
                cJSON_ArrayForEach(it, queue) {
                    // Xu ly trang thai vat pham
                    char *status = (count == cur_idx) ? "(*) DANG DAU GIA" : (count < cur_idx ? "[Da xong]" : "[Dang cho]");
                    printf("    %d. %-15s | Gia BD: %-6d | Mua ngay: %-6d %s\n", 
                           count + 1,
                           cJSON_GetStringValue(cJSON_GetObjectItem(it, "title")),
                           (int)cJSON_GetNumberValue(cJSON_GetObjectItem(it, "start_price")),
                           (int)cJSON_GetNumberValue(cJSON_GetObjectItem(it, "buy_now")),
                           status);
                    count++;
                }
            }
            printf("\n===========================================\n");
            break;

        case S2C_NEW_ITEM_PENDING: // Thong bao vat pham moi sap dau gia
            printf("\n>>> [VAT PHAM MOI] Bat dau dau gia: %s (Gia khoi diem: %d)\n",
                   get_json_string(json, "title"),
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "start_price")));
            break;

        case S2C_NEW_BID:
            printf("\n>>> [BID] %s vua dat gia: %d\n", 
                   get_json_string(json, "bidder"), 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "current_price")));
            break;

        case S2C_AUCTION_ENDED:
             printf("\n>>> [KET THUC] Phien dau gia vat pham '%s' da ket thuc!\n", get_json_string(json, "item_title"));
             printf("    Nguoi thang: %s - Gia cuoi: %d\n", 
                   get_json_string(json, "winner") ? get_json_string(json, "winner") : "None", 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "final_price")));
            break;

        case S2C_TIME_ALERT:
            printf("\n>>> [THONG BAO] Phien dau gia con %d giay nua!\n", 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "time_left")));
            break;

        case S2C_GENERIC_ERROR:
             printf("\n[ERROR] %s\n", get_json_string(json, "message"));
             break;

        case S2C_SEARCH_RESULT:
            printf("\n--- KET QUA TIM KIEM ---\n");
            cJSON *res_arr = cJSON_GetObjectItem(json, "results");
            cJSON *res;
            cJSON_ArrayForEach(res, res_arr) {
                printf("Phong #%d: %s | Gia: %d | [%s]\n", 
                    (int)cJSON_GetNumberValue(cJSON_GetObjectItem(res, "room_id")),
                    cJSON_GetStringValue(cJSON_GetObjectItem(res, "title")),
                    (int)cJSON_GetNumberValue(cJSON_GetObjectItem(res, "start_price")),
                    cJSON_GetStringValue(cJSON_GetObjectItem(res, "status")));
            }
            break;

        case S2C_HISTORY_LIST:
            // Phan loai tieu de hien thi theo vai tro hien tai
            if (current_role == ROLE_ADMIN) printf("\n--- NHAT KY GIAO DICH TOAN HE THONG (ADMIN) ---\n");
            else if (current_role == ROLE_AUCTIONEER) printf("\n--- LICH SU CAC MAT HANG BAN DA BAN ---\n");
            else printf("\n--- LICH SU CAC PHIEN BAN DA THANG ---\n");

            cJSON *h_arr = cJSON_GetObjectItem(json, "history");
            cJSON *h;
            cJSON_ArrayForEach(h, h_arr) {
                time_t t = (time_t)cJSON_GetNumberValue(cJSON_GetObjectItem(h, "time"));
                char *time_s = ctime(&t); time_s[strlen(time_s)-1] = '\0';
                char *winner_str = cJSON_GetStringValue(cJSON_GetObjectItem(h, "winner"));
                printf("- [%s] Vat pham: %-15s | Gia: %-8d | Thang: %s\n", 
                    time_s,
                    cJSON_GetStringValue(cJSON_GetObjectItem(h, "item")),
                    (int)cJSON_GetNumberValue(cJSON_GetObjectItem(h, "price")),
                    winner_str ? winner_str : "None");
            }
            break;
        
        case S2C_JOIN_ROOM_SUCCESS: // Xu ly tham gia phong thanh cong
            printf("\n[SUCCESS] Ban da vao phong #%d thanh cong!\n", 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "room_id")));
            break;

        default:
            printf("SERVER: %s\n", json_str);
    }
    cJSON_Delete(json);
    printf("YOU> "); 
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    int max_fd;
    char stdin_buf[BUFFER_SIZE];
    char *server_ip = "127.0.0.1";

    if (argc > 1) server_ip = argv[1];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return 1;
    }

    printf("=== AUCTION CLIENT (C VERSION) ===\n");
    printf("Chua dang nhap. Hay su dung: register <u] <p> <role] hoac login <u] <p>\n");
    printf("YOU> ");
    fflush(stdout);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_fd = sockfd;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            break;
        }

        // --- Nhan tu server ---
        if (FD_ISSET(sockfd, &read_fds)) {
            char *payload = NULL;
            int status = receive_message(sockfd, &payload); // Su dung framing de nhan goi tin
            if (status == 0) {
                print_server_response(payload);
                free(payload);
            } else {
                printf("\nMat ket noi Server!\n");
                break;
            }
        }

        // --- Nhan tu stdin---
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(stdin_buf, BUFFER_SIZE, stdin) != NULL) {
                trim(stdin_buf);
                if (strlen(stdin_buf) > 0) {
                    char *json_to_send = convert_command_to_json(stdin_buf);
                    if (json_to_send) {
                        send_message(sockfd, json_to_send); // Gui goi tin JSON kem do dai
                        free(json_to_send);
                    } else {
                         printf("YOU> "); fflush(stdout);
                    }
                } else {
                     printf("YOU> "); fflush(stdout);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}