#include "websocket.h"
#include "utils.h"

#define BUFFER_SIZE 1024

/*
 * handle_websocket_handshake()
 * - Handle the WebSocket handshake request from the client
 *   and send the appropriate response to upgrade the connection
 *   to a WebSocket connection.
 *
 *   The client sends a WebSocket handshake request which includes
 *   the Sec-WebSocket-Key header. The server must respond with a
 *   HTTP 101 Switching Protocols response with the Sec-WebSocket-Accept
 *   header which is computed based on the client key.
 *
 *   Parameters:
 *     - client_fd: The file descriptor of the client socket
 *     that sent the WebSocket handshake request
 *
 *   Returns:
 *     - 0 if the WebSocket handshake was successful
 *     - -1 if an error occurred during the handshake
 *     or the request was invalid
 *
 */
int handle_websocket_handshake(int client_fd) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
  if (bytes_read <= 0) {
    perror("Failed to read WebSocket handshake");
    return -1;
  }

  char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
  if (!key_start) {
    printf("Invalid WebSocket request\n");
    return -1;
  }
  key_start += strlen("Sec-WebSocket-Key: ");
  char *key_end = strstr(key_start, "\r\n");
  if (!key_end) {
    printf("Invalid WebSocket key\n");
    return -1;
  }

  char sec_websocket_key[128];
  memset(sec_websocket_key, 0, sizeof(sec_websocket_key));
  strncpy(sec_websocket_key, key_start, key_end - key_start);

  char accept_key[256];
  compute_websocket_accept_key(sec_websocket_key, accept_key);

  char response[BUFFER_SIZE];
  snprintf(response, sizeof(response),
           "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: %s\r\n\r\n",
           accept_key);

  write(client_fd, response, strlen(response));
  printf("WebSocket handshake completed!\n");

  return 0;
}
