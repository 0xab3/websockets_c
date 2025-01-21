#include "websocket.h"
#include <sys/types.h>
int ws_establish_tcp_connection(const char *host, uint16_t port) {
  const int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_addr.s_addr = inet_addr(host),
                             .sin_port = htons(port)

  };
  const int ret =
      connect(tcp_socket_fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    return ret;
  }
  return tcp_socket_fd;
}

int ws_send_http_upgdate_request(int tcp_socket_fd,
                                 const Dyn_Array_Better_String headers) {
  size_t http_header_idx = 0;
  struct iovec http_header_write_vec[MAX_HTTP_HEADERS] = {0};
  assert(headers.len < MAX_HTTP_HEADERS &&
         "MAX HTTP HEADER LIMIT IS 64! IF YOU HAVE MORE THAN THAT INCREASE IT "
         "IN THE HEADER FILE!!!");
  for (http_header_idx = 0; http_header_idx < headers.len; http_header_idx++) {
    http_header_write_vec[http_header_idx].iov_base =
        headers.items[http_header_idx].data;
    http_header_write_vec[http_header_idx].iov_len =
        headers.items[http_header_idx].len;
  }

  ssize_t ret =
      writev(tcp_socket_fd, http_header_write_vec, (int)http_header_idx);
  WS_DEBUG("written %d bytes\n", ret);
  return 0;
}

void ws_do_http_upgrade(int tcp_socket_fd) {
  Dyn_Array_Better_String upgrade_request = {0};
  DA_ERROR result = DA_SUCCESS;

  da_append(upgrade_request, BS("GET /chat HTTP/1.1\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("Host: 127.0.0.1\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("Upgrade: websocket\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("Connection: Upgrade\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request,
            BS("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("Origin: http://127.0.0.1\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("Sec-WebSocket-Protocol: chat\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("Sec-WebSocket-Version: 13\r\n"), result);
  assert(result == DA_SUCCESS);
  da_append(upgrade_request, BS("\r\n"), result);
  assert(result == DA_SUCCESS);

  ws_send_http_upgdate_request(tcp_socket_fd, upgrade_request);
}
