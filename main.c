#include "websocket.h"

#define BS_IMPLEMENTATION
#include "./libs/bs.h"
#include <stddef.h>

#include <assert.h>
#include <bits/types/struct_iovec.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
int main(void) {
  const char *host = "127.0.0.1";
  const uint16_t port = 5000;
  Ws_Context ctx = ws_context_init(NULL);

  WS_STATUS ws_status = ws_establish_tcp_connection(&ctx, host, port);
  if (ws_status != WS_SUCCESS) {
    WS_LOG_ERROR("failed to connect to %s at port %d: %s", host, port,
                 strerror(errno));
    return EXIT_FAILURE;
  }

  ws_do_http_upgrade(&ctx);
  return EXIT_SUCCESS;
}
