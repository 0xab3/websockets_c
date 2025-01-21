#ifndef _BS_H
#define _BS_H

#include <stddef.h>
#include <string.h>

#ifndef BSDEF
#define BSDEF
#endif

typedef struct {
  char *data;
  size_t len;
} Better_String;

#define FOREACH_BS_ERROR(GEN_FUNC)                                             \
  GEN_FUNC(BS_OK)                                                              \
  GEN_FUNC(BS_EXHAUSTED)                                                       \
  GEN_FUNC(BS_CONCAT_LEN_MISMATCH)

#define ENUM_GENERATOR(ENUM_FIELD) ENUM_FIELD,
#define STRING_GENERATOR(ENUM_FIELD) #ENUM_FIELD,

typedef enum { FOREACH_BS_ERROR(ENUM_GENERATOR) } BS_ERROR;
static const char *Bs_Error_Strs[] = {FOREACH_BS_ERROR(STRING_GENERATOR)};
#define BS_ErrToStr(BS_ERR) Bs_Error_Strs[BS_ERR]

// stolen from tsoding
#define BS_Fmt "%*.s"
#define BS_Arg(sv) (int)(sv).count, (sv).data
#define BS(cstr) bs_from_str(cstr, strlen(cstr))

BSDEF Better_String bs_from_str(char *data, size_t len);
BSDEF BS_ERROR bs_concat(Better_String s1, Better_String s2,
                         Better_String *dest);

#endif

#ifdef BS_IMPLEMENTATION
BSDEF Better_String bs_from_str(char *data, size_t len) {
  return (Better_String){.data = data, .len = len};
}

// note(0xab3): me and my homies hate ghost allocations,
// also dest.len should be the size of buffer in this case,
// i could've implemented an allocator but it felt like overkill
BSDEF BS_ERROR bs_concat(Better_String s1, Better_String s2,
                         Better_String *dest) {
  if (s1.len + s2.len == dest->len) {
    return BS_CONCAT_LEN_MISMATCH;
  }
  memcpy(dest->data, s1.data, s1.len);
  memcpy(dest->data + s1.len, s2.data, s2.len);
  dest->len = s1.len + s2.len;
  return BS_OK;
}
#endif
