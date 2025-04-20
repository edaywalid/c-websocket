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

int ws_frame_read(int client_fd, ws_frame_t *frame) {
    if (!frame) return -1;
    
    uint8_t header[2];
    if (read(client_fd, header, 2) != 2) {
        perror("Failed to read WebSocket frame header");
        return -1;
    }
    
    // Parse first byte
    frame->fin = (header[0] & 0x80) != 0;
    frame->opcode = header[0] & 0x0F;
    
    // Parse second byte
    frame->masked = (header[1] & 0x80) != 0;
    uint8_t payload_len = header[1] & 0x7F;
    
    // Get extended payload length
    if (payload_len == 126) {
        uint16_t len16;
        if (read(client_fd, &len16, 2) != 2) {
            perror("Failed to read extended payload length (16-bit)");
            return -1;
        }
        frame->payload_length = ntohs(len16);
    } else if (payload_len == 127) {
        uint64_t len64;
        if (read(client_fd, &len64, 8) != 8) {
            perror("Failed to read extended payload length (64-bit)");
            return -1;
        }
        frame->payload_length = be64toh(len64);
    } else {
        frame->payload_length = payload_len;
    }
    
    // Read masking key if present
    if (frame->masked) {
        if (read(client_fd, frame->masking_key, 4) != 4) {
            perror("Failed to read masking key");
            return -1;
        }
    }
    
    // Read payload
    if (frame->payload_length > 0) {
        frame->payload = (uint8_t *)malloc(frame->payload_length);
        if (!frame->payload) {
            perror("Failed to allocate memory for payload");
            return -1;
        }
        
        size_t bytes_read = 0;
        while (bytes_read < frame->payload_length) {
            ssize_t result = read(client_fd, 
                                 frame->payload + bytes_read, 
                                 frame->payload_length - bytes_read);
            if (result <= 0) {
                perror("Failed to read payload");
                free(frame->payload);
                frame->payload = NULL;
                return -1;
            }
            bytes_read += result;
        }
        
        // Unmask payload if masked
        if (frame->masked) {
            ws_frame_unmask(frame);
        }
    } else {
        frame->payload = NULL;
    }
    
    return 0;
}
