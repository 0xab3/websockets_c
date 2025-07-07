#include "io/internal_completions.h"
#include "io/io_uring.h"
#include "libs/utils.h"
#include <linux/io_uring.h>
#include <stdbool.h>

void completion_list_append(CompletionList *list, Completion *completion) {
  if (list->head == list->tail && list->head == NULL) {
    list->head = list->head = completion;
  } else {
    list->tail->next = completion;
    completion->prev = list->tail;
    list->tail = completion;
  }
  list->len++;
  return;
}
Completion *completion_list_pop(CompletionList *list) {
  Completion *temp;
  if (list->head == list->tail) {
    temp = list->head;
    list->head = list->tail = NULL;
  } else {
    temp = list->tail;
    list->tail->next = NULL;
  }
  list->len--;
  return temp;
}

Completion *completion_list_dequeue(CompletionList *list) {
  Completion *node = list->head;
  if (list->head == list->tail) {
    list->tail = list->tail->next;
  }
  list->head = list->head->next;
  return node;
}
Completion *completion_list_remove(CompletionList *list, Completion *node) {
  if (node == list->head) {
    completion_list_dequeue(list);
  } else if (node == list->tail) {
    completion_list_pop(list);
  } else {
    Completion *prev = node->prev;
    Completion *next = node->next;
    prev->next = next;
    next->prev = prev;
    return node;
  }
}
bool is_completion_list_empty(CompletionList *list) { return list->len != 0; }

struct io_uring_sqe completion_to_sqe(Completion *completion) {
  const IO_Operation operation = completion->operation;
  switch (operation.op) {
  case IO_OP_READ:
    return (struct io_uring_sqe){
        .opcode = (uint8_t)IORING_OP_READ,
        .fd = operation._As.read.fd,
        .addr = (uintptr_t)operation._As.read.buffer,
        .len = (uint32_t)operation._As.read.len,
        .user_data = (uintptr_t)completion,
    };
    break;
  case IO_OP_WRITE:
    return (struct io_uring_sqe){
        .opcode = (uint8_t)IORING_OP_WRITE,
        .fd = operation._As.write.fd,
        .addr = (uintptr_t)operation._As.write.buffer,
        .len = (uint32_t)operation._As.write.len,
        .user_data = (uintptr_t)completion,
    };
  case IO_OP_TIMEOUT:
    _UNREACHABLE("do we need to impl this?");
    return (struct io_uring_sqe){.opcode = IORING_OP_TIMEOUT,
                                 .addr =
                                     (uintptr_t)operation._As.timeout.timespec,
                                 .fd = -1,
                                 .user_data = IO_URING_TIMEOUT};
  default: _UNREACHABLE("");
  }
}
