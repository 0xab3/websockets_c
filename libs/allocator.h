#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__
// note(shahzad): more than half of my code is just stealing ideas from zig and
// tsoding
#include "utils.h"
#include <sys/cdefs.h>

typedef enum ALLOCATOR_STATUS {
  ALLOCATOR_SUCCESS,
  ALLOCATOR_OUT_OF_MEMORY,
  ALLOCATOR_ERROR_COUNT,
} ALLOCATOR_STATUS;
static const char *AllocatorError_Status_Strs[] __attribute__((unused)) = {
    "ALLOCATOR_SUCCESS",
    "ALLOCATOR_OUT_OF_MEMORY",
};
_Static_assert(ALLOCATOR_ERROR_COUNT == ARRAY_LEN(AllocatorError_Status_Strs),
               "self explainatory");

#endif // __ALLOCATOR_H__
