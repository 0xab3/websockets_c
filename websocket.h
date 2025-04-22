#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include "./libs/bs.h"

#include "./libs/dyn_array.h"
#include <arpa/inet.h>
#include <assert.h>
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifndef opaque_ptr_t
#define opaque_ptr_t void *
#endif

#define SEC_WEBSOCKET_KEY_LEN 16

#define WS_LOG_DEBUG _LOG_DEBUG
#define WS_LOG_WARN _LOG_WARN
#define WS_LOG_ERROR _LOG_ERROR

#define WS_ErrToStr(WS_ERR) Ws_Error_Strs[WS_ERR]

#define WS_ALLOC(allocator_ctx, ...) malloc(__VA_ARGS__);
#define WS_REALLOC(allocator_ctx, ...) realloc(__VA_ARGS__);
#define WS_FREE(allocator_ctx, ...) free(__VA_ARGS__);

typedef enum {
  WS_SUCCESS,
  WS_CONNECTION_FAILURE,
  WS_WRITE_FAILED,
  WS_FAILED,
  WS_STATUS_COUNT,
} WS_STATUS;

static const char *Ws_Error_Strs[] __attribute__((unused)) = {
    "WS_SUCCESS", "WS_CONNECTION_FAILURE", "WS_WRITE_FAILED", "WS_FAILED"};

_Static_assert(WS_STATUS_COUNT == ARRAY_LEN(Ws_Error_Strs),
               "forgor💀 to add enum variant to Ws_Error_Strs");

da_new(BetterString_View);
da_new(uint8_t);

#define Ws_Buffer Dyn_Array_uint8_t

typedef struct {
  opaque_ptr_t ctx;
} Ws_AllocatorCtx;

typedef struct {
  Ws_AllocatorCtx allocator_ctx; // mark(unused)
  Ws_Buffer data;
  int tcp_fd;
  char padding[4];
} Ws_Context;

Ws_Context ws_context_init(opaque_ptr_t allocator);
WS_STATUS ws_establish_tcp_connection(Ws_Context *ctx, const char *host,
                                      uint16_t port) __attribute_nonnull__((1));
WS_STATUS ws_send_http_upgdate_request(Ws_Context *ctx,
                                       const BetterString_View *request);
void ws_do_http_upgrade(Ws_Context *ctx);

#endif
