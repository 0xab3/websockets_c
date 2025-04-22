#ifndef __BASE64__
#define __BASE64__
#include "libs/utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef enum {
  BASE64_STATUS_SUCCESS,
  BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL,
} BASE64_STATUS;
BASE64_STATUS base64_encode(const uint8_t *__restrict buffer, size_t len,
                           char *__restrict output_buffer, size_t output_len,
                           size_t *encoded_b64_len);
static inline uint8_t _get_base64_value_for_data(uint8_t data);
void base64_decode(void);

// todo(shahzad): this shit is so bad that i wanna kms, idk how to write it
// any better
BASE64_STATUS base64_encode(const uint8_t *__restrict buffer, size_t len,
                           char *__restrict output_buffer, size_t output_len,
                           size_t *encoded_b64_len) {
  if (output_len < (size_t)(((double)len / 3.0) * 4.0)) {
    return BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL;
  }
  uint8_t remainder = 0;
  uint8_t remainder_shift = 8;
  uint8_t buffer_shift = 0;
  uint32_t extract_remainder_shift = 0x0;
  size_t output_buffer_idx = 0;
  for (size_t i = 0; i < len; i++) {
    if (remainder_shift == 1) {
      remainder_shift = 0;
    }
    buffer_shift += 2;
    const uint8_t c1 =
        (buffer[i] >> buffer_shift | (remainder << remainder_shift)) & 63;

    extract_remainder_shift = ((extract_remainder_shift << 2) | 3);

    if (extract_remainder_shift > 0x3f) {
      extract_remainder_shift = 0x0;
      i -= 1;
    }

    remainder = buffer[i] & extract_remainder_shift;

    remainder_shift >>= 1;

    buffer_shift %= 8;
    if (remainder_shift == 0) {
      remainder_shift = 8;
    }
    output_buffer[output_buffer_idx] = _get_base64_value_for_data(c1);
    output_buffer_idx++;
    assert(output_buffer_idx < output_len);
  }
  if (remainder_shift == 1) {
    remainder_shift = 0;
  }
  assert((remainder << remainder_shift) < 64);
  output_buffer[output_buffer_idx++] =
      _get_base64_value_for_data((remainder << remainder_shift) & 0x63);
  assert(output_buffer_idx < output_len);

  for (size_t i = 0; i < output_buffer_idx % 3; i++) {
    output_buffer[output_buffer_idx++] = '=';
    assert(output_buffer_idx < output_len);
  }

  output_buffer[output_buffer_idx++] = 0;
  *encoded_b64_len = output_buffer_idx;

  return BASE64_STATUS_SUCCESS;
}

static inline uint8_t _get_base64_value_for_data(uint8_t data) {
  if (data < 26) {
    return data + 'A';
  } else if (data < 52) {
    return (data - 26) + 'a';
  } else if (data < 62) {
    return (data - 52) + '0';
  } else if (data == 62) {
    return '+';
  } else if (data == 63) {
    return '/';
  } else {
    printf("%d\n", data);
    assert(0 && "unreachable");
  }
}
void base64_decode(void) {}

#endif // __BASE64__
