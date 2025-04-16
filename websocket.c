#include "websocket.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
// note(shahzad): this doesn't look good
Ws_Context ws_context_init(opaque_ptr_t allocator_ctx) {
  Ws_Context ctx = {
      .allocator_ctx = {.ctx = allocator_ctx},
  };
  return ctx;
}
WS_STATUS ws_establish_tcp_connection(Ws_Context *ctx, const char *host,
                                      uint16_t port) {
  const int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_addr.s_addr = inet_addr(host),
                             .sin_port = htons(port)

  };
  const int ret =
      connect(tcp_socket_fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    return WS_CONNECTION_FAILURE;
  }
  ctx->tcp_fd = tcp_socket_fd;
  return WS_SUCCESS;
}

WS_STATUS ws_send_http_upgdate_request(Ws_Context *ctx,
                                       const BetterString_View *request) {
  ssize_t ret = write(ctx->tcp_fd, request->view, request->len);
  WS_LOG_DEBUG("sent http upgrade request, written %ld bytes\n", ret);
  if (ret == -1) {
    return WS_WRITE_FAILED;
  }
  assert((size_t)ret == request->len);
  return WS_SUCCESS;
}

static size_t ws_read_tcp_data(Ws_Context *ctx, uint8_t *buff, size_t len) {

  ssize_t bytes_read = read(ctx->tcp_fd, buff, len);
  WS_LOG_DEBUG("WS_READ returned with %ld, STATUS: %s\n", bytes_read,
               strerror(errno));
  WS_LOG_DEBUG("response\n");
  WS_LOG_DEBUG("----------------------------------------\n");
  WS_LOG_DEBUG("%s", buff);
  WS_LOG_DEBUG("----------------------------------------\n");
  _UNUSED(bytes_read);
  return 0;
}

void ws_do_http_upgrade(Ws_Context *ctx) {
  BetterString_Builder upgrade_request = {0};

  bs_builder_append_cstr(&upgrade_request, "GET /chat HTTP/1.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Host: 127.0.0.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Upgrade: websocket\r\n");
  bs_builder_append_cstr(&upgrade_request, "Connection: Upgrade\r\n");
  bs_builder_append_cstr(&upgrade_request,
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n");
  bs_builder_append_cstr(&upgrade_request, "Origin: http://127.0.0.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Sec-WebSocket-Protocol: chat\r\n");
  bs_builder_append_cstr(&upgrade_request, "Sec-WebSocket-Version: 13\r\n");
  bs_builder_append_cstr(&upgrade_request, "\r\n");

  BetterString_View result_str = bs_builder_to_sv(&upgrade_request);

  ws_send_http_upgdate_request(ctx, &result_str);

  static uint8_t buffer[1024] = {0};
  ws_read_tcp_data(ctx, buffer, 1024);
}
