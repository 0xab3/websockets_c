#include <stdint.h>

typedef struct splitmix64Ctx {
  uint64_t seed;
}splitmix64Ctx;

splitmix64Ctx splitmix64_init(uint64_t seed);
uint64_t splitmix64_next(splitmix64Ctx *ctx);

splitmix64Ctx splitmix64_init(uint64_t seed){
  return (splitmix64Ctx){.seed = seed};
}
uint64_t splitmix64_next(splitmix64Ctx *ctx) {
  uint64_t z = (ctx->seed += 0x9E3779B97F4A7C15);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
  return z ^ (z >> 31);
}
