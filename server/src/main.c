#include "server.h"
#include "state.h"
#include "handlers.h"
#include "db_manager.h" 
#include "user_manager.h"
#include "auction_engine.h"

UserState users[MAX_USERS];
RoomState rooms[MAX_ROOMS];

int main() {
  int listen_fd, new_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;

  fd_set master_set, read_fds;
  int fd_max;

  memset(users, 0, sizeof(users));
  memset(rooms, 0, sizeof(rooms));

  // Khoi tao Database
  if (db_init("auction.db") != SQLITE_OK) {
      fprintf(stderr, "[Critical] Failed to initialize Database\n");
      exit(EXIT_FAILURE);
  }

  // 1. Khoi tao socket
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
      -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);
  memset(&(server_addr.sin_zero), '\0', 8);

  // 2. Bind & Listen
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
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

    struct timeval tv;
    tv.tv_sec = 1; // Cho toi da 1s
    tv.tv_usec = 0;
    int activity = select(fd_max + 1, &read_fds, NULL, NULL, &tv);
    if (activity == -1) {
      perror("select");
      break;
    }

    // Goi logic kiem tra thoi gian dau gia tu module auction_engine
    check_auctions(); 

    if (activity == 0) {
      continue;
    }

    for (int i = 0; i <= fd_max; i++) {
      if (FD_ISSET(i, &read_fds)) {

        if (i == listen_fd) {
          // Chap nhan ket noi moi
          client_len = sizeof(client_addr);
          new_fd =
              accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

          if (new_fd == -1) {
            perror("accept");
          } else {
            printf("[Connect] New connection from %s on fd %d\n",
                   inet_ntoa(client_addr.sin_addr), new_fd);
            FD_SET(new_fd, &master_set);
            if (new_fd > fd_max)
              fd_max = new_fd;

            // Khoi tao trang thai nguoi dung tu module user_manager
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
              // Su dung bo dieu phoi (Router) tu module handlers de xu ly tin nhan
              response = handle_client_message(i, json);
              cJSON_Delete(json);
            } else {
              response = create_error_response(ERR_INVALID_MESSAGE,
                                               "Invalid JSON format");
            }
            if (response) {
              send_message(i, response);
              printf("[Sent fd %d]: %s\n", i, response);
              free(response);
            }
            free(payload);
          } else {
            printf("[Disconnect] Client fd %d disconnected\n", i);
            // Don dep trang thai nguoi dung tu module user_manager
            clear_user(i); 
            close(i);
            FD_CLR(i, &master_set);
          }
        }
      }
    }
  }
  // Dong database
  db_close();
  return 0;
}