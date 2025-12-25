#include "server.h" // Đã bao gồm cJSON.h, framing.h, protocol.h, json_util.h...
#include <ctype.h>

#define BUFFER_SIZE 4096

// Hàm xóa khoảng trắng thừa đầu/cuối (trim)
void trim(char *s) {
    char *p = s;
    int l = strlen(p);
    while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
    while(*p && isspace(*p)) ++p, --l;
    memmove(s, p, l + 1);
}

// Hàm chuyển đổi lệnh người dùng nhập -> Chuỗi JSON gửi Server
char* convert_command_to_json(char *input) {
    char *cmd = strtok(input, " ");
    if (!cmd) return NULL;

    cJSON *req = cJSON_CreateObject();
    
    // --- LỆNH: REGISTER user pass ---
    if (strcmp(cmd, "register") == 0) {
        char *user = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        if (user && pass) {
            cJSON_AddNumberToObject(req, "type", C2S_REGISTER); // 101
            cJSON_AddStringToObject(req, "user", user);
            cJSON_AddStringToObject(req, "pass", pass);
        } else {
            printf(">> Sai cu phap! Dung: register <user> <pass>\n");
            cJSON_Delete(req); return NULL;
        }
    }
    // --- LỆNH: LOGIN user pass ---
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
    // --- LỆNH: CREATE title start_price buy_now_price ---
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
    // --- LỆNH: JOIN room_id ---
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
    // --- LỆNH: BID price ---
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
    // --- LỆNH: BUYNOW ---
    else if (strcmp(cmd, "buynow") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_BUY_NOW); // 402
    }
    // --- LỆNH: LIST ---
    else if (strcmp(cmd, "list") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_LIST_ROOMS); // 201
    }
    // --- HỖ TRỢ NHẬP JSON THÔ (Cho debug) ---
    else if (cmd[0] == '{') {
        cJSON_Delete(req);
        return strdup(input);
    }
    else {
        printf(">> Lenh khong hop le! (register, login, create, join, bid, buynow, list)\n");
        cJSON_Delete(req);
        return NULL;
    }

    char *json_str = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);
    return json_str;
}

// Hàm in phản hồi từ Server cho đẹp
void print_server_response(char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        printf("RAW: %s\n", json_str);
        return;
    }

    int type = 0;
    get_json_int(json, "type", &type);

    switch (type) {
        case S2C_GENERIC_OK: // Phản hồi thành công chung (dùng cho register, buynow...)
            printf("\n[SUCCESS] Thao tac thanh cong: %s\n", get_json_string(json, "message"));
            break;
        case S2C_LOGIN_SUCCESS:
            printf("\n[SUCCESS] Dang nhap thanh cong!\n");
            break;
        case S2C_ROOM_LIST:
            printf("\n--- DANH SACH PHONG ---\n");
            cJSON *rooms = cJSON_GetObjectItem(json, "rooms");
            cJSON *r;
            cJSON_ArrayForEach(r, rooms) {
                int id = 0, price = 0;
                char *title = NULL;
                get_json_int(r, "id", &id);
                get_json_int(r, "price", &price);
                title = (char*)get_json_string(r, "title");
                printf("#%d - %s (Gia hien tai: %d)\n", id, title, price);
            }
            printf("-----------------------\n");
            break;
        case S2C_NEW_BID:
            printf("\n>>> [BID] %s vua dat gia: %d\n", 
                   get_json_string(json, "bidder"), 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "current_price")));
            break;
        case S2C_AUCTION_ENDED:
             printf("\n>>> [KET THUC] Nguoi thang: %s - Gia cuoi: %d\n", 
                   get_json_string(json, "winner"), 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "final_price")));
            break;
        case S2C_GENERIC_ERROR:
             printf("\n[ERROR] %s\n", get_json_string(json, "message"));
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
    printf("Commands: register, login, create, list, join, bid, buynow\n"); // Cập nhật danh sách lệnh
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

        // --- NHẬN TỪ SERVER ---
        if (FD_ISSET(sockfd, &read_fds)) {
            char *payload = NULL;
            int status = receive_message(sockfd, &payload); // Sử dụng framing để nhận đủ gói
            if (status == 0) {
                print_server_response(payload);
                free(payload);
            } else {
                printf("\nMat ket noi Server!\n");
                break;
            }
        }

        // --- NHẬN TỪ BÀN PHÍM ---
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(stdin_buf, BUFFER_SIZE, stdin) != NULL) {
                trim(stdin_buf);
                if (strlen(stdin_buf) > 0) {
                    char *json_to_send = convert_command_to_json(stdin_buf);
                    if (json_to_send) {
                        send_message(sockfd, json_to_send); // Gửi gói tin JSON kèm độ dài
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