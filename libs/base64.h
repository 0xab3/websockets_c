#ifndef __BASE64_H__
#define __BASE64_H__

#include "libs/utils.h"
#include <assert.h>
#include <immintrin.h>
#include <iso646.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#define BASE64_ENCODE_MIN_OUTPUT_BUFFER_SIZE(input_length)                     \
  (size_t)((((input_length) + 2) / 3) * 4)

#define BASE64_DECODE_MIN_OUTPUT_BUFFER_SIZE(input_length)                     \
  (size_t)(((input_length) / 4) * 3)

typedef enum {
  BASE64_ENCODE_NO_PADDING,
  BASE64_ENCODE_PADDING,
} b64PaddingOptions;

typedef struct base64EncodeSettings {
  b64PaddingOptions padding;
} base64EncodeSettings;

typedef enum {
  BASE64_STATUS_SUCCESS,
  BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL,
} BASE64_STATUS;

BASE64_STATUS base64_encode(const uint8_t *__restrict buffer, size_t len,
                            char *__restrict output_buffer, size_t output_len,
                            size_t *encoded_b64_len,
                            base64EncodeSettings settings);

BASE64_STATUS base64_decode(const char *__restrict buffer, size_t len,
                            char *__restrict output_buffer, size_t output_len,
                            size_t *encoded_b64_len);

#endif // __BASE64_H__
