#include "server.h"

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
  int sockfd;
  struct sockaddr_in serv_addr;
  fd_set read_fds;
  int max_fd;
  char stdin_buf[BUFFER_SIZE];
  char *server_ip = "127.0.0.1";

  // Cho phep truyen IP qua tham so: ./clientd 127.0.0.1
  if (argc > 1) {
    server_ip = argv[1];
  }

  // 1.Tao socket voi Server
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

  // Ghi log ra stderr de khong lan vao JSON cua stdout
  fprintf(stderr, "[C-Client] Connected to Server at %s:%d\n", server_ip,
          SERVER_PORT);

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);       // Kênh 1: Nghe tu Server
    FD_SET(STDIN_FILENO, &read_fds); // Kênh 2: Nghe tu NodeJS (qua Stdin)
    max_fd = sockfd;

    // Cho du lieu tu 1 trong 2 kenh
    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
      perror("Select error");
      break;
    }

    // TH1: Server gui tin ve
    if (FD_ISSET(sockfd, &read_fds)) {
      char *payload = NULL;
      // Ham receive_message trong framing.c
      int status = receive_message(sockfd, &payload);

      if (status == 0) {
        // IN RA STDOUT -> NodeJS doc
        printf("%s\n", payload);
        fflush(stdout);
        free(payload);
      } else {
        fprintf(stderr, "[C-Client] Server disconnected.\n");
        break;
      }
    }

    // TH2: NodeJS gui xuong
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      // Doc 1 dong JSON tu NodeJS
      if (fgets(stdin_buf, BUFFER_SIZE, stdin) != NULL) {
        stdin_buf[strcspn(stdin_buf, "\n")] = 0;

        if (strlen(stdin_buf) > 0) {
          // Gui toi Server
          send_message(sockfd, stdin_buf);
        }
      } else {
        fprintf(stderr, "[C-Client] Stdin closed (NodeJS exited).\n");
        break;
      }
    }
  }

  close(sockfd);
  return 0;
}