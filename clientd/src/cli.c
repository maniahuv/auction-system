#include "server.h" // Đã bao gồm cJSON.h, framing.h, protocol.h, json_util.h...
#include <ctype.h>
#include "state.h"

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
    // Khởi tạo phòng đấu giá với vật phẩm đầu tiên
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
    // --- LỆNH: ADDITEM room_id title start_price buy_now_price ---
    // Thêm vật phẩm vào hàng chờ của một phòng cụ thể (Queue Management)
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
    // Lệnh xóa vật phẩm khỏi hàng chờ của phòng hiện tại
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
    // --- LỆNH: LEAVE ---
    else if (strcmp(cmd, "leave") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_LEAVE_ROOM); // 204
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
    // --- LỆNH: HISTORY ---
    else if (strcmp(cmd, "history") == 0) {
        cJSON_AddNumberToObject(req, "type", C2S_GET_HISTORY); // 501
    }
    // --- HỖ TRỢ NHẬP JSON THÔ (Cho debug) ---
    else if (cmd[0] == '{') {
        cJSON_Delete(req);
        return strdup(input);
    }
    else {
        printf(">> Lenh khong hop le! (register, login, create, additem, join, leave, bid, buynow, delete, search, history, list)\n");
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
        case S2C_GENERIC_OK: // Phản hồi thành công chung (register, buynow, additem...)
            printf("\n[SUCCESS] %s\n", get_json_string(json, "message"));
            break;

        case S2C_LOGIN_SUCCESS:
            printf("\n[SUCCESS] Dang nhap thanh cong!\n");
            break;

        case S2C_ROOM_LIST:
            printf("\n======= DANH SACH PHONG & HANG CHO =======\n");
            cJSON *rooms_json = cJSON_GetObjectItem(json, "rooms");
            cJSON *r;
            cJSON_ArrayForEach(r, rooms_json) {
                int id = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "id"));
                int cur_p = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "current_price"));
                int cur_idx = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(r, "current_idx"));
                
                printf("\n[PHONG #%d] - Gia hien tai: %d\n", id, cur_p);
                printf("  Danh sach hang cho:\n");
                
                cJSON *queue = cJSON_GetObjectItem(r, "queue");
                cJSON *it;
                int count = 0;
                cJSON_ArrayForEach(it, queue) {
                    // Xử lý trạng thái hiển thị của từng món hàng
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

        case S2C_NEW_ITEM_PENDING: // Thông báo khi món hàng tiếp theo lên sàn
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
             printf("\n>>> [KET THUC] Vat pham '%s' da co chu!\n", get_json_string(json, "item_title"));
             printf("    Nguoi thang: %s - Gia cuoi: %d\n", 
                   get_json_string(json, "winner"), 
                   (int)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "final_price")));
            break;

        case S2C_TIME_ALERT:
            printf("\n>>> [CANH BAO] Phien dau gia con %d giay nua!\n", 
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
            printf("\n--- LICH SU THANG DAU GIA CUA BAN ---\n");
            cJSON *h_arr = cJSON_GetObjectItem(json, "history");
            cJSON *h;
            cJSON_ArrayForEach(h, h_arr) {
                time_t t = (time_t)cJSON_GetNumberValue(cJSON_GetObjectItem(h, "time"));
                printf("- %s | Gia: %d | Ngay: %s", 
                    cJSON_GetStringValue(cJSON_GetObjectItem(h, "item")),
                    (int)cJSON_GetNumberValue(cJSON_GetObjectItem(h, "price")),
                    ctime(&t));
            }
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
    printf("Commands: register, login, create, additem, list, join, leave, bid, buynow, delete, search, history\n"); // Cập nhật danh sách lệnh
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

