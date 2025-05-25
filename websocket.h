#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__
#include "./libs/bs.h"
#ifdef __linux__
#include "io/linux.h"
#endif
#include "libs/utils.h"
#include <arpa/inet.h>
#include <assert.h>
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifndef opaque_ptr_t
#define opaque_ptr_t void *
#endif // opaque_ptr_t

#define WEBSOCKET_OPTIONS_DEFAULT(_host)                                       \
  &(Ws_Options){.host = _host, .port = 80, .is_tls = false, .path = "/"}
#define SEC_WEBSOCKET_KEY_LEN 24

#define WS_LOG_DEBUG _LOG_DEBUG
#define WS_LOG_WARN _LOG_WARN
#define WS_LOG_ERROR _LOG_ERROR

#define WS_ErrToStr(WS_ERR) Ws_Error_Strs[WS_ERR]

#define WS_ALLOC(allocator_ctx, ...) malloc(__VA_ARGS__);
#define WS_REALLOC(allocator_ctx, ...) realloc(__VA_ARGS__);
#define WS_FREE(allocator_ctx, ...) free(__VA_ARGS__);

typedef enum {
  WS_SUCCESS = 0,
  WS_CONNECTION_FAILURE,
  WS_WRITE_FAILED,
  WS_INTERNAL_FAILURE,
  WS_STATUS_COUNT,
} WS_STATUS;

static const char *Ws_Error_Strs[] __attribute__((unused)) = {
    "WS_SUCCESS", "WS_CONNECTION_FAILURE", "WS_INTERNAL_FAILURE", "WS_FAILED"};

_Static_assert(WS_STATUS_COUNT == ARRAY_LEN(Ws_Error_Strs),
               "forgor💀 to add enum variant to Ws_Error_Strs");

typedef struct Ws_AllocatorContext {
  opaque_ptr_t ctx;
} Ws_AllocatorContext;

typedef struct Ws_Options {
  const char *host;
  const char *path; // null terminated
  uint16_t port;
  uint16_t is_tls; // cries in memory alignment
} Ws_Options;

typedef struct {
  struct Ws_AllocatorContext allocator_ctx; // mark(unused)
  struct Ws_Options opts;
  int tcp_fd;
  struct io io_ctx;
  opaque_ptr_t data;
  char padding[4];
} Ws_Context;

typedef void(Ws_Handler)(const char *data, size_t len, opaque_ptr_t userdata);

// i really don't wanna write a uri parser so user will have to
// rawdog the host and port
Ws_Context ws_context_new(opaque_ptr_t allocator, Ws_Options *opts);
WS_STATUS ws_connect(Ws_Context *ctx);
WS_STATUS ws_establish_tcp_connection(Ws_Context *ctx)
    __attribute_nonnull__((1));
WS_STATUS ws_write_raw(Ws_Context *ctx, const uint8_t *data, size_t len);
void ws_on(const char *event, Ws_Handler *handler);
void ws_do_http_upgrade(Ws_Context *ctx, const char *sec_websocket_key,
                        size_t sec_websocket_key_len);

#endif // __WEBSOCKET_H__
