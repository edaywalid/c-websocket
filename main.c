#include "server.h"
#include "websocket.h"
#include "websocket_frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <stdbool.h>
#include <time.h>

#define BUFFER_SIZE 1024

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    printf("Received signal %d, shutting down...\n", sig);
    running = 0;
}

void handle_client_data(int client_fd) {
    ws_frame_t frame;
    
    if (ws_frame_read(client_fd, &frame) == 0) {
        printf("Received WebSocket frame: OpCode=%d, Length=%llu\n", 
               frame.opcode, (unsigned long long)frame.payload_length);
        
        switch (frame.opcode) {
            case WS_TEXT:
                printf("Text message: %.*s\n", (int)frame.payload_length, frame.payload);
                
                ws_frame_t *response = ws_frame_create(true, WS_TEXT, 
                                                     frame.payload, 
                                                     frame.payload_length, 
                                                     false);
                if (response) {
                    ws_frame_write(client_fd, response);
                    free(response);
                }
                break;
                
            case WS_BINARY:
                printf("Received binary data of %llu bytes\n", 
                       (unsigned long long)frame.payload_length);
                break;
                
            case WS_PING:
                printf("Received PING\n");
                ws_frame_t *pong = ws_frame_create(true, WS_PONG, 
                                                 frame.payload, 
                                                 frame.payload_length, 
                                                 false);
                if (pong) {
                    ws_frame_write(client_fd, pong);
                    free(pong);
                }
                break;
                
            case WS_CLOSE:
                printf("Received CLOSE frame\n");
                ws_frame_t *close = ws_frame_create(true, WS_CLOSE, 
                                                  frame.payload, 
                                                  frame.payload_length, 
                                                  false);
                if (close) {
                    ws_frame_write(client_fd, close);
                    free(close);
                }
                return;
                
            default:
                printf("Unsupported OpCode: %d\n", frame.opcode);
                break;
        }
        
        ws_frame_free(&frame);
    } else {
        printf("Error reading WebSocket frame or client disconnected\n");
    }
}

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
