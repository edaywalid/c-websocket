#include "websocket_frame.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

/**
 * Unmask the payload of a WebSocket frame
 * 
 * @param frame The frame to unmask
 */
static void ws_frame_unmask(ws_frame_t *frame) {
    if (frame->masked && frame->payload) {
        for (uint64_t i = 0; i < frame->payload_length; i++) {
            frame->payload[i] ^= frame->masking_key[i % 4];
        }
    }
}

