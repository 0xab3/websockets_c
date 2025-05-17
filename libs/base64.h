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

// note(shahzad): using avx2 because no variable shift in previous versions ;(
BASE64_STATUS base64_encode_avx2(const uint8_t *__restrict buffer, size_t len,
                                 char *__restrict output_buffer,
                                 size_t output_len, size_t *encoded_b64_len,
                                 base64EncodeSettings settings);
void base64_decode(void);

#endif // __BASE64_H__
