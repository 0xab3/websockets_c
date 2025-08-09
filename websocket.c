#define _XOPEN_SOURCE 500

#include <fcntl.h>

#ifndef BIO_IMPLEMENTATION
#define BIO_IMPLEMENTATION
#endif // BIO_IMPLEMENTATION
#include "libs/buff_io.h"

#include "libs/allocator.h"
#include "libs/base64.h"

#ifdef __linux
#include "libs/crng_linux.c"
#endif

#include "libs/dyn_array.h"
#include "libs/rng_xoroshiro.c"

#include "libs/sha1.h"
#include "libs/utils.h"
#include <linux/time_types.h>
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

internal ssize_t ws__write_raw(Ws_Context *ctx, char *data, size_t len);
internal ssize_t ws__read_raw(Ws_Context *ctx, char *buff, size_t len);
internal ssize_t write_exact(int fd, const char *buff, size_t len);
internal void _print_ws_as_hex(uint8_t *data, size_t len) {
  printf("==============================\n");
  for (size_t i = 0; i < len; i++) {
    printf("%x ", data[i]);
  }
  printf("==============================\n");
}
// internal void _print_ws_frame(const Ws_FrameHeader *frame) {
// printf("Ws_Frame {\n");
// printf("  fin: %u\n", frame->fin);
// printf("  rsv1: %u\n", frame->rsv1);
// printf("  rsv2: %u\n", frame->rsv2);
// printf("  rsv3: %u\n", frame->rsv3);
// printf("  opcode: %u\n", frame->opcode);
// printf("  mask: %u\n", frame->masking_key);
// printf("  payload_len: %lu\n", frame->payload_len);
// BetterString_View svd = bs_escape(
//     bs_from_string((char *)frame->payload_data, frame->payload_len));
// printf("  payload_data: \"%.*s\"\n", BSV_Arg(svd));
// printf("}\n");
// }

// internal Ws_FrameHeader websocket_buffer_to_frame_header(uint8_t *buffer,
//                                                          size_t len) {
//   size_t buffer_idx = 0;
//   const uint8_t fin = buffer[buffer_idx] >> 7;
//   const uint8_t rsv1 = (buffer[buffer_idx] >> 6) & 0x1;
//   const uint8_t rsv2 = (buffer[buffer_idx] >> 5) & 0x1;
//   const uint8_t rsv3 = (buffer[buffer_idx] >> 4) & 0x1;
//   const uint8_t opcode = buffer[buffer_idx] & 0xf;
//   buffer_idx++;
//   const uint8_t mask = buffer[buffer_idx] >> 7;
//   uint32_t masking_key = 0;
//   uint64_t payload_len = buffer[buffer_idx] & 0x7f;
//   buffer_idx++;
//   if (payload_len == 126) {
//     payload_len = to_le16(*(uint16_t *)(buffer + buffer_idx));
//     buffer_idx += 2;
//
//   } else if (payload_len == 127) {
//     payload_len = to_le64(*(uint64_t *)(buffer + buffer_idx));
//     buffer_idx += 8;
//   }
//   if (mask == 1) {
//     masking_key = to_le32(*(uint32_t *)(buffer + buffer_idx));
//     buffer_idx += 4;
//   }
//   Ws_FrameHeader header = (Ws_FrameHeader){
//       .fin = fin,
//       .rsv1 = rsv1,
//       .rsv2 = rsv2,
//       .rsv3 = rsv3,
//       .opcode = opcode,
//       .payload_len = payload_len,
//       .masking_key = masking_key,
//       .payload_data = buffer + buffer_idx,
//   };
//   return header;
// }

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
static inline _httpHeader _http_parse_header(_httpPayload *payload,
                                             BetterString_View *request) {
  BetterString_View http_header = bs_trim(*request);

  if (http_header.len == 0) _UNREACHABLE("empty http header!");

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
  _LOG_DEBUG("key: \"" BSV_Fmt "\"", BSV_Arg(key));
  _LOG_DEBUG("value: \"" BSV_Fmt "\"", BSV_Arg(value));

  return (_httpHeader){0};
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
      .crng_ctx = crng_init(),
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
  // should we do allocator shit?
  static char buffer[BUFIO_DEFAULT_BUFFER_SIZE] = {0};
  BufReader_init(&ctx->tcp_reader, tcp_socket_fd, buffer,
                 BUFIO_DEFAULT_BUFFER_SIZE);
  return WS_SUCCESS;
}

static inline bool _verify_utf8(const char *data, size_t len) {
  _UNUSED(data);
  _UNUSED(len);
  return true;
}

static bool _ws_verify_websocket_sec_key(Ws_Context *ctx,
                                         BetterString_View sec_key,
                                         BetterString_View sec_accept) {
  // todo(shahzad)!: fix this please
  BetterString_Builder sec_key_builder = bs_builder_new(ctx);
  ssize_t bytes_wrote =
      bs_builder_sprintf(&sec_key_builder, BSV_Fmt "%s", BSV_Arg(sec_key),
                         "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

  if (bytes_wrote < 0) {
    if (bytes_wrote == -ALLOCATOR_OUT_OF_MEMORY) {
      _TODO("fix allocator");
    }
    _UNREACHABLE();
  }

  sha1Digest digest = sha1_digest((const uint8_t *)sec_key_builder.string.data,
                                  sec_key_builder.string.len);

  assert(memcmp(digest._As.u32, (uint32_t[5]){0}, 5) != 0);

  sha1_print_digest(&digest);
  digest = sha1_to_le(digest);

  const uint8_t *sha1_digest_array = digest._As.u8;
  const size_t sha1_digest_array_len = sizeof(digest._As.u8);

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
  WS_LOG_DEBUG("sec_key: \"" BSV_Fmt "\"", BSV_Arg(base64_digest_sv));
  WS_LOG_DEBUG("sec_sec_accept: \"" BSV_Fmt "\"", BSV_Arg(sec_accept));
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

  WS_LOG_DEBUG("sec key original: \"" BSV_Fmt "\"",
               BSV_Arg(sec_websocket_key_view));

  ssize_t bytes_wrote = bs_builder_sprintf(
      &upgrade_request, "GET %s HTTP/1.1\r\n", ctx->opts.path);
  assert(bytes_wrote > 0);

  bs_builder_append_cstr(&upgrade_request, "Host: 127.0.0.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Upgrade: websocket\r\n");
  bs_builder_append_cstr(&upgrade_request, "Connection: Upgrade\r\n");
  bytes_wrote =
      bs_builder_sprintf(&upgrade_request, "Sec-WebSocket-Key: " BSV_Fmt "\r\n",
                         BSV_Arg(sec_websocket_key_view));
  assert(bytes_wrote > 0);
  bs_builder_append_cstr(&upgrade_request, "Origin: http://127.0.0.1\r\n");
  bs_builder_append_cstr(&upgrade_request, "Sec-WebSocket-Protocol: chat\r\n");
  bs_builder_append_cstr(&upgrade_request, "Sec-WebSocket-Version: 13\r\n");
  bs_builder_append_cstr(&upgrade_request, "\r\n");
  _LOG_TODO("add functionality to append custom headers provided by the "
            "user");

  BetterString_ViewManaged result_str =
      bs_builder_to_managed_sv(&upgrade_request);
  _LOG_DEBUG("==============================");
  _LOG_DEBUG(BSV_Fmt, BSV_Arg(result_str));
  _LOG_DEBUG("==============================");

  bytes_wrote = ws__write_raw(ctx, result_str.view, result_str.len);
  assert((size_t)bytes_wrote == result_str.len);
  bs_builder_destory(&upgrade_request);
}
internal _httpPayload ws__http_read_header(Ws_Context *ctx) {
  _httpPayload payload = {0};
  static char buffer[1024] = {0};

  ssize_t bytes_read =
      BufReader_read_until(&ctx->tcp_reader, buffer, 1024, "\r\n");
  if (bytes_read == -1 || bytes_read == BUFIO_ERROR_CONNECTION_CLOSED) {
    WS_LOG_DEBUG("failed to read %ld: %s\n", bytes_read, strerror(errno));
    assert(0 && "not handled");
  }
  BetterString_View line_as_view = bs_from_string(buffer, bytes_read);
  payload.http_line = bs_clone(ctx->allocator_ctx.ctx, bs_trim(line_as_view));
  while (true) {
    bytes_read = BufReader_read_until(&ctx->tcp_reader, buffer, 1024, "\r\n");
    if (bytes_read == -1 || bytes_read == BUFIO_ERROR_CONNECTION_CLOSED) {
      WS_LOG_DEBUG("failed to read until %ld: %s\n", bytes_read,
                   strerror(errno));
      assert(0 && "not handled");
    }
    line_as_view = bs_from_string(buffer, (size_t)bytes_read);
    WS_LOG_DEBUG("read line \"" BSV_Fmt "\"", BSV_Arg(bs_trim(line_as_view)));

    // this means that we have parsed the header completely
    if (bs_eq(line_as_view, BSV("\r\n"))) return payload;

    _verify_utf8(line_as_view.view, line_as_view.len);

    _http_parse_header(&payload, &line_as_view);
  }
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

  _httpPayload payload = ws__http_read_header(ctx);

  const BetterString_View status_code = _http_status_code(&payload);
  if (bs_eq(status_code, BSV("101")) == false) {
    WS_LOG_DEBUG("wrong error code: expected %s got \"" BSV_Fmt "\"", "101",
                 BSV_Arg(status_code));
    return WS_CONNECTION_FAILURE;
  }
  _httpHeader *response_sec_key =
      _http_payload_header_get(&payload, BSV("Sec-WebSocket-Accept"));
  _ws_verify_websocket_sec_key(ctx, sec_websocket_key, response_sec_key->value);
  _ws_verify_sub_protocol();
  return WS_SUCCESS;
}
internal int32_t ws__get_masking_key(Ws_Context *ctx) {
  return crng_next_i32(&ctx->crng_ctx);
}

internal char *ws__mask_payload(Ws_Context *ctx, int32_t mask,
                                const char *payload, size_t payload_len) {
  _UNUSED(ctx);
  // TODO(shahzad): switch to a temporary allocator
  char *bytes =
      WS_ALLOC(ctx->allocator_ctx.ctx, (sizeof(*payload) * payload_len));
  for (size_t i = 0; i < payload_len; i++) {
    bytes[i] = payload[i] ^ ((const char *)&mask)[i % 4];
  }
  return bytes;
}

internal void ws_write_frame(Ws_Context *ctx, Ws_Frame frame) {
  const int32_t masking_key = ws__get_masking_key(ctx);
  uint8_t fin_and_opcode = (uint8_t)((frame.fin & 0x1) << 7) |
                           (uint8_t)((frame.rsv & 0x7) << 4) |
                           (uint8_t)(frame.opcode & 0xF);

  write_exact((int)ctx->tcp_reader.fd, (char *)&fin_and_opcode,
              sizeof(uint8_t));

  uint8_t pre_payload_len = (uint8_t)frame.payload_len;
  assert(pre_payload_len < 125);
  if (frame.payload_len > 125) {
    pre_payload_len = 126;
    if (frame.payload_len > 0xffff) pre_payload_len = 127;
  }

  assert(frame.mask);
  if (frame.mask) pre_payload_len |= 0x80;

  write_exact((int)ctx->tcp_reader.fd, (char *)&pre_payload_len,
              sizeof(uint8_t));
  write_exact((int)ctx->tcp_reader.fd, (const char *)&masking_key, 4);
  const char *masked_payload =
      ws__mask_payload(ctx, masking_key, frame.payload, frame.payload_len);
  write_exact((int)ctx->tcp_reader.fd, masked_payload,
              sizeof(*masked_payload) * frame.payload_len);
}
void ws_send_message(Ws_Context *ctx, const char *data, size_t len,
                     bool is_binary) {
  // TODO(shahzad): implement framing
  Ws_Frame frame = {
      .fin = 1,
      .mask = 1,
      .opcode = is_binary ? WS_FRAME_OP_BINARY : WS_FRAME_OP_TEXT,
      .payload_len = len,
      .payload = data,
  };
  ws_write_frame(ctx, frame);
}
internal ssize_t ws__write_raw(Ws_Context *ctx, char *data, size_t len) {
  return write((int)ctx->tcp_reader.fd, data, len);
}

internal ssize_t ws__read_raw(Ws_Context *ctx, char *buff, size_t len) {
  return BufReader_read(&ctx->tcp_reader, buff, len);
}
internal ssize_t write_exact(int fd, const char *buff, size_t len) {
  assert(fd != -1);
  size_t bytes_wrote = 0;
  do {
    ssize_t chunk_wrote = write(fd, buff + bytes_wrote, len - bytes_wrote);

    if (chunk_wrote == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      } else return -1;
    } else if (chunk_wrote == 0) return 0; // connection is closed

    assert(chunk_wrote > 0);
    bytes_wrote += (size_t)chunk_wrote;
  } while ((size_t)bytes_wrote < len);
  assert(bytes_wrote == len);
  return (ssize_t)bytes_wrote;
}
