#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include "bs.h"

#include "dyn_array.h"
#include <arpa/inet.h>
#include <assert.h>
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/uio.h>
#define MAX_HTTP_HEADERS 64

#define WS_DEBUG(...)                                                          \
  do {                                                                         \
    fprintf(stderr, "DEBUG: %s(): ", __func__);                                \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#define WS_WARN(...)                                                           \
  do {                                                                         \
    fprintf(stderr, "WARN: %s(): ", __func__);                                 \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#define WS_ERROR(...)                                                          \
  do {                                                                         \
    fprintf(stderr, "ERROR: %s(): ", __func__);                                \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)

da_new(Better_String);
int ws_establish_tcp_connection(const char *host, uint16_t port);
int ws_send_http_upgdate_request(int tcp_socket_fd,
                                 Dyn_Array_Better_String request);
void ws_do_http_upgrade(int tcp_socket_fd);
#endif
