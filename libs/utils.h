#ifndef __SHITTY_AHH_UTILS_H__
#define __SHITTY_AHH_UTILS_H__

#include <assert.h>
#include <stdio.h>
#define _LOG_DEBUG(...)                                                        \
  do {                                                                         \
    fprintf(stderr, "DEBUG: %s(): ", __func__);                                \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)

#define _LOG_WARN(...)                                                         \
  do {                                                                         \
    fprintf(stderr, "WARN: %s(): ", __func__);                                 \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#define _LOG_ERROR(...)                                                        \
  do {                                                                         \
    fprintf(stderr, "ERROR: %s(): ", __func__);                                \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)

#define _UNREACHABLE(...) assert(0 && "UNREACHABLE: "__VA_ARGS__)
#define _UNUSED(x) (void)x
#define ARRAY_LEN(xs) sizeof(xs) / sizeof(xs[0])
#define _TODO(...) assert(0 && "TODO: "__VA_ARGS__)

// why do even cstdlib exist =|
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif // __SHITTY_AHH_UTILS_H__
