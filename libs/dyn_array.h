#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

#include <stddef.h>
#include <stdlib.h>

#define DA_MALLOC(ctx, ...) malloc(__VA_ARGS__)
#define DA_REALLOC(ctx, ...) realloc(__VA_ARGS__)
#define DA_FREE(ctx, ...) free(__VA_ARGS__)

#ifndef opaque_ptr_t
#define opaque_ptr_t void *
#endif // opaque_ptr_t

#define da_type_new(T)                                                         \
  {                                                                            \
    opaque_ptr_t allocator_ctx;                                                \
    T *items;                                                                  \
    size_t len;                                                                \
    size_t capacity;                                                           \
  }

// note(shahzad): don't look at me we are not programming in zig here
#define da_init(capacity, element_size)                                        \
  {.items = DA_MALLOC(NULL, capacity * element_size),                          \
   .len = 0,                                                                   \
   .capacity = capacity}

// note(shahzad): everything below is kinda shit but idk how to do it in a
// better way

#define da_append(xs, x, __result_ptr)                                         \
  do {                                                                         \
    __result_ptr = NULL;                                                       \
    void *tmp_realloc = NULL;                                                  \
    if ((xs).len >= (xs).capacity) {                                           \
      if ((xs).capacity == 0)                                                  \
        (xs).capacity = 8;                                                     \
      else                                                                     \
        (xs).capacity *= 2;                                                    \
      tmp_realloc = DA_REALLOC((xs).allocator_ctx, (xs).items,                 \
                               (xs).capacity * (sizeof(*(xs).items)));         \
      if (tmp_realloc != NULL) {                                               \
        /* NOTE: this brakes cpp compatibility, but if you use cpp then don't  \
         * use this library*/                                                  \
        __result_ptr = tmp_realloc;                                            \
        (xs).items = tmp_realloc;                                              \
      }                                                                        \
    }                                                                          \
    if (__result_ptr == tmp_realloc) {                                         \
      (xs).items[(xs).len++] = x;                                              \
      __result_ptr = (xs).items + (xs).len;                                    \
    }                                                                          \
  } while (0);

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

#define da_deinit(alloc_ctx, array)                                            \
  do {                                                                         \
    DA_FREE(alloc_ctx, array.elem);                                            \
    array.len = 0;                                                             \
    array.capacity = 0;                                                        \
  } while (0)

#endif
