#include "server.h"

int main() {
    int listen_fd, new_fd; 
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    
    fd_set master_set, read_fds;
    int fd_max; 
	
	// 1.Khoi tao socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
	
	// REUSEADDR de khoi dong lai server ngay lap tuc
    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(SERVER_PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

	// 2.Bind du lieu
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // 3.Lang nghe ket noi
    if (listen(listen_fd, MAX_BACKLOG) == -1) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // 4.Chuan bi select
    FD_ZERO(&master_set); 
    FD_ZERO(&read_fds);

    FD_SET(listen_fd, &master_set); 
    fd_max = listen_fd;        

    printf("Server is running on %d... PORT\n", SERVER_PORT);

    // 5.Server xu ly logic
    while (1) {
        read_fds = master_set; 
        
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
		
		// Quet lan luot qua cac socket
        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) {
                
                if (i == listen_fd) {
                    // Client chua ket noi, yeu cau ket noi
                    client_len = sizeof(client_addr);
                    new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        printf("New request from %s on %d\n", inet_ntoa(client_addr.sin_addr), new_fd);
                        FD_SET(new_fd, &master_set); 
                        if (new_fd > fd_max) {
                            fd_max = new_fd; 
                        }
                    }
                } else {
                    // Client da ket noi, gui du lieu
                    char *payload = NULL;
                    
                    int recv_status = receive_message(i, &payload);

                    if (recv_status == 0) {
                        printf("Recept from [fd %d]: %s\n", i, payload);
                        // Se thay the khoi duoi bang "dispatch_message(i, payload);"
                        
                        // Gui tin tam thoi S2C_GENERIC_OK
                        char *ok_response = create_ok_response();
                        if (ok_response) {
                            send_message(i, ok_response);
                            free(ok_response);
                        }
                        
                        free(payload); // Very important

                    } else if (recv_status == 1) {
                        // Client ngat ket noi
                        printf("Client [fd %d] is unconnected.\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        // Loi khac
                        perror("receive_message error");
                        close(i);
                        FD_CLR(i, &master_set);
                    }
                }
            }
        }
    }

    return 0;
}
