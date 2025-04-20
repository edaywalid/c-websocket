#ifndef WEBSOCKET_FRAME_H
#define WEBSOCKET_FRAME_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/* WebSocket frame flags and opcodes */
#define WS_FIN      0x80
#define WS_MASK     0x80

/* OpCodes */
#define WS_CONTINUATION  0x0
#define WS_TEXT          0x1
#define WS_BINARY        0x2
#define WS_CLOSE         0x8
#define WS_PING          0x9
#define WS_PONG          0xA

typedef struct {
    uint8_t fin;
    uint8_t opcode;
    uint8_t masked;
    uint64_t payload_length;
    uint8_t masking_key[4];
    uint8_t *payload;
} ws_frame_t;

/**
 * Read a WebSocket frame from the client socket
 * 
 * @param client_fd The client socket file descriptor
 * @param frame Pointer to store the frame
 * @return 0 on success, -1 on error
 */
int ws_frame_read(int client_fd, ws_frame_t *frame);

/**
 * Write a WebSocket frame to the client socket
 * 
 * @param client_fd The client socket file descriptor
 * @param frame Pointer to the frame to write
 * @return 0 on success, -1 on error
 */
int ws_frame_write(int client_fd, ws_frame_t *frame);

/**
 * Free resources allocated for a WebSocket frame
 * 
 * @param frame Pointer to the frame to free
 */
void ws_frame_free(ws_frame_t *frame);

/**
 * Create a new WebSocket frame with the given parameters
 * 
 * @param fin Is this the final frame in a sequence?
 * @param opcode The frame opcode
 * @param payload The payload data
 * @param payload_length Length of the payload
 * @param masked Should the frame be masked?
 * @return Pointer to new frame or NULL on error
 */
ws_frame_t* ws_frame_create(bool fin, uint8_t opcode, 
                            const uint8_t *payload, 
                            uint64_t payload_length, 
                            bool masked);

#endif /* WEBSOCKET_FRAME_H */ 