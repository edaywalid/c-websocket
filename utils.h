#ifndef UTILS_H
#define UTILS_H
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

void compute_websocket_accept_key(const char *client_key, char *accept_key);

#endif
