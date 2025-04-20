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

int ws_frame_write(int client_fd, ws_frame_t *frame) {
    if (!frame) return -1;
    
    // Prepare header
    uint8_t header[14]; // Max header size (2 + 8 + 4)
    size_t header_size = 2;
    
    // First byte: FIN bit + opcode
    header[0] = (frame->fin ? 0x80 : 0x00) | (frame->opcode & 0x0F);
    
    // Second byte: MASK bit + payload length
    if (frame->payload_length < 126) {
        header[1] = (frame->masked ? 0x80 : 0x00) | (frame->payload_length & 0x7F);
    } else if (frame->payload_length <= 0xFFFF) {
        header[1] = (frame->masked ? 0x80 : 0x00) | 126;
        uint16_t len16 = htons((uint16_t)frame->payload_length);
        memcpy(&header[2], &len16, 2);
        header_size += 2;
    } else {
        header[1] = (frame->masked ? 0x80 : 0x00) | 127;
        uint64_t len64 = htobe64(frame->payload_length);
        memcpy(&header[2], &len64, 8);
        header_size += 8;
    }
    
    // Add masking key if needed
    if (frame->masked) {
        memcpy(&header[header_size], frame->masking_key, 4);
        header_size += 4;
    }
    
    // Write header
    if (write(client_fd, header, header_size) != (ssize_t)header_size) {
        perror("Failed to write WebSocket frame header");
        return -1;
    }
    
    // Write payload
    if (frame->payload_length > 0 && frame->payload) {
        uint8_t *payload_to_send = frame->payload;
        uint8_t *masked_payload = NULL;
        
        // If masked, create a copy of the payload and mask it
        if (frame->masked) {
            masked_payload = (uint8_t *)malloc(frame->payload_length);
            if (!masked_payload) {
                perror("Failed to allocate memory for masked payload");
                return -1;
            }
            memcpy(masked_payload, frame->payload, frame->payload_length);
            for (uint64_t i = 0; i < frame->payload_length; i++) {
                masked_payload[i] ^= frame->masking_key[i % 4];
            }
            payload_to_send = masked_payload;
        }
        
        // Write payload
        if (write(client_fd, payload_to_send, frame->payload_length) != 
                (ssize_t)frame->payload_length) {
            perror("Failed to write WebSocket frame payload");
            if (masked_payload) free(masked_payload);
            return -1;
        }
        
        if (masked_payload) free(masked_payload);
    }
    
    return 0;
}

void ws_frame_free(ws_frame_t *frame) {
    if (frame && frame->payload) {
        free(frame->payload);
        frame->payload = NULL;
    }
}

ws_frame_t* ws_frame_create(bool fin, uint8_t opcode, 
                           const uint8_t *payload, 
                           uint64_t payload_length, 
                           bool masked) {
    ws_frame_t *frame = (ws_frame_t *)malloc(sizeof(ws_frame_t));
    if (!frame) {
        perror("Failed to allocate memory for WebSocket frame");
        return NULL;
    }
    
    frame->fin = fin;
    frame->opcode = opcode;
    frame->masked = masked;
    frame->payload_length = payload_length;
    
    // Generate random masking key if needed
    if (masked) {
        for (int i = 0; i < 4; i++) {
            frame->masking_key[i] = rand() & 0xFF;
        }
    }
    
    // Copy payload
    if (payload_length > 0 && payload) {
        frame->payload = (uint8_t *)malloc(payload_length);
        if (!frame->payload) {
            perror("Failed to allocate memory for payload");
            free(frame);
            return NULL;
        }
        memcpy(frame->payload, payload, payload_length);
    } else {
        frame->payload = NULL;
    }
    
    return frame;
} 