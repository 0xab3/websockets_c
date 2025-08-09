#include "websocket.h"
#define BIO_IMPLEMENTATION
#include "./libs/buff_io.h"
#define BS_IMPLEMENTATION
#include "./libs/bs.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
internal void on_message(Ws_Message message, void *userdata) {
  _LOG_DEBUG("%.*s\n", (int)message.size, message.data);
}
int main(void) {
  const char *host = "127.0.0.1";
  const uint16_t port = 5000;
  Ws_Context ctx = ws_context_new(NULL, &(Ws_Options){.host = "127.0.0.1",
                                                      .port = 5000,
                                                      .is_tls = false,
                                                      .path = "/"});

  WS_STATUS ws_status = ws_connect(&ctx);
  if (ws_status != WS_SUCCESS) {
    WS_LOG_ERROR("%s, failed to connect to %s at port %d: %s",
                 WS_ErrToStr(ws_status), host, port, strerror(errno));

    return EXIT_FAILURE;
  }

  const char *message = "man wth is this shit bro idk what to do";
  ws_send_message(&ctx, message, strlen(message), false);
  return EXIT_SUCCESS;
}
