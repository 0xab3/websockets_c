#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

#include <stddef.h>
#include <stdlib.h>

#define DA_MAX_CAPACITY 256
#define DA_MALLOC(ctx, ...) malloc(__VA_ARGS__)
#define DA_REALLOC(ctx, ...) realloc(__VA_ARGS__)
#define DA_FREE(ctx, ...) free(__VA_ARGS__)

#define da_new(T)                                                              \
  typedef struct {                                                             \
    T *items;                                                                  \
    size_t len;                                                                \
    size_t capacity;                                                           \
  } Dyn_Array_##T

#define FOREACH_DA_ERROR(GEN_FUNC)                                             \
  GEN_FUNC(DA_SUCCESS)                                                         \
  GEN_FUNC(DA_FULL)

#define ENUM_GENERATOR(ENUM_FIELD) ENUM_FIELD,
#define STRING_GENERATOR(ENUM_FIELD) #ENUM_FIELD,

typedef enum { FOREACH_DA_ERROR(ENUM_GENERATOR) } DA_ERROR;
static const char *Da_Error_Strs[] = {FOREACH_DA_ERROR(STRING_GENERATOR)};
#define DA_ErrToStr(DA_ERR) Da_Error_Strs[DA_ERR]

// note(0xab3): everything below is kinda shit but idk how to do it in a better
// way

#define da_append(xs, x, __result)                                             \
  __result = DA_SUCCESS;                                                       \
  if (xs.len >= xs.capacity) {                                                 \
    if (xs.capacity < DA_MAX_CAPACITY) {                                       \
      if (xs.capacity == 0)                                                    \
        xs.capacity = 8;                                                       \
      else                                                                     \
        xs.capacity *= 2;                                                      \
      xs.items = DA_REALLOC(NULL, xs.items, xs.capacity);         \
    } else {                                                                   \
      __result = DA_FULL;                                                      \
    }                                                                          \
  }                                                                            \
  if (__result == DA_SUCCESS)                                                  \
    xs.items[xs.len++] = x;

#define da_remove_swap(xs, index)                                              \
  do {                                                                         \
    xs.items[index] = xs.items[xs.len];                                        \
    xs.len -= 1;                                                               \
  } while (0);

#define da_remove_ordered(xs, index)                                           \
  do {                                                                         \
    if (index <= xs.len) {                                                     \
      if (index < xs.len) {                                                    \
        memmove(xs.items + index, xs.items + (index + 1),                      \
                xs.items + xs.len - xs.items + (index + 1));                   \
      }                                                                        \
      xs.len--;                                                                \
    }                                                                          \
  } while (0)

#define da_pop(xs) xs.len < 1 ? NULL : xs.items[--xs.len]

#define da_deinit(array)                                                       \
  do {                                                                         \
    DA_FREE(NULL, array.elem);                                                 \
    array.len = 0;                                                             \
    array.capacity = 0;                                                        \
  } while (0)

#endif
