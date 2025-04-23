#ifndef __BASE64__
#define __BASE64__
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

typedef struct base64EncodeSettings {
  enum {
    BASE64_ENCODE_NO_PADDING,
    BASE64_ENCODE_PADDING,
  } padding;
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
static inline uint8_t _get_base64_value_for_data(uint8_t data);
void base64_decode(void);

BASE64_STATUS base64_encode_avx2(const uint8_t *__restrict buffer, size_t len,
                                 char *__restrict output_buffer,
                                 size_t output_len, size_t *encoded_b64_len,
                                 base64EncodeSettings settings) {
  if (output_len < (size_t)(((double)len / 3.0) * 4.0)) {
    return BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL;
  }
  size_t encoded_b64_writer_idx = 0;
  const size_t _6byte_aligned_len = len / 6;

  for (size_t i = 0; i < _6byte_aligned_len; i++) {
    const __m256i mm_buf_orig =
        _mm256_cvtepu8_epi32(_mm_loadu_si64((__m128i const *)buffer));

    const __m256i mm_buffer_1 = _mm256_permutevar8x32_epi32(
        mm_buf_orig, _mm256_setr_epi32(0, 0, 1, 2, 3, 3, 4, 5));
    const __m256i and_buffer =
        _mm256_broadcastsi128_si256(_mm_setr_epi32(0, 0x3, 0xf, 0x3f));
    const __m256i and_result = _mm256_and_si256(mm_buffer_1, and_buffer);

    const __m256i mm_buffer_2 = _mm256_permutevar8x32_epi32(
        mm_buf_orig, _mm256_setr_epi32(0, 1, 2, 2, 3, 4, 5, 5));

    __m256i shift_reg1 = _mm256_setr_epi32(8, 4, 2, 0, 8, 4, 2, 0);
    __m256i shift_reg2 = _mm256_setr_epi32(2, 4, 6, 8, 2, 4, 6, 8);

    const __m256i r1 = _mm256_sllv_epi32(and_result, shift_reg1);
    const __m256i r2 = _mm256_srlv_epi32(mm_buffer_2, shift_reg2);

    const __m256i r1_or_r2 = _mm256_or_si256(r1, r2);
    const __m256i final_and = _mm256_set1_epi8(0x3f);
    __m256i results = _mm256_and_si256(r1_or_r2, final_and);

    __m256i shuffle_mask = _mm256_setr_epi8(
        0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
      );
    __m256i shuffle_result = _mm256_shuffle_epi8(results, shuffle_mask);
    const __m256i packed8 = _mm256_permutevar8x32_epi32(
        shuffle_result, _mm256_setr_epi32(0, 0, 0, 0, 0, 4, 0, 0));

    const __m128i result = _mm256_extracti128_si256(packed8, 1);

    char data[8] = {0};
    _mm_storeu_si64(data, result);

    // todo(shahzad): add simd here
    for (size_t j = 0; j < 8; j++) {
      output_buffer[encoded_b64_writer_idx++] =
      (char)_get_base64_value_for_data((uint8_t)data[j]);
    }
    buffer += 6;
    len -= 6;
  }
  size_t remaining_length = 0;
  BASE64_STATUS b64_status = base64_encode(
      buffer, len, output_buffer + encoded_b64_writer_idx,
      output_len - encoded_b64_writer_idx, &remaining_length, settings);
  *encoded_b64_len = encoded_b64_writer_idx + remaining_length;
  return b64_status;
}

// todo(shahzad): this shit is so bad that i wanna kms, idk how to write it
// any better
BASE64_STATUS base64_encode(const uint8_t *__restrict buffer, size_t len,
                            char *__restrict output_buffer, size_t output_len,
                            size_t *encoded_b64_len,
                            base64EncodeSettings settings) {
  if (output_len < (size_t)(((double)len / 3.0) * 4.0)) {
    return BASE64_STATUS_ERROR_DESTINATION_BUFFER_TOO_SMALL;
  }
  const size_t remaining_bytes = len % 3;
  const size_t aligned_3_byte_len = len - remaining_bytes;
  size_t output_buffer_idx = 0;
  uint8_t bytes_3[3] = {0};
  for (size_t i = 0; i < aligned_3_byte_len; i += 3) {
    bytes_3[0] = buffer[i];
    bytes_3[1] = buffer[i + 1];
    bytes_3[2] = buffer[i + 2];
    output_buffer[output_buffer_idx++] =
        (char)_get_base64_value_for_data((bytes_3[0] >> 2) & 0x3f);

    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        (((bytes_3[0] & 3) << 4) | ((bytes_3[1] >> 4))) & 0x3f);

    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        (((bytes_3[1] & 0xf) << 2) | (bytes_3[2] >> 6)) & 0x3f);

    output_buffer[output_buffer_idx++] =
        (char)_get_base64_value_for_data(bytes_3[2] & 0x3f);
    assert(output_buffer_idx < output_len);
  }

  printf("=====================================\n");
  switch (remaining_bytes) {
  case 0:
    break;
  case 1:
    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        (buffer[aligned_3_byte_len + 0] >> 2) & 0x3f);
    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        (buffer[aligned_3_byte_len + 0] << 4) & 0x3f);
    break;
  case 2:
    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        (buffer[aligned_3_byte_len + 0] >> 2) & 0x3f);
    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        ((buffer[aligned_3_byte_len + 0] << 4 |
          (buffer[aligned_3_byte_len + 1] >> 4))) &
        0x3f);
    output_buffer[output_buffer_idx++] = (char)_get_base64_value_for_data(
        (buffer[aligned_3_byte_len + 1] << 2) & 0x3f);
    break;
  default:
    _LOG_DEBUG("remaining bytes: %lu\n", remaining_bytes);
    _UNREACHABLE();
  }
  assert(output_buffer_idx < output_len);

  if (settings.padding == BASE64_ENCODE_PADDING) {
    for (size_t i = 0; i < output_buffer_idx % 4; i++) {
      output_buffer[output_buffer_idx++] = '=';
      assert(output_buffer_idx < output_len);
    }
  }
  output_buffer[output_buffer_idx++] = 0;
  assert(output_buffer_idx < output_len);
  *encoded_b64_len = output_buffer_idx;
  return BASE64_STATUS_SUCCESS;
}
static inline uint8_t _get_base64_value_for_data(uint8_t data) {
  const char chr_data = (char)data;
  if (chr_data < 26) {
    return (uint8_t)chr_data + 'A';
  } else if (chr_data < 52) {
    return (uint8_t)(chr_data - 26) + 'a';
  } else if (chr_data < 62) {
    return (uint8_t)(chr_data - 52) + '0';
  } else if (chr_data == 62) {
    return '+';
  } else if (chr_data == 63) {
    return '/';
  } else {
    printf("%d\n", chr_data);
    assert(0 && "unreachable");
  }
}
void base64_decode(void) {}

#endif // __BASE64__
