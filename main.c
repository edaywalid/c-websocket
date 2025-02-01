#include "server.h"
#include "websocket.h"

int main() {
  int server_fd = start_server();

  while (1) {
    int client_fd = accept_client(server_fd);
    if (client_fd > 0) {
      handle_websocket_handshake(client_fd);
      close(client_fd);
    }
  }

  close(server_fd);
  return 0;
}
