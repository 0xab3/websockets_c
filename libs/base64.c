#include "./base64.h"
#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>

static __always_inline void base64_value_for_data(const char *data, size_t size,
                                                  char *output_buffer);
// bruh this shit slower then the compiler vectorized version, i am gonna kms.
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
    const __m256i input_bytes_32 =
        _mm256_cvtepu8_epi32(_mm_loadu_si64((__m128i const *)buffer));

    const __m256i input_bytes_16_1 = _mm256_permutevar8x32_epi32(
        input_bytes_32, _mm256_setr_epi32(0, 0, 1, 2, 3, 3, 4, 5));
    const __m256i input_bytes_16_1_and_mask =
        _mm256_broadcastsi128_si256(_mm_setr_epi32(0, 0x3, 0xf, 0x3f));
    const __m256i input_bytes_16_1_and_result =
        _mm256_and_si256(input_bytes_16_1, input_bytes_16_1_and_mask);

    const __m256i input_bytes_16_2 = _mm256_permutevar8x32_epi32(
        input_bytes_32, _mm256_setr_epi32(0, 1, 2, 2, 3, 4, 5, 5));

    __m256i input_byte_16_1_shift_mask =
        _mm256_setr_epi32(8, 4, 2, 0, 8, 4, 2, 0);
    __m256i input_byte_16_2_shift_mask =
        _mm256_setr_epi32(2, 4, 6, 8, 2, 4, 6, 8);

    const __m256i input_byte_16_1_shift_result = _mm256_sllv_epi32(
        input_bytes_16_1_and_result, input_byte_16_1_shift_mask);
    const __m256i input_byte_16_2_shift_result =
        _mm256_srlv_epi32(input_bytes_16_2, input_byte_16_2_shift_mask);

    const __m256i _16_1_or_16_2 = _mm256_or_si256(input_byte_16_1_shift_result,
                                                  input_byte_16_2_shift_result);
    const __m256i final_and_mask = _mm256_set1_epi8(0x3f);
    __m256i results = _mm256_and_si256(_16_1_or_16_2, final_and_mask);

    // note(shahzad): result is in 32bit and we want to pack it to 8 bits
    // removing everything but the least significant byte, that's why we use
    // this this shuffle mask
    __m256i shuffle_mask = _mm256_setr_epi8(
        0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8,
        12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    __m256i shuffle_result = _mm256_shuffle_epi8(results, shuffle_mask);
    const __m256i packed8 = _mm256_permutevar8x32_epi32(
        shuffle_result, _mm256_setr_epi32(0, 0, 0, 0, 0, 4, 0, 0));

    const __m128i result = _mm256_extracti128_si256(packed8, 1);

    char data[8] = {0};
    _mm_storeu_si64(&data, result);

    base64_value_for_data(data, 8, output_buffer + encoded_b64_writer_idx);
    encoded_b64_writer_idx += 8;
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
  char output_quad[4] = {0};
  for (size_t i = 0; i < aligned_3_byte_len; i += 3) {
    bytes_3[0] = buffer[i];
    bytes_3[1] = buffer[i + 1];
    bytes_3[2] = buffer[i + 2];

    output_quad[0] = (bytes_3[0] >> 2) & 0x3f;
    output_quad[1] = (((bytes_3[0] & 3) << 4) | ((bytes_3[1] >> 4))) & 0x3f;
    output_quad[2] = (((bytes_3[1] & 0xf) << 2) | (bytes_3[2] >> 6)) & 0x3f;
    output_quad[3] = bytes_3[2] & 0x3f;
    base64_value_for_data(output_quad, 4, output_buffer);
    output_buffer_idx += 4;
    assert(output_buffer_idx < output_len);
  }

  switch (remaining_bytes) {
  case 0: break;
  case 1:
    output_quad[0] = (buffer[aligned_3_byte_len + 0] >> 2) & 0x3f;
    output_quad[1] = (buffer[aligned_3_byte_len + 0] << 4) & 0x3f;
    base64_value_for_data(output_quad, 2, output_buffer);
    break;
  case 2:
    output_quad[0] = (buffer[aligned_3_byte_len + 0] >> 2) & 0x3f;
    output_quad[1] = (buffer[aligned_3_byte_len + 0] << 4) & 0x3f;
    output_quad[2] = (buffer[aligned_3_byte_len + 1] << 2) & 0x3f;
    base64_value_for_data(output_quad, 3, output_buffer);
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
  output_buffer[output_buffer_idx + 1] = 0;
  assert(output_buffer_idx < output_len);
  *encoded_b64_len = output_buffer_idx;
  return BASE64_STATUS_SUCCESS;
}
static __always_inline void base64_value_for_data(const char *data, size_t size,
                                                  char *output_buffer) {

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
      assert(0 && "unreachable");
    }
  }
}
void base64_decode(void) {}
