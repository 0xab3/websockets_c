#ifndef __SHITTY_AHH_UTILS_H__
#define __SHITTY_AHH_UTILS_H__

#include <assert.h>
#include <stdio.h>

#ifndef internal
#define internal static inline
#endif // internal

#ifndef opaque_ptr_t
#define opaque_ptr_t void *
#endif // opaque_ptr_t

#define _LOG_TODO(...)                                                         \
  do {                                                                         \
    fprintf(stderr, "TODO: %s(): ", __func__);                                 \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)

#define _LOG_DEBUG(...)                                                        \
  do {                                                                         \
    fprintf(stderr, "DEBUG: %s(): ", __func__);                                \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)

#define _LOG_WARN(...)                                                         \
  do {                                                                         \
    fprintf(stderr, "WARN: %s(): ", __func__);                                 \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#define _LOG_ERROR(...)                                                        \
  do {                                                                         \
    fprintf(stderr, "ERROR: %s(): ", __func__);                                \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)

#define _UNREACHABLE(...) assert(0 && "UNREACHABLE: "__VA_ARGS__)
#define _UNUSED(x) (void)x
#define ARRAY_LEN(xs) sizeof(xs) / sizeof(xs[0])
#define _TODO(...) assert(0 && "TODO: "__VA_ARGS__)

#define _LIKELY(x) __builtin_expect(!!(x), 1)
#define _UNLIKELY(x) __builtin_expect(!!(x), 0)

// why do even cstdlib exist =|
#define MAX(a, b) (((a) > (b) ? (a) : (b)))
#define MIN(a, b) (((a) < (b) ? (a) : (b)))

#define ROTL64(value, rotation)                                                \
  (((value) << (rotation)) | ((value) >> (64 - (rotation))))
#define ROTL32(value, rotation)                                                \
  (((value) << (rotation)) | ((value) >> (32 - (rotation))))

#endif // __SHITTY_AHH_UTILS_H__
