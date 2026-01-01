#include "server.h"
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    char stdin_buf[BUFFER_SIZE];
    char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return 1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return 1;

    // Daemon mode: chi truyen nhan JSON tho qua stdin/stdout
    fprintf(stderr, "[Daemon] Connected to %s\n", server_ip);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(sockfd, &read_fds)) {
            char *payload = NULL;
            if (receive_message(sockfd, &payload) == 0) {
                printf("%s\n", payload); // In JSON tho cho UI doc
                fflush(stdout);
                free(payload);
            } else break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(stdin_buf, BUFFER_SIZE, stdin)) {
                stdin_buf[strcspn(stdin_buf, "\n")] = 0;
                if (strlen(stdin_buf) > 0) {
                    send_message(sockfd, stdin_buf); // Gui JSON tho tu UI
                }
            }
        }
    }
    close(sockfd);
    return 0;
}