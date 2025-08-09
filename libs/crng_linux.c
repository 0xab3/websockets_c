#ifndef __CRNG_LINUX_H__
#define __CRNG_LINUX_H__
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

typedef struct CryptoRngContext {
  int crng_fd;
} CryptoRngContext;

CryptoRngContext crng_init(void);
int64_t crng_next_i64(CryptoRngContext *ctx);
int32_t crng_next_i32(CryptoRngContext *ctx);
void crng_deinit(CryptoRngContext *ctx);

CryptoRngContext crng_init(void) {
  const int crng_ctx = open("/dev/urandom", O_RDONLY);
  return (CryptoRngContext){.crng_fd = crng_ctx};
}
int64_t crng_next_i64(CryptoRngContext *ctx) {
  int64_t key = 0;
  assert(read(ctx->crng_fd, &key, sizeof(key)) == 8);
  return key;
}

int32_t crng_next_i32(CryptoRngContext *ctx) {
  int32_t key = 0;
  assert(read(ctx->crng_fd, &key, sizeof(key)) == 4);
  return key;
}
void crng_deinit(CryptoRngContext *ctx) {
  close(ctx->crng_fd);
  ctx->crng_fd = -1;
}
#endif // __CRNG_LINUX_H__
