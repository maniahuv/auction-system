#include "server.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h> // Thêm thư viện này để dùng gethostbyname

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    char stdin_buf[BUFFER_SIZE];

    // 1. Xử lý tham số dòng lệnh (Host và Port từ Python truyền sang)
    char *server_host = (argc > 1) ? argv[1] : "127.0.0.1";
    int server_port = (argc > 2) ? atoi(argv[2]) : 8080;

    // 2. Khởi tạo socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return 1;

    // 3. Giải mã Tên miền (DNS) - Quan trọng để chạy ngrok
    struct hostent *server = gethostbyname(server_host);
    if (server == NULL) {
        fprintf(stderr, "[Error] Không thể giải mã tên miền: %s\n", server_host);
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    // Copy địa chỉ IP đã giải mã vào cấu trúc serv_addr
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // Sử dụng Port động từ tham số argv[2]
    serv_addr.sin_port = htons(server_port);

    // 4. Kết nối tới Server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "[Error] Connection Failed to %s:%d\n", server_host, server_port);
        return 1;
    }

    // Gửi log lỗi về stderr để Python hiển thị trong console debug
    fprintf(stderr, "[Daemon] Connected successfully to %s:%d\n", server_host, server_port);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        // Nhận dữ liệu từ Server C -> In ra stdout cho Python đọc
        if (FD_ISSET(sockfd, &read_fds)) {
            char *payload = NULL;
            if (receive_message(sockfd, &payload) == 0) {
                printf("%s\n", payload); 
                fflush(stdout);
                free(payload);
            } else break;
        }

        // Nhận dữ liệu từ Python (stdin) -> Gửi sang Server C
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(stdin_buf, BUFFER_SIZE, stdin)) {
                stdin_buf[strcspn(stdin_buf, "\n")] = 0;
                if (strlen(stdin_buf) > 0) {
                    send_message(sockfd, stdin_buf); 
                }
            }
        }
    }
    
    close(sockfd);
    fprintf(stderr, "[Daemon] Disconnected.\n");
    return 0;
}