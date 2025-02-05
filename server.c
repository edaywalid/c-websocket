#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int start_server() {
  int server_fd;
  struct sockaddr_in server_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, MAX_CLIENTS) < 0) {
    perror("Listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d...\n", PORT);
  return server_fd;
}

int accept_client(int server_fd) {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    perror("Client accept failed");
  } else {
    printf("New client connected!\n");
  }

  return client_fd;
}
