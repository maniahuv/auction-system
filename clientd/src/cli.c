#include "server.h"
#include "client_state.h"
#include "client_utils.h"
#include "cmd_parser.h"
#include "ui_render.h"
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    char stdin_buf[BUFFER_SIZE];
    char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket error"); return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect failed"); return 1;
    }

    printf("=== AUCTION CLIENT ===\n");
    printf("Trang thai: Chua dang nhap.\n");
    printf("Goi y: Hay su dung 'register <u] <p> <role]' hoac 'login <u] <p>'\n");
    printf("YOU> ");
    fflush(stdout);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(sockfd, &read_fds)) {
            char *payload = NULL;
            if (receive_message(sockfd, &payload) == 0) {
                print_server_response(payload);
                free(payload);
            } else {
                printf("\nMat ket noi Server!\n"); break;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(stdin_buf, BUFFER_SIZE, stdin)) {
                trim(stdin_buf);
                if (strlen(stdin_buf) > 0) {
                    char *json = convert_command_to_json(stdin_buf);
                    if (json) {
                        send_message(sockfd, json);
                        free(json);
                    } else { printf("YOU> "); fflush(stdout); }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}