#include "./base64.h"
#include "libs/utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

static __always_inline void
_base64_value_for_data(const char *data, size_t size, char *output_buffer);
static __always_inline void _base64_to_data(const char *data, size_t size,
                                            char *output_buffer);

BASE64_STATUS base64_encode(const uint8_t *__restrict buffer, size_t len,
                            char *__restrict output_buffer, size_t output_len,
                            size_t *encoded_b64_len,
                            base64EncodeSettings settings) {
  if (output_len < BASE64_ENCODE_REQUIRED_OUTPUT_BUFFER_SIZE(len)) {
    return BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL;
  }
  const size_t remaining_bytes = len % 3;
  const size_t aligned_3_byte_len = len - remaining_bytes;
  size_t output_buffer_idx = 0;
  uint8_t bytes_3[3] = {0};
  char output_quad[4] = {0};
  for (size_t i = 0; i < aligned_3_byte_len; i += 3) {
    bytes_3[0] = buffer[i];
    bytes_3[1] = buffer[i + 1];
    bytes_3[2] = buffer[i + 2];

    output_quad[0] = (bytes_3[0] >> 2) & 0x3f;
    output_quad[1] = (((bytes_3[0] & 3) << 4) | ((bytes_3[1] >> 4))) & 0x3f;
    output_quad[2] = (((bytes_3[1] & 0xf) << 2) | (bytes_3[2] >> 6)) & 0x3f;
    output_quad[3] = bytes_3[2] & 0x3f;
    _base64_value_for_data(output_quad, 4, output_buffer + output_buffer_idx);
    output_buffer_idx += 4;
    assert(output_buffer_idx < output_len);
  }

  switch (remaining_bytes) {
  case 0: break;
  case 1:
    output_quad[0] = (buffer[aligned_3_byte_len] >> 2) & 0x3f;
    output_quad[1] = ((buffer[aligned_3_byte_len] & 0x03) << 4) & 0x3f;
    _base64_value_for_data(output_quad, 2, output_buffer + output_buffer_idx);
    output_buffer_idx += 2;
    break;
  case 2:
    output_quad[0] = ((unsigned char)buffer[aligned_3_byte_len + 0] >> 2);
    output_quad[1] = (((buffer[aligned_3_byte_len + 0] & 3) << 4) |
                      ((buffer[aligned_3_byte_len + 1] >> 4))) &
                     0x3f;
    output_quad[2] = (buffer[aligned_3_byte_len + 1] << 2) & 0x3f;
    _base64_value_for_data(output_quad, 3, output_buffer + output_buffer_idx);
    output_buffer_idx += 3;
    break;
  default: _LOG_DEBUG("remaining bytes: %lu", remaining_bytes); _UNREACHABLE();
  }
  assert(output_buffer_idx < output_len);

  if (settings.padding == BASE64_ENCODE_PADDING) {
    for (size_t i = 0; i < output_buffer_idx % 4; i++) {
      output_buffer[output_buffer_idx++] = '=';
      assert(output_buffer_idx <= output_len);
    }
  }
  *encoded_b64_len = output_buffer_idx;
  return BASE64_STATUS_SUCCESS;
}
BASE64_STATUS base64_decode(const char *__restrict buffer, size_t len,
                            char *__restrict output_buffer, size_t output_len,
                            size_t *encoded_b64_len) {

  // todo(shahzad)!: better names
  if (output_len < BASE64_DECODE_REQUIRED_OUTPUT_BUFFER_SIZE(len)) {
    return BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL;
  }
  const size_t remaining_bytes = len % 4;

  const size_t aligned_4_byte_len = len - remaining_bytes;
  size_t output_buffer_idx = 0;
  char bytes_4[4] = {0};
  char output_tri[3] = {0};
  for (size_t i = 0; i < aligned_4_byte_len; i += 4) {
    bytes_4[0] = buffer[i];
    bytes_4[1] = buffer[i + 1];
    bytes_4[2] = buffer[i + 2];
    bytes_4[3] = buffer[i + 3];

    _base64_to_data(bytes_4, 4, bytes_4);
    output_tri[0] = (char)(bytes_4[0] << 2) | (bytes_4[1] >> 4);
    output_tri[1] = (char)(bytes_4[1] << 4) | ((bytes_4[2] >> 2) & 0xf);
    output_tri[2] = (char)(bytes_4[2] << 6) | (bytes_4[3]);
    output_buffer[output_buffer_idx] = output_tri[0];
    output_buffer[output_buffer_idx + 1] = output_tri[1];
    output_buffer[output_buffer_idx + 2] = output_tri[2];

    output_buffer_idx += 3;
    assert(output_buffer_idx < output_len);
  }

  switch (remaining_bytes) {
  case 0: break;
  case 1: _UNREACHABLE("shouldn't be possible");
  case 2:
    bytes_4[0] = buffer[aligned_4_byte_len];
    bytes_4[1] = buffer[aligned_4_byte_len + 1];
    _base64_to_data(bytes_4, 2, bytes_4);
    output_buffer[output_buffer_idx] =
        (char)(bytes_4[0] << 2) | (bytes_4[1] >> 4);
    output_buffer[output_buffer_idx + 1] = (char)(bytes_4[1] << 4);
    output_buffer_idx += 2;

    break;
  case 3:
    bytes_4[0] = buffer[aligned_4_byte_len];
    bytes_4[1] = buffer[aligned_4_byte_len + 1];
    bytes_4[2] = buffer[aligned_4_byte_len + 2];

    _base64_to_data(bytes_4, 3, bytes_4);
    output_buffer[output_buffer_idx] =
        (char)(bytes_4[0] << 2) | (bytes_4[1] >> 4);
    output_buffer[output_buffer_idx + 1] =
        (char)(bytes_4[1] << 4) | ((bytes_4[2] >> 2) & 0x3);
    output_buffer[output_buffer_idx + 2] = (char)(bytes_4[2] << 6);

    output_buffer_idx += 3;
    break;
  default: _LOG_DEBUG("remaining bytes: %lu", remaining_bytes); _UNREACHABLE();
  }
  assert(output_buffer_idx < output_len);
  output_buffer[output_buffer_idx + 1] = 0;
  assert(output_buffer_idx < output_len);
  *encoded_b64_len = output_buffer_idx;
  return BASE64_STATUS_SUCCESS;
}
static __always_inline void
_base64_value_for_data(const char *data, size_t size, char *output_buffer) {
  for (size_t i = 0; i < size; i++) {
    if (data[i] < 26) {
      output_buffer[i] = (char)(data[i] + 'A');
    } else if (data[i] < 52) {
      output_buffer[i] = (char)((data[i] - 26) + 'a');
    } else if (data[i] < 62) {
      output_buffer[i] = (char)((data[i] - 52) + '0');
    } else if (data[i] == 62) {
      output_buffer[i] = '+';
    } else if (data[i] == 63) {
      output_buffer[i] = '/';
    } else {
      printf("%d\n", data[i]);
      printf("%c\n", data[i]);
      assert(0 && "unreachable");
    }
  }
}
static __always_inline void _base64_to_data(const char *data, size_t size,
                                            char *output_buffer) {
  for (size_t i = 0; i < size; i++) {
    if (data[i] >= 'A' && data[i] <= 'Z') {
      output_buffer[i] = data[i] - 'A';
    } else if (data[i] >= 'a' && data[i] <= 'z') {
      output_buffer[i] = data[i] - 'a' + 26;
    } else if (isdigit(data[i])) {
      output_buffer[i] = data[i] - '0' + 52;
    } else if (data[i] == '+') {
      output_buffer[i] = 62;
    } else if (data[i] == '/') {
      output_buffer[i] = 63;
    } else if (data[i] == '=') {
      break;
    } else {
      printf("%c\n", data[i]);
      printf("%d\n", data[i]);
      assert(0 && "unreachable");
    }
  }
}
