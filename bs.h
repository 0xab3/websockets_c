#ifndef _BS_H
#define _BS_H

#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>

#ifndef BSDEF
#define BSDEF extern
#endif

typedef struct {
  const char *view;
  size_t len;
} BetterString_View;

#define BS_ALLOC(allocator_ctx, ...) malloc(__VA_ARGS__);
#define BS_REALLOC(allocator_ctx, ...) realloc(__VA_ARGS__);
#define BS_FREE(allocator_ctx, ...) free(__VA_ARGS__);

#ifndef opaque_ptr_t
#define opaque_ptr_t void *
#endif

typedef struct {
  opaque_ptr_t allocator_ctx;
  struct {
    char *data;
    size_t len;
    size_t capacity;
  } string;
} BetterString_Builder;

typedef enum {
  BS_STATUS_OK,
  BS_STATUS_ALLOCATOR_OUT_OF_MEMORY,
  BS_STATUS_COUNT,
} BS_BUILDER_STATUS;

static const char *Bs_Builder_Status_Strs[] __attribute__((unused)) = {
    "BS_OK",
    "BS_ALLOCATOR_OUT_OF_MEMORY",
};
#define Bs_Builder_ErrToStr(BS_STATUS) Bs_Builder_Status_Strs[WS_ERR]

_Static_assert(BS_STATUS_COUNT == ARRAY_LEN(Bs_Builder_Status_Strs),
               "self explainatory");

// stolen from tsoding
#define BS_Fmt "%*.s"
#define BS_Arg(sv) (int)(sv).len, (sv).data
#define BS(cstr) bsv_from_str(cstr, strlen(cstr))

BSDEF BetterString_View bsv_from_str(char *data, size_t len);
BSDEF BetterString_Builder bs_builder_new(opaque_ptr_t allocator_ctx);
BSDEF BetterString_View bs_builder_to_sv(BetterString_Builder *builder);
BSDEF BS_BUILDER_STATUS bs_builder_append_sv(BetterString_Builder *builder,
                                             BetterString_View string);

// append cstr to bs
BSDEF BS_BUILDER_STATUS bs_builder_append_cstr(BetterString_Builder *builder,
                                               const char *cstr);
#endif

#ifdef BS_IMPLEMENTATION

BSDEF BetterString_View bsv_from_str(char *data, size_t len) {
  return (BetterString_View){.view = data, .len = len};
}

BSDEF BetterString_Builder bs_builder_new(opaque_ptr_t allocator_ctx) {
  assert(allocator_ctx != NULL);
  return (BetterString_Builder){
      .string = {.data = NULL, .len = 0},
      .allocator_ctx = allocator_ctx,
  };
}
BSDEF BetterString_View bs_builder_to_sv(BetterString_Builder *builder) {
  return (BetterString_View){.view = builder->string.data,
                             .len = builder->string.len};
}

// type conversion is ass so implementing this shit
static inline size_t log2i(size_t number) {
  size_t ret = 0;
  while (number != 0) {
    number = number >> 1;
    ret++;
  }
  return ret;
}

BSDEF BS_BUILDER_STATUS bs_builder_append_sv(BetterString_Builder *builder,
                                             const BetterString_View string) {
  const size_t available = builder->string.capacity - builder->string.len;
  if (available < string.len) {
    const size_t new_capacity =
        (size_t)1 << (log2i(builder->string.capacity + string.len));
    builder->string.capacity = new_capacity;
    char *tmp = (char *)BS_REALLOC(builder->allocator_ctx, builder->string.data,
                                   builder->string.capacity);
    if (tmp == NULL) {
      return BS_STATUS_ALLOCATOR_OUT_OF_MEMORY;
    }
    builder->string.data = tmp;
  }
  memcpy(builder->string.data + builder->string.len, string.view, string.len);
  builder->string.len += string.len;

  return BS_STATUS_OK;
}

BSDEF BS_BUILDER_STATUS bs_builder_append_cstr(BetterString_Builder *builder,
                                               const char * cstr) {
  return bs_builder_append_sv(
      builder, (BetterString_View){.view = cstr, .len = strlen(cstr)});
}

#endif
