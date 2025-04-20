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
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    int server_fd = start_server();
    
    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    
    int max_fd = server_fd;
    
    printf("WebSocket server started. Press Ctrl+C to stop.\n");
    
    while (running) {
        read_fds = master_set;
        
        struct timeval timeout = {1, 0};
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("Select error");
            break;
        }
        
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept_client(server_fd);
            if (client_fd > 0) {
                FD_SET(client_fd, &master_set);
                if (client_fd > max_fd) {
                    max_fd = client_fd;
                }
                printf("Client added to active set\n");
            }
        }
        
        for (int fd = 0; fd <= max_fd; fd++) {
            if (fd == server_fd || !FD_ISSET(fd, &read_fds)) {
                continue;
            }
            
            char peek_buf[1];
            if (recv(fd, peek_buf, 1, MSG_PEEK) <= 0) {
                printf("Client %d disconnected\n", fd);
                close(fd);
                FD_CLR(fd, &master_set);
            }
            else if (peek_buf[0] == 'G') {
                if (handle_websocket_handshake(fd) != 0) {
                    printf("WebSocket handshake failed for client %d\n", fd);
                    close(fd);
                    FD_CLR(fd, &master_set);
                } else {
                    printf("WebSocket handshake successful for client %d\n", fd);
                }
            }
            else {
                handle_client_data(fd);
            }
        }
    }
    
    printf("Shutting down server...\n");
    
    for (int fd = 0; fd <= max_fd; fd++) {
        if (fd != server_fd && FD_ISSET(fd, &master_set)) {
            close(fd);
        }
    }
    
    close(server_fd);
    return 0;
}
