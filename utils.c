#include "utils.h"
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>

/*
 * compute_websocket_accept_key()
 * - Compute the WebSocket accept key based on the client key
 *   as per the WebSocket protocol RFC 6455
 *   https://tools.ietf.org/html/rfc6455#section-1.3
 *
 *  The client key is concatenated with the WebSocket GUID
 *  and then SHA1 hashed. The resulting hash is then base64
 *  encoded to get the accept key.
 *
 *  Parameters:
 *  client_key: The client key from the WebSocket handshake request
 *  accept_key: The buffer to store the computed accept key
 *
 */

void compute_websocket_accept_key(const char *client_key, char *accept_key) {
  char key_concat[256];
  unsigned char sha1_hash[SHA_DIGEST_LENGTH];

  snprintf(key_concat, sizeof(key_concat), "%s%s", client_key, WS_GUID);

  SHA1((unsigned char *)key_concat, strlen(key_concat), sha1_hash);

  BIO *bio, *b64;
  BUF_MEM *buffer_ptr;

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

  BIO_write(bio, sha1_hash, SHA_DIGEST_LENGTH);
  BIO_flush(bio);
  BIO_get_mem_ptr(bio, &buffer_ptr);
  BIO_set_close(bio, BIO_NOCLOSE);
  BIO_free_all(bio);

  memcpy(accept_key, buffer_ptr->data, buffer_ptr->length);
  accept_key[buffer_ptr->length] = '\0';
  BUF_MEM_free(buffer_ptr);
}
