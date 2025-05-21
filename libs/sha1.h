#ifndef __SHA1_H__
#define __SHA1_H__

#include "libs/extint.h"
#include "libs/utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define SHA1_ALLOC(allocator_ctx, ...) malloc(__VA_ARGS__);
#define SHA1_FREE(allocator_ctx, ...) free(__VA_ARGS__);
typedef struct sha1Digest {
  union {
    uint32_t u32[5];
    uint8_t u8[20];
  } _As;
} sha1Digest;

sha1Digest sha1_to_le(sha1Digest);
void W_t(uint32_t *wtd_block, const uint32_t *block);
uint32_t K_t(uint32_t t);
uint32_t F_t(uint32_t t, uint32_t B, uint32_t C, uint32_t D);

sha1Digest sha1_digest(opaque_ptr_t allocator_ctx, const uint8_t *msg,
                       size_t msg_len);
uint8_t *sha1_alloc_padded_message(opaque_ptr_t allocator_ctx,
                                   const uint8_t *msg, uint64_t msg_len,
                                   uint64_t *padded_len);

static inline size_t sha1_required_message_len(size_t len) {
  const size_t required_clamping_bytes = 64 - (len % 64);
  // 64bit of length and 8 bit of extra 1 that is needed
  if (required_clamping_bytes < 9) {
    len += 64;
  }
  len += required_clamping_bytes;

  return len;
}

void W_t(uint32_t *wtd_block, const uint32_t *block) {
  for (size_t t = 0; t < 16; t++) {
    wtd_block[t] = block[t];
  }
  for (size_t t = 16; t < 80; t++) {
    wtd_block[t] = ROTL32(wtd_block[t - 3] ^ wtd_block[t - 8] ^
                              wtd_block[t - 14] ^ wtd_block[t - 16],
                          1);
  }
  return;
}
inline uint32_t F_t(uint32_t t, uint32_t B, uint32_t C, uint32_t D) {
  if (t <= 19)
    return (B & C) | ((~B) & D);
  else if (t <= 39)
    return B ^ C ^ D;
  else if (t <= 59)
    return (B & C) | (B & D) | (C & D);
  else if (t <= 79)
    return B ^ C ^ D;
  _UNREACHABLE("t out of range: t <= 79");
}
inline uint32_t K_t(uint32_t t) {
  if (t <= 19)
    return 0x5A827999;
  if (t <= 39)
    return 0x6ED9EBA1;
  if (t <= 59)
    return 0x8F1BBCDC;
  if (t <= 79)
    return 0xCA62C1D6;
  _UNREACHABLE("t out of range: t <= 79");
}
uint8_t *sha1_alloc_padded_message(opaque_ptr_t allocator_ctx,
                                   const uint8_t *msg, uint64_t msg_len,
                                   uint64_t *padded_len) {
  _UNUSED(allocator_ctx);

  const size_t bit_len = msg_len * 8;
  const size_t new_len = sha1_required_message_len(msg_len);
  *padded_len = new_len;

  assert(new_len != msg_len);
  assert(new_len % 64 == 0);

  uint8_t *new_msg = SHA1_ALLOC(allocator_ctx, new_len);
  if (new_msg == NULL) {
    return NULL;
  }

  memcpy(new_msg, msg, msg_len);
  new_msg[msg_len] = 0x80;
  memset(new_msg + msg_len + 1, 0, new_len - msg_len - 1);

  write_be64(new_msg + new_len - 8, bit_len);
  return new_msg;
}

sha1Digest sha1_digest(opaque_ptr_t allocator_ctx, const uint8_t *msg,
                       size_t msg_len) {
  sha1Digest result = {0};
  uint32_t H[] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

  uint64_t padded_len = 0;
  uint8_t *padded_msg =
      sha1_alloc_padded_message(allocator_ctx, msg, msg_len, &padded_len);
  if (padded_msg == NULL) {
    return (sha1Digest){0};
  }
  assert(padded_len != 0);
  assert(padded_len % 64 == 0);

  uint32_t block[80] = {0};
  uint32_t W[16] = {0};

  for (size_t i = 0; i < padded_len; i += 64) {

    for (int j = 0; j < 16; j++) {
      W[j] = read_be32(padded_msg + i + j * 4);
    }

    W_t(block, W);

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

void sha1_print_digest(const sha1Digest *digest);
void sha1_print_digest(const sha1Digest *digest) {
  for (int i = 0; i < 5; ++i) {
    printf("%08x", digest->_As.u32[i]);
  }
  printf("\n");
}
#endif
