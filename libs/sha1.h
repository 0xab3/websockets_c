#ifndef __SHA1_H__
#define __SHA1_H__

#include "libs/extint.h"
#include "libs/utils.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct sha1Digest {
  union {
    uint32_t u32[5];
    uint8_t u8[20];
  } _As;
} sha1Digest;

typedef enum SHA1_STATUS {
  SHA1_NO_REQUIRE_ANOTHER_BLOCK = 0,
  SHA1_REQUIRE_ANOTHER_BLOCK = 1,
} SHA1_STATUS;

sha1Digest sha1_digest(const uint8_t *msg, size_t msg_len);
sha1Digest sha1_to_le(sha1Digest);
void W_t(uint32_t *wtd_block, const uint32_t *block);
uint32_t K_t(uint32_t t);
uint32_t F_t(uint32_t t, uint32_t B, uint32_t C, uint32_t D);
void sha1_print_digest(const sha1Digest *digest);

static inline void read_block_be32(const uint8_t *src, uint32_t *dst,
                                   size_t words_count) {
  for (size_t i = 0; i < words_count; i++) {
    dst[i] = read_be32(src + i * 4);
  }
}

static inline SHA1_STATUS sha1_require_another_block(size_t block_length) {
  const size_t required_clamping_bytes = 64 - (block_length % 64);
  if (required_clamping_bytes < 9) {
    return SHA1_REQUIRE_ANOTHER_BLOCK;
  }
  return SHA1_NO_REQUIRE_ANOTHER_BLOCK;
}

void W_t(uint32_t *W, const uint32_t *block) {
  for (size_t t = 0; t < 16; t++) {
    W[t] = block[t];
  }
  for (size_t t = 16; t < 80; t++) {
    W[t] = ROTL32(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
  }
  return;
}
inline uint32_t F_t(uint32_t t, uint32_t B, uint32_t C, uint32_t D) {
  if (t <= 19) return (B & C) | ((~B) & D);
  else if (t <= 39) return B ^ C ^ D;
  else if (t <= 59) return (B & C) | (B & D) | (C & D);
  else if (t <= 79) return B ^ C ^ D;
  _UNREACHABLE("t out of range: t <= 79");
}
inline uint32_t K_t(uint32_t t) {
  if (t <= 19) return 0x5A827999;
  if (t <= 39) return 0x6ED9EBA1;
  if (t <= 59) return 0x8F1BBCDC;
  if (t <= 79) return 0xCA62C1D6;
  _UNREACHABLE("t out of range: t <= 79");
}
static inline void sha1_process_block(uint32_t *block, uint32_t *H) {
  uint32_t A = H[0], B = H[1], C = H[2], D = H[3], E = H[4];
  for (uint32_t t = 0; t < 80; t++) {
    uint32_t TEMP = ROTL32(A, 5) + F_t(t, B, C, D) + E + block[t] + K_t(t);
    E = D;
    D = C;
    C = ROTL32(B, 30);
    B = A;
    A = TEMP;
  }
  H[0] = H[0] + A;
  H[1] = H[1] + B;
  H[2] = H[2] + C;
  H[3] = H[3] + D;
  H[4] = H[4] + E;
}

sha1Digest sha1_digest(const uint8_t *msg, size_t msg_len) {
  sha1Digest result = {0};
  uint32_t H[] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

  const size_t bit_len = msg_len * 8;
  size_t aligned_length = msg_len - (msg_len % 64);
  size_t remaining_byte_length = msg_len - aligned_length;
  uint32_t block[80] = {0};

  // W only fills 64 bytes (16 words) the extra are for edge case
  // where you require additional block as block doesn't contain
  // 9 free bytes to 0x80 put the length in
  union {
    uint8_t as_bytes[128];
    uint32_t as_words[32];
  } W = {0};

  assert(aligned_length % 64 == 0);
  assert(remaining_byte_length < 64);

  for (size_t i = 0; i < aligned_length; i += 64) {
    read_block_be32(msg + i, W.as_words, 16);
    W_t(block, W.as_words);
    sha1_process_block(block, H);
  }
  SHA1_STATUS extra_block_required =
      sha1_require_another_block(remaining_byte_length);
  const size_t n_blocks = 1 + extra_block_required;

  memcpy(W.as_bytes, msg + aligned_length, remaining_byte_length);
  memset(W.as_bytes + remaining_byte_length, 0, 128 - remaining_byte_length);
  W.as_bytes[remaining_byte_length++] = 0x80;
  if (n_blocks == 1) {
    write_be64(W.as_bytes + 56, bit_len);
  } else if (n_blocks == 2) {
    write_be64(W.as_bytes + 120, bit_len);
  } else {
    _UNREACHABLE();
  }
  uint32_array_to_be_bytes(W.as_words, sizeof(W.as_words) / 4, W.as_bytes);
  W_t(block, W.as_words);
  sha1_process_block(block, H);
  if (n_blocks == 2) {
    W_t(block, W.as_words + 16);
    sha1_process_block(block, H);
  }

  result._As.u32[0] = H[0];
  result._As.u32[1] = H[1];
  result._As.u32[2] = H[2];
  result._As.u32[3] = H[3];
  result._As.u32[4] = H[4];
  return result;
}
sha1Digest sha1_to_le(sha1Digest digest) {
  digest._As.u32[0] = to_le32(digest._As.u32[0]);
  digest._As.u32[1] = to_le32(digest._As.u32[1]);
  digest._As.u32[2] = to_le32(digest._As.u32[2]);
  digest._As.u32[3] = to_le32(digest._As.u32[3]);
  digest._As.u32[4] = to_le32(digest._As.u32[4]);
  return digest;
}

void sha1_print_digest(const sha1Digest *digest) {
  fprintf(stderr, "DEBUG: sha1_print_digest(): ");
  for (int i = 0; i < 5; ++i) {
    printf("%08x", digest->_As.u32[i]);
  }
  printf("\n");
}
#endif
