#include"framing.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>

// Ham noi bo dam bao gui du byte 
static int send_all(int sockfd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *data = (const char *)buf;

    while (total_sent < len) {
        int n = write(sockfd, data + total_sent, len - total_sent);
        if (n == -1) {
            perror("send_all write");
            return -1;
        }
        total_sent += n;
    }
    return 0;
}

// Gui message
int send_message(int sockfd, const char *payload) {
    if (payload == NULL) {
        return -1;
    }

    uint32_t len = strlen(payload);
    uint32_t net_len = htonl(len); 
	
	// Gui 4byte length truoc
    if (send_all(sockfd, &net_len, sizeof(net_len)) == -1) {
        fprintf(stderr, "Failed to send message length\n");
        return -1;
    }

	// Gui payload sau
    if (send_all(sockfd, payload, len) == -1) {
        fprintf(stderr, "Failed to send message payload\n");
        return -1;
    }

    return 0; 
}

// Ham noi bo dam bao gui du byte 
static int recv_all(int sockfd, void *buf, size_t len) {
    size_t total_recv = 0;
    char *data = (char *)buf;

    while (total_recv < len) {
        int n = read(sockfd, data + total_recv, len - total_recv);
        if (n == -1) {
            perror("recv_all read");
            return -1; 
        }
        if (n == 0) {
            return 1; 
        }
        total_recv += n;
    }
    return 0;
}

// Nhan message
int receive_message(int sockfd, char **payload) {
    uint32_t net_len;
    int recv_status = recv_all(sockfd, &net_len, sizeof(net_len)); //Nhan 4byte length
    
    if (recv_status == -1) {
        fprintf(stderr, "Failed to receive message length\n");
        return -1; 
    }
    if (recv_status == 1) {
        return 1; 
    }

    uint32_t len = ntohl(net_len);

	// Cap phat len + 1 (1 Cho ki tu "0")
    char *buf = (char *)malloc(len + 1);
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for payload\n");
        return -1; 
    }

    // Nhan dung len byte cua payload
    recv_status = recv_all(sockfd, buf, len);
    if (recv_status == -1) {
        fprintf(stderr, "Failed to receive message payload\n");
        free(buf);
        return -1; 
    }
    if (recv_status == 1) {
        fprintf(stderr, "Connection closed while receiving payload\n");
        free(buf);
        return 1; 
    }
    
    buf[len] = '\0';
    *payload = buf;
    
    return 0; 
}
