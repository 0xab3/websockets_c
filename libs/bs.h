#ifndef __BS_H__
#define __BS_H__

#include "allocator.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <strings.h>
#include <sys/cdefs.h>
#include <unistd.h>

#ifndef BSDEF
#define BSDEF
#endif // BSDEF

#define BS_ESCAPE_BUFFER_MAX_SIZE 256
#ifndef opaque_ptr_t
#define opaque_ptr_t void *
#endif // opaque_ptr_t

typedef struct {
  const char *view;
  size_t len;
} BetterString_View;

// note(shahzad): BetterString_ViewManaged defines that the underlying string is
// allocated (the caller should save a ref to the allocator_ctx and the
// underlying pointer should not be modified)
typedef struct {
  char *view;
  size_t len;
} BetterString_ViewManaged;

#define BS_ALLOC(allocator_ctx, ...) malloc(__VA_ARGS__);
#define BS_REALLOC(allocator_ctx, ...) realloc(__VA_ARGS__);
#define BS_FREE(allocator_ctx, ...) free(__VA_ARGS__);

typedef struct {
  opaque_ptr_t allocator_ctx;
  struct {
    char *data;
    size_t len;
    size_t capacity;
  } string;
} BetterString_Builder;

// stolen from tsoding
#define BSV_Fmt "%.*s"
#define BSV_Arg(sv) (int)(sv).len, (sv).view
#define BSV(cstr) bs_from_string(cstr, strlen(cstr))

// note(shahzad):
//  every function that works on view is prefixed by bs
//  every function that works on builder is prefixed by bs_builder

BSDEF BetterString_View bs_from_string(const char *data, size_t len);
BSDEF ALLOCATOR_STATUS bs_clone(opaque_ptr_t allocator_ctx,
                                BetterString_View str,
                                BetterString_ViewManaged *result)
    __nonnull((3));
BSDEF BetterString_View bs_trim(BetterString_View str);
BSDEF BetterString_View bs_trim_right(BetterString_View str);
BSDEF BetterString_View bs_trim_left(BetterString_View str);
// note(shahzad): we only support split once and not split, sue me.
BSDEF BetterString_View bs_split_once_by_bs(BetterString_View *str,
                                            BetterString_View bs_delim);
BSDEF BetterString_View bs_split_once_by_delim(BetterString_View *str,
                                               char delim);
BSDEF ssize_t bs_find_cstr(BetterString_View haysack, const char *needle);
BSDEF ssize_t bs_find(BetterString_View haysack, BetterString_View needle);
BSDEF ssize_t bs_char_at(BetterString_View haysack, char needle);
BSDEF bool bs_eq(BetterString_View s1, BetterString_View s2);
BSDEF bool bs_eq_ignore_case(BetterString_View s1, BetterString_View s2);

BSDEF BetterString_Builder bs_builder_new(opaque_ptr_t allocator_ctx);
BSDEF void bs_builder_destory(BetterString_Builder *builder);
static __always_inline void bs_builder_reset(BetterString_Builder *builder);
BSDEF BetterString_View bs_builder_to_sv(BetterString_Builder *builder);
BSDEF BetterString_ViewManaged
bs_builder_to_managed_sv(BetterString_Builder *builder);
BSDEF ALLOCATOR_STATUS bs_builder_reserve(BetterString_Builder *builder,
                                          size_t capacity);
BSDEF ALLOCATOR_STATUS bs_builder_reserve_exact(BetterString_Builder *builder,
                                                size_t capacity);
BSDEF ALLOCATOR_STATUS bs_builder_append_sv(BetterString_Builder *builder,
                                            BetterString_View string);
BSDEF ALLOCATOR_STATUS bs_builder_append_cstr(BetterString_Builder *builder,
                                              const char *cstr);
#endif // __BS_H__

#ifdef BS_IMPLEMENTATION

#define bs_builder_sprintf(error, builder, fmt, ...)                           \
  do {                                                                         \
    int formatted_len = snprintf(NULL, 0, fmt, __VA_ARGS__);                   \
    if ((*builder).string.capacity <                                           \
        (*builder).string.len + (size_t)formatted_len) {                       \
      error = bs_builder_reserve(builder, (*builder).string.len +              \
                                              (size_t)formatted_len);          \
      if (error != ALLOCATOR_SUCCESS) {                                        \
        assert(error == ALLOCATOR_OUT_OF_MEMORY);                              \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    sprintf((*builder).string.data + (*builder).string.len, fmt, __VA_ARGS__); \
    (*builder).string.len += (size_t)formatted_len;                            \
  } while (0)

BSDEF BetterString_View bs_from_string(const char *data, size_t len) {
  return (BetterString_View){.view = data, .len = len};
}
BSDEF ALLOCATOR_STATUS bs_clone(opaque_ptr_t allocator_ctx,
                                BetterString_View str,
                                BetterString_ViewManaged *result) {
  BetterString_Builder bs_builder = bs_builder_new(allocator_ctx);
  ALLOCATOR_STATUS status = bs_builder_reserve_exact(&bs_builder, str.len);
  if (status != ALLOCATOR_SUCCESS) {
    assert(status == ALLOCATOR_OUT_OF_MEMORY);
    return status;
  }

  // copy the data, fuck dry!
  memcpy(bs_builder.string.data, str.view, str.len);
  bs_builder.string.len = str.len;

  *result = bs_builder_to_managed_sv(&bs_builder);
  return ALLOCATOR_SUCCESS;
}

BSDEF BetterString_View bs_trim(BetterString_View str) {
  assert(str.view != NULL);
  BetterString_View trimmed = str;
  trimmed = bs_trim_right(trimmed);
  trimmed = bs_trim_left(trimmed);
  return trimmed;
}
BSDEF BetterString_View bs_trim_right(BetterString_View str) {
  assert(str.view != NULL);
  BetterString_View trimmed = str;
  size_t idx = str.len;
  while (isspace(trimmed.view[idx]) && idx > 0)
    idx--;
  trimmed.len -= idx;
  return trimmed;
}

BSDEF BetterString_View bs_trim_left(BetterString_View str) {
  assert(str.view != NULL);
  BetterString_View trimmed = str;
  size_t idx = 0;
  while (isspace(trimmed.view[idx]) && idx < trimmed.len)
    idx++;
  trimmed.view += idx;
  trimmed.len -= idx;
  return trimmed;
}

BSDEF ssize_t bs_find_cstr(BetterString_View haysack, const char *needle) {
  return bs_find(haysack,
                 (BetterString_View){.view = needle, .len = strlen(needle)});
}

BSDEF bool bs_eq(BetterString_View s1, BetterString_View s2) {
  if (s1.len != s2.len) {
    return false;
  }
  return memcmp(s1.view, s2.view, MAX(s1.len, s2.len)) == 0;
}
BSDEF bool bs_eq_ignore_case(BetterString_View s1, BetterString_View s2) {
  if (s1.len != s2.len) {
    return false;
  }
  // i have a dream to not use any stdlib functions except syscalls
  return strncasecmp(s1.view, s2.view, s1.len) == 0;
}
BSDEF ssize_t bs_find(BetterString_View haysack, BetterString_View needle) {
  BetterString_View window = bs_from_string(haysack.view, needle.len);
  size_t i = 0;
  while (haysack.len - i >= window.len) {
    _LOG_DEBUG("window: \"" BSV_Fmt "\"\n", BSV_Arg(window));
    _LOG_DEBUG("needle: \"" BSV_Fmt "\"\n", BSV_Arg(needle));
    if (bs_eq(window, needle)) {
      return (ssize_t)i;
    }
    window.view++;
    i++;
  }
  return -1;
}
BSDEF ssize_t bs_char_at(BetterString_View haysack, char needle) {
  for (size_t i = 0; i < haysack.len; i++) {
    if (haysack.view[i] == needle) {
      return (ssize_t)i;
    }
  }
  return (ssize_t)-1;
}

BSDEF BetterString_View bs_split_once_by_bs(BetterString_View *str,
                                            BetterString_View bs_delim) {

  BetterString_View chopped = {0};
  const ssize_t idx = bs_find(*str, bs_delim);
  if (idx == -1) {
    return chopped;
  }
  assert((size_t)idx + bs_delim.len < str->len);

  chopped.view = str->view;
  chopped.len = (size_t)idx;

  str->len -= (size_t)idx + bs_delim.len;
  str->view += (size_t)idx + bs_delim.len;

  return chopped;
}
BSDEF BetterString_View bs_split_once_by_delim(BetterString_View *str,
                                               char delim) {
  BetterString_View chopped = {0};
  const ssize_t char_idx = bs_char_at(*str, delim);
  if (char_idx == -1) {
    return chopped;
  }
  assert((size_t)char_idx <= str->len);

  chopped.view = str->view;
  chopped.len = (size_t)char_idx;

  // it doesn't really matter if this holds invalid pointer as the len is 0
  str->view += char_idx + 1;
  str->len -= (size_t)char_idx + 1;
  return chopped;
}
// writer MUST write all the data that is provided to it
static inline BSDEF ssize_t bs_escape(BetterString_View str,
                                      opaque_ptr_t userdata,
                                      ssize_t(writer)(BetterString_View str,
                                                      opaque_ptr_t userdata)) {

  static char escaped[BS_ESCAPE_BUFFER_MAX_SIZE] = {0};
  size_t escaped_idx = 0;
  for (size_t i = 0; i < str.len; i++) {
    char escaped_char[2] = {0};
    escaped_char[0] = '\\';
    switch (str.view[i]) {
    case '\0': escaped_char[1] = '0'; break;
    case '\a': escaped_char[1] = 'a'; break;
    case '\b': escaped_char[1] = 'b'; break;
    case '\f': escaped_char[1] = 'f'; break;
    case '\n': escaped_char[1] = 'n'; break;
    case '\r': escaped_char[1] = 'r'; break;
    case '\t': escaped_char[1] = 't'; break;
    case '\v': escaped_char[1] = 'v'; break;
    case '\\': escaped_char[1] = '\\'; break;
    case '\?': escaped_char[1] = '\?'; break;
    case '\'': escaped_char[1] = '\''; break;
    case '\"': escaped_char[1] = '\"'; break;
    default:
      break;
      // burh this shit sucks ass
    }
    if (escaped_idx + 1 > 256) {
      ssize_t bytes_written =
          writer(bs_from_string(escaped, escaped_idx), userdata);
      if (bytes_written != (ssize_t)escaped_idx) {
        _LOG_ERROR("writter failed at index: ")
        return bytes_written;
      }
    }
    escaped_idx = 0;
    escaped[escaped_idx] = escaped_char[0];
    escaped[escaped_idx + 1] = escaped_char[1];
  }

  if (escaped_idx > 0) {
    ssize_t bytes_written =
        writer(bs_from_string(escaped, escaped_idx), userdata);
    if (bytes_written != (ssize_t)escaped_idx) {
      return bytes_written;
    }
  }
}

BSDEF BetterString_Builder bs_builder_new(opaque_ptr_t allocator_ctx) {
  return (BetterString_Builder){
      .string = {.data = NULL, .len = 0},
      .allocator_ctx = allocator_ctx,
  };
}

BSDEF void bs_builder_destory(BetterString_Builder *builder) {
  assert(builder->string.data != NULL &&
         "destroying shit that doesn't even exist wth");
  BS_FREE(builder->allocator_ctx, builder->string.data);
  builder->string.len = 0;
  builder->string.capacity = 0;
  return;
}

static __always_inline void bs_builder_reset(BetterString_Builder *builder) {
  builder->string.len = 0;
}

BSDEF BetterString_View bs_builder_to_sv(BetterString_Builder *builder) {
  return (BetterString_View){.view = builder->string.data,
                             .len = builder->string.len};
}

BSDEF BetterString_ViewManaged
bs_builder_to_managed_sv(BetterString_Builder *builder) {
  return (BetterString_ViewManaged){.view = builder->string.data,
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
BSDEF ALLOCATOR_STATUS bs_builder_reserve(BetterString_Builder *builder,
                                          size_t capacity) {
  return bs_builder_reserve_exact(builder, (size_t)1 << log2i(capacity));
}

BSDEF ALLOCATOR_STATUS bs_builder_reserve_exact(BetterString_Builder *builder,
                                                size_t capacity) {
  builder->string.capacity = capacity;
  char *tmp = (char *)BS_REALLOC(builder->allocator_ctx, builder->string.data,
                                 builder->string.capacity);
  if (tmp == NULL) {
    return ALLOCATOR_OUT_OF_MEMORY;
  }
  builder->string.data = tmp;
  return ALLOCATOR_SUCCESS;
}

BSDEF ALLOCATOR_STATUS bs_builder_append_sv(BetterString_Builder *builder,
                                            const BetterString_View string) {
  const size_t available = builder->string.capacity - builder->string.len;
  if (available < string.len) {
    ALLOCATOR_STATUS is_reserved =
        bs_builder_reserve(builder, builder->string.capacity + string.len);
    if (is_reserved != ALLOCATOR_SUCCESS) {
      assert(is_reserved == ALLOCATOR_OUT_OF_MEMORY);
      return ALLOCATOR_OUT_OF_MEMORY;
    }
  }
  memcpy(builder->string.data + builder->string.len, string.view, string.len);
  builder->string.len += string.len;

  return ALLOCATOR_SUCCESS;
}

BSDEF ALLOCATOR_STATUS bs_builder_append_cstr(BetterString_Builder *builder,
                                              const char *cstr) {
  return bs_builder_append_sv(
      builder, (BetterString_View){.view = cstr, .len = strlen(cstr)});
}

#endif // BS_IMPLEMENTATION
