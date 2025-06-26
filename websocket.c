#include "websocket.h"
#include "libs/extint.h"
#ifdef __linux__
#include "io/linux.h"
#endif
#include "libs/allocator.h"
#include "libs/base64.h"
#include "libs/dyn_array.h"
#include "libs/rng_xoroshiro.c"
#include "libs/sha1.h"
#include "libs/utils.h"
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#ifndef BS_IMPLEMENTATION
#define BS_IMPLEMENTATION
#endif // BS_IMPLEMENTATION
#include "libs/bs.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct _httpHeader {
  BetterString_View key;
  BetterString_View value;
} _httpHeader;

typedef struct _httpPayload {
  opaque_ptr_t allocator_ctx;
  BetterString_ViewManaged http_line;
  _httpHeader *items;
  size_t len;
  size_t capacity;
  const char *body;
} _httpPayload;

// note(shahzad): c doesn't support returning array from function
typedef struct _secWebsocketKey {
  char data[SEC_WEBSOCKET_KEY_LEN];
} _secWebsocketKey;

static _httpHeader *_http_payload_header_get(_httpPayload *payload,
                                             BetterString_View key) {
  // i know hash map exists
  for (size_t i = 0; i < payload->len; i++) {

    if (bs_eq(payload->items[i].key, key) == true) {
      return payload->items + i;
    }
  }
  return NULL;
}

// managed or nah, that's the question
static inline void _http_payload_append_header(_httpPayload *payload,
                                               const _httpHeader http_header) {
  _httpHeader *result = NULL;
  da_append(*payload, http_header, result);
  assert(result != NULL);
}
static inline ALLOCATOR_STATUS _http_parse_header(_httpPayload *payload,
                                                  BetterString_View *request) {

  // _TODO("FIXME!: this function can't return allocator status.");
  // todo(shahzad)!: use arena allocator
  if (payload->http_line.view == NULL || payload->http_line.len == 0) {
    BetterString_View http_line = bs_split_once_by_bs(request, BSV("\r\n"));

    payload->http_line = bs_clone(payload->allocator_ctx, http_line);
    if (payload->http_line.view == NULL) {
      return ALLOCATOR_OUT_OF_MEMORY;
    }
    _LOG_DEBUG("http line: " BSV_Fmt "\n", BSV_Arg(http_line));
  }
  while (request->len > 0) {
    BetterString_View http_header =
        bs_trim(bs_split_once_by_bs(request, BSV("\r\n")));
    if (http_header.len == 0) {
      break;
    }

    BetterString_View key = bs_trim(bs_split_once_by_delim(&http_header, ':'));
    BetterString_View value = bs_trim(http_header);

    // note(shahzad)!: write directly to arena allocator instead of bs_builders
    BetterString_Builder owned_key = bs_builder_new(payload->allocator_ctx);
    BetterString_Builder owned_value = bs_builder_new(payload->allocator_ctx);

    bs_builder_reserve_exact(&owned_key, key.len);
    bs_builder_reserve_exact(&owned_value, value.len);
    bs_builder_append_sv(&owned_key, key);
    bs_builder_append_sv(&owned_value, value);

    assert(owned_key.string.len == key.len);
    assert(owned_value.string.len == value.len);

    _http_payload_append_header(
        payload, (_httpHeader){.key = bs_builder_to_sv(&owned_key),
                               .value = bs_builder_to_sv(&owned_value)});
    _LOG_DEBUG("key: \"" BSV_Fmt "\"\n", BSV_Arg(key));
    _LOG_DEBUG("value: \"" BSV_Fmt "\"\n", BSV_Arg(value));
    _LOG_DEBUG("\n");
  }
  return ALLOCATOR_SUCCESS;
}

static inline BetterString_View _http_status_code(_httpPayload *payload) {
  // todo!(shahzad): parse this shit to an integer
  if (payload->http_line.view == NULL) {
    return (BetterString_View){0};
  }
  BetterString_ViewManaged http_line = payload->http_line;
  BetterString_View code =
      bs_split_once_by_bs((BetterString_View *)&http_line, BSV(" "));
  code = bs_split_once_by_bs((BetterString_View *)&http_line, BSV(" "));
  assert(code.view != NULL);
  assert(code.len != 0);
  return code;
}
// note(shahzad): this doesn't look good
Ws_Context ws_context_new(opaque_ptr_t allocator_ctx, Ws_Options *opts) {
  Ws_Context ctx = {
      .allocator_ctx = {.ctx = allocator_ctx},
      .opts = *opts,
  };
  return ctx;
}

WS_STATUS ws_establish_tcp_connection(Ws_Context *ctx) {
  const char *host = ctx->opts.host;
  const uint16_t port = ctx->opts.port;

  const int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_addr.s_addr = inet_addr(host),
                             .sin_port = htons(port)};
  const int connect_status =
      connect(tcp_socket_fd, (struct sockaddr *)&addr, sizeof(addr));
  if (connect_status == -1) {
    return WS_CONNECTION_FAILURE;
  }
  ctx->tcp_fd = tcp_socket_fd;
  ctx->io_ctx = io_init();
  return WS_SUCCESS;
}

WS_STATUS ws_write_raw(Ws_Context *ctx, const uint8_t *data, size_t len) {
  int events_processed =
      io_send_write_event(&ctx->io_ctx, ctx->tcp_fd, data, len);
  assert(events_processed == 1);
  const ssize_t bytes_wrote = io_get_processed_event_result(&ctx->io_ctx);

  WS_LOG_DEBUG("written %ld bytes out of %lu\n", bytes_wrote, len);
  if (bytes_wrote == -1) {
    return WS_WRITE_FAILED;
  }
  assert((size_t)bytes_wrote == len);
  return WS_SUCCESS;
}

static inline bool _verify_utf8(const char *data, size_t len) {
  _UNUSED(data);
  _UNUSED(len);
  _LOG_WARN("unimplemented!\n");
  return true;
}
// todo(shahzad): refactor this
static ssize_t ws_read_raw(Ws_Context *ctx, uint8_t *buff, size_t len) {
  int events_processed =
      io_send_read_event(&ctx->io_ctx, ctx->tcp_fd, buff, len);
  assert(events_processed == 1);
  const ssize_t bytes_read = io_get_processed_event_result(&ctx->io_ctx);
  WS_LOG_DEBUG("WS_READ returned with %ld, STATUS: %s\n", bytes_read,
               strerror(errno));
  return bytes_read;
}

static bool _ws_verify_websocket_sec_key(Ws_Context *ctx,
                                         BetterString_View sec_key,
                                         BetterString_View sec_accept) {
  // todo(shahzad)!: fix this please
  BetterString_Builder sec_key_builder = bs_builder_new(ctx);
  ALLOCATOR_STATUS status = ALLOCATOR_SUCCESS;
  bs_builder_sprintf(status, &sec_key_builder, BSV_Fmt "%s", BSV_Arg(sec_key),
                     "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  assert(status == ALLOCATOR_SUCCESS);
  sha1Digest digest = sha1_digest((const uint8_t *)sec_key_builder.string.data,
                                  sec_key_builder.string.len);

  assert(memcmp(digest._As.u32, (uint32_t[5]){0}, 5) != 0);

  sha1_print_digest(&digest);
  digest = sha1_to_le(digest);

  const uint8_t *sha1_digest_array = digest._As.u8;
  const size_t sha1_digest_array_len = sizeof(digest._As.u8);
  for (size_t i = 0; i < sha1_digest_array_len; i++) {
    printf("%x", sha1_digest_array[i]);
  }
  printf("\n");

  assert(sha1_digest_array_len == 5 * 4);

  const size_t sha1_base64_required_len =
      BASE64_ENCODE_REQUIRED_OUTPUT_BUFFER_SIZE(sha1_digest_array_len + 2);

  BetterString_Builder base64_digest_builder =
      bs_builder_new_alloc(ctx->allocator_ctx.ctx, sha1_base64_required_len);

  base64_encode(sha1_digest_array, sha1_digest_array_len,
                base64_digest_builder.string.data,
                base64_digest_builder.string.capacity,
                &base64_digest_builder.string.len,
                (base64EncodeSettings){.padding = BASE64_ENCODE_PADDING});
  const BetterString_View base64_digest_sv =
      bs_builder_to_sv(&base64_digest_builder);
  WS_LOG_DEBUG("sec_key: \"" BSV_Fmt "\"\n", BSV_Arg(base64_digest_sv));
  WS_LOG_DEBUG("sec_sec_accept: \"" BSV_Fmt "\"\n", BSV_Arg(sec_accept));
  assert(bs_eq(base64_digest_sv, sec_accept));

  return true;
}

static inline void _ws_verify_sub_protocol(void) {
  _LOG_WARN("TODO: unimplemented");
}
static inline _secWebsocketKey _generate_sec_websocket_key(void) {
  _secWebsocketKey key = {0};
  xoroshiro128Context rng_ctx = xoroshiro128_init((size_t)time(NULL));
  uint8_t random_key[16] = {0};
  xoroshiro128_fill(&rng_ctx, random_key, 16);
  BASE64_STATUS base64_status = base64_encode(
      random_key, 16, key.data, SEC_WEBSOCKET_KEY_LEN, &(size_t){0},
      (base64EncodeSettings){.padding = BASE64_ENCODE_PADDING});
  assert(base64_status == BASE64_STATUS_SUCCESS);
  return key;
}
// todo(shahzad)!: add error shit here when upgrade fails
void ws_do_http_upgrade(Ws_Context *ctx, const char *sec_websocket_key,
                        size_t sec_websocket_key_len) {
  BetterString_Builder upgrade_request = {0};
  BetterString_View sec_websocket_key_view = {.view = sec_websocket_key,
                                              .len = sec_websocket_key_len};

  WS_LOG_DEBUG("sec key original: \"" BSV_Fmt "\"\n",
               BSV_Arg(sec_websocket_key_view));

  ALLOCATOR_STATUS builder_status = ALLOCATOR_SUCCESS;
  bs_builder_sprintf(builder_status, &upgrade_request, "GET %s HTTP/1.1\r\n",
                     ctx->opts.path);

  bs_builder_append_cstr(&upgrade_request, "Host: 127.0.0.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Upgrade: websocket\r\n");
  bs_builder_append_cstr(&upgrade_request, "Connection: Upgrade\r\n");
  bs_builder_sprintf(builder_status, &upgrade_request,
                     "Sec-WebSocket-Key: " BSV_Fmt "\r\n",
                     BSV_Arg(sec_websocket_key_view));
  bs_builder_append_cstr(&upgrade_request, "Origin: http://127.0.0.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Sec-WebSocket-Protocol: chat\r\n");
  bs_builder_append_cstr(&upgrade_request, "Sec-WebSocket-Version: 13\r\n");
  bs_builder_append_cstr(&upgrade_request, "\r\n");
  _LOG_TODO("add functionality to append custom headers provided by the "
            "user\n");

  BetterString_View result_str = bs_builder_to_sv(&upgrade_request);

  // todo(shahzad)!!!!!!!!!!!!!!: handle memory management with io_uring
  ws_write_raw(ctx, (const uint8_t *)result_str.view, result_str.len);
  // bs_builder_destory(&upgrade_request);
}
WS_STATUS ws_connect(Ws_Context *ctx) {
  if (ctx->opts.is_tls == true) {
    WS_LOG_ERROR("We don't support secure websocket!");
    return WS_CONNECTION_FAILURE;
  }
  const WS_STATUS tcp_connect_status = ws_establish_tcp_connection(ctx);
  if (tcp_connect_status != WS_SUCCESS) {
    return tcp_connect_status;
  }

  BetterString_View sec_websocket_key =
      bs_from_string(_generate_sec_websocket_key().data, SEC_WEBSOCKET_KEY_LEN);

  ws_do_http_upgrade(ctx, sec_websocket_key.view, sec_websocket_key.len);

  // todo(shahzad)!: use allocator
  static uint8_t buffer[1024] = {0};
  const ssize_t bytes_read = ws_read_raw(ctx, buffer, 1024);
  if (bytes_read == -1) {
    return WS_CONNECTION_FAILURE;
  }
  _verify_utf8((const char *)buffer, (size_t)bytes_read);
  BetterString_View read_data =
      bs_from_string((const char *)buffer, (size_t)bytes_read);

  WS_LOG_DEBUG(
      "got http response: ==================================\n" BSV_Fmt,
      BSV_Arg(read_data));
  WS_LOG_DEBUG("=====================================================\n");

  _httpPayload payload = {0};
  _http_parse_header(&payload, &read_data);

  if (read_data.len != 0) { // couldn't parse all of the header
    _TODO("implement streaming data to http parser.");
    memmove(buffer, read_data.view, read_data.len);
    read_data.view = (const char *)buffer;
  }
  // todo(shahzad)!: this should be an integer
  const BetterString_View status_code = _http_status_code(&payload);
  if (bs_eq(status_code, BSV("101")) == false) {
    WS_LOG_DEBUG("wrong error code: expected %s got \"" BSV_Fmt "\"\n", "101",
                 BSV_Arg(status_code));
    return WS_CONNECTION_FAILURE;
  }
  _httpHeader *response_sec_key =
      _http_payload_header_get(&payload, BSV("Sec-WebSocket-Accept"));
  _ws_verify_websocket_sec_key(ctx, sec_websocket_key, response_sec_key->value);
  _ws_verify_sub_protocol();

  return WS_SUCCESS;
}
