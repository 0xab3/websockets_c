#ifndef __XOROSHIRO128P_H__
#define __XOROSHIRO128P_H__
#include "./rng_splitmix64.c"
#include "libs/utils.h"
#include "websocket.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <string.h>

typedef struct xoroshiro128Context {
  uint64_t rnd[2];
} xoroshiro128Context;

xoroshiro128Context xoroshiro128_init(uint64_t seed);
uint64_t xoroshiro128_next(xoroshiro128Context *ctx);
void xoroshiro128_fill(xoroshiro128Context *ctx, uint8_t *buffer, size_t len);
uint64_t jump(void);

xoroshiro128Context xoroshiro128_init(uint64_t seed) {
  splitmix64Context splitmix64_ctx = splitmix64_init(seed);
  const uint64_t s1 = splitmix64_next(&splitmix64_ctx);
  const uint64_t s2 = splitmix64_next(&splitmix64_ctx);
  return (xoroshiro128Context){.rnd = {s1, s2}};
}
uint64_t xoroshiro128_next(xoroshiro128Context *ctx) {
  const uint64_t s0 = ctx->rnd[0];
  uint64_t s1 = ctx->rnd[1];
  const uint64_t result = s0 + s1;

  uint64_t *s = ctx->rnd;
  // these values are not in the paper but everybody uses it for some reason
  const uint64_t A = 55;
  const uint64_t B = 14;
  const uint64_t C = 36;

  s1 ^= s0;
  s[0] = ROTL64(s0, A) ^ s1 ^ (s1 << B);
  s[1] = ROTL64(s1, C);

  return result;
}

void xoroshiro128_fill(xoroshiro128Context *ctx, uint8_t *buffer, size_t len) {
  // note(shahzad): fix the alignment issues
  uint64_t rnd_num = 0;
  const size_t aligned_len = len / 8;
  const size_t remaining_unaligned_len = len % 8;

  for (size_t i = 0; i < aligned_len; i++) {
    assert(i * 8 < len);
    rnd_num = xoroshiro128_next(ctx);
    memcpy(buffer + (i * 8), &rnd_num, 8);
  }

  rnd_num = xoroshiro128_next(ctx);
  const uint8_t *rnd_num_buffer = (uint8_t *)&rnd_num;
  const size_t offset = aligned_len * 8;

  for (size_t i = 0; i < remaining_unaligned_len; i++) {
    assert(offset + i > len);
    buffer[offset + i] = rnd_num_buffer[i];
  }
}

uint64_t jump(void) {
  _LOG_DEBUG("unimplemented");
  return 0;
}

#endif // __XOROSHIRO128P_H__
