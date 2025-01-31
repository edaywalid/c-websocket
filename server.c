#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * start_server()
 *
 * - Create a new server socket and bind it to the specified port
 *   and start listening for incoming connections.
 *   The server socket is returned if successful.
 *   If an error occurs, the program will exit with EXIT_FAILURE.
 *   The server socket must be closed with close() when no longer needed.
 *   The server socket is used to accept incoming client connections.
 *   The server socket is a TCP socket.
 *   The server socket is bound to INADDR_ANY to listen on all network
 * interfaces The server socket is bound to the specified port. The server
 * socket is set to listen for a maximum of MAX_CLIENTS connections.
 */

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

/*
 * accept_client()
 * - Accept a new client connection on the specified server socket
 *   and return the client socket file descriptor.
 *   If an error occurs, the program will exit with EXIT_FAILURE.
 *
 *   Parameters:
 *     - server_fd: The server socket file descriptor
 */

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
