#ifndef SERVER_H
#define SERVER_H

// Cac thu vien C chuan 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <errno.h>  

// Các thu vien Networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <sys/select.h>


// Thu vien duoc viet tu folder common
#include "protocol.h"
#include "framing.h"
#include "json_util.h"

// Cau hinh server
#define SERVER_PORT 8080 // Cong server chay
#define MAX_BACKLOG 10   // So luong ket noi toi da hang cho
#define MAX_CLIENTS 30   // So luong client toi da

#endif
