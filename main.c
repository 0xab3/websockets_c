#include "websocket.h"
#include <stdio.h>

#define BS_IMPLEMENTATION
#include "bs.h"
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

  int ret = ws_establish_tcp_connection(host, port);
  if (ret == -1) {
    WS_ERROR("failed to connect to %s at port %d: %s", host, port,
             strerror(errno));
    return EXIT_FAILURE;
  }
  ws_do_http_upgrade(ret);

  return EXIT_SUCCESS;
}
