#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define PORT 8080

int start_server();
int accept_client(int server_fd);

#endif
