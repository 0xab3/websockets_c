#ifndef __IO_COMPLETIONS_H__
#define __IO_COMPLETIONS_H__

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct Completion Completion;
typedef int(io_completion_cb)(Completion *cqe, void *userdata);
typedef enum IO_OPERATION_TYPE {
  IO_OP_READ,
  IO_OP_WRITE,
  IO_OP_TIMEOUT,
} IO_OPERATION_TYPE;

typedef struct IO_Operation {
  IO_OPERATION_TYPE op;
  union {
    struct {
      uint8_t *buffer;
      size_t len;
      int fd;
      char padding[4];
    } read;
    struct {
      uint8_t *buffer;
      size_t len;
      int fd;
      char padding[4];
    } write;
    struct {
      struct __kernel_timespec *timespec;
    } timeout;
  } _As;
} IO_Operation;

struct Completion {
  int res;
  IO_Operation operation;
  Completion *next; // cries in cache affinity
  Completion *prev; // cries in cache affinity
  io_completion_cb *completion_cb;
  intptr_t userdata;
};
// there is a very good reason to use linked lists here i need pointer to non
// moving data in memory which and every element needs to be iterable
typedef struct CompletionList {
  Completion *head;
  Completion *tail;
  size_t len;
} CompletionList;

void completion_list_append(CompletionList *list, Completion *completion);
Completion *completion_list_pop(CompletionList *list);
Completion *completion_list_dequeue(CompletionList *list);
Completion *completion_list_remove(CompletionList *list, Completion *node);
bool is_completion_list_empty(CompletionList *list);
struct io_uring_sqe completion_to_sqe(Completion *completion);

#endif
