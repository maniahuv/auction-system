#include "server.h"
#include "state.h" 

UserState users[MAX_USERS];
RoomState rooms[MAX_ROOMS];

char* handle_login(int fd, cJSON *json);
char* handle_bid(int fd, cJSON *json);

// Quan ly ket noi

// Khoi tao slot cho user moi ket noi
void init_user(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == 0) { // Tim slot trung
            users[i].fd = fd;
            memset(users[i].username, 0, 50);
            users[i].is_logged_in = 0;
            users[i].current_room_id = -1;
            printf("[System] Init state for fd %d at slot %d\n", fd, i);
            return;
        }
    }
    printf("[Warning] Server full, cannot init user for fd %d\n", fd);
}

// Xoa khi user ngat ket noi
void clear_user(int fd) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].fd == fd) {
            printf("[System] Clearing state for User '%s' (fd %d)\n", users[i].username, fd);
            users[i].fd = 0;
            users[i].is_logged_in = 0;
            users[i].current_room_id = -1;
            return;
        }
    }
}

int main() {
    int listen_fd, new_fd; 
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    
    fd_set master_set, read_fds;
    int fd_max; 

    memset(users, 0, sizeof(users));
    memset(rooms, 0, sizeof(rooms));
	
	// 1. Khoi tao socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
	
    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(SERVER_PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

	// 2. Bind & Listen
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, MAX_BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 3. Setup Select
    FD_ZERO(&master_set); 
    FD_ZERO(&read_fds);
    FD_SET(listen_fd, &master_set); 
    fd_max = listen_fd;        

    printf("=== AUCTION SERVER STARTED ON PORT %d ===\n", SERVER_PORT);

    // 4. Server Loop
    while (1) {
        read_fds = master_set; 
        
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
		
        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) {
                
                if (i == listen_fd) {
                    // Chap nhan ket noi moi
                    client_len = sizeof(client_addr);
                    new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        printf("[Connect] New connection from %s on fd %d\n", inet_ntoa(client_addr.sin_addr), new_fd);
                        FD_SET(new_fd, &master_set); 
                        if (new_fd > fd_max) fd_max = new_fd; 
                        
                        init_user(new_fd);
                    }
                } else {
                    // Tu client
                    char *payload = NULL;
                    int recv_status = receive_message(i, &payload);

                    if (recv_status == 0) {
                        printf("[Recv fd %d]: %s\n", i, payload);

                        cJSON *json = parse_json(payload);
                        char *response = NULL;

                        if (json) {
                            int type = 0;
                            get_json_int(json, "type", &type);

                            switch (type) {
                                case C2S_LOGIN:
                                    response = handle_login(i, json);
                                    break;
                                case C2S_BID:
                                    response = handle_bid(i, json);
                                    break;
                                default:
                                    response = create_error_response(ERR_UNKNOWN, "Unknown command type");
                                    break;
                            }
                            cJSON_Delete(json);
                        } else {
                            response = create_error_response(ERR_INVALID_MESSAGE, "Invalid JSON format");
                        }

                        if (response) {
                            send_message(i, response);
                            printf("[Sent fd %d]: %s\n", i, response);
                            free(response); 
                        }
                        
                        free(payload); 

                    } else {
                        printf("[Disconnect] Client fd %d disconnected\n", i);
                        clear_user(i);
                        close(i);
                        FD_CLR(i, &master_set);
                    }
                }
            }
        }
    }
    return 0;
}
