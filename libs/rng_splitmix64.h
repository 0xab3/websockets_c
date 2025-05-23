#include <stdint.h>

typedef struct splitmix64Context {
  uint64_t seed;
} splitmix64Context;

splitmix64Context splitmix64_init(uint64_t seed);
uint64_t splitmix64_next(splitmix64Context *ctx);

splitmix64Context splitmix64_init(uint64_t seed) {
  return (splitmix64Context){.seed = seed};
}
uint64_t splitmix64_next(splitmix64Context *ctx) {
  uint64_t z = (ctx->seed += 0x9E3779B97F4A7C15);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
  return z ^ (z >> 31);
}
