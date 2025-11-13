#ifndef FRAMING_H
#define FRAMING_H

// Gui tin nhan qua socket
// message := [4byte][payload]
// [4byte]: Do dai cua payload
int send_message(int sockfd, const char *payload);

// Nhan 1 hay nhieu tin nhan tu socket
// Tu cap phat bo nho
int receive_message(int sockfd, char **payload);

#endif
