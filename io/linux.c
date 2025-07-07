#include "linux.h"
#include "./io_uring.h"
#include "io/internal_completions.h"
#include "libs/dyn_array.h"
#include "libs/utils.h"
#include "unix_internal.c"
#include <assert.h>
#include <bits/types/sigset_t.h>
#include <complex.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <time.h>

struct Io io_init(void) {
  // const int flags = IORING_SETUP_SQPOLL;
  const int flags = 0;
  return (struct Io){.uring = io_uring_init(&(struct io_uring_params){
                         .flags = flags, .sq_thread_idle = /* 2000 */ 0})};
}

IO_SUBMIT_STATUS io_prepare_read(struct Io *io_ctx, int fd, uint8_t *buff,
                                 size_t len, Completion *completion) {
  IoUring_SubmissionQueue *s_queue = &io_ctx->uring.s_queue;
  memset(completion, 0, sizeof(*completion));

  {
    completion->operation.op = IO_OP_READ;
    completion->operation._As.read.fd = fd;
    completion->operation._As.read.buffer = buff;
    completion->operation._As.read.len = len;
  }

  assert(len == (uint32_t)len);
  const IO_URING_STATUS put_status =
      sq_put_read(s_queue, fd, buff, (uint32_t)len, (uintptr_t)completion);
  if (put_status == IO_URING_SQ_FULL) {
    return IO_QUEUE_NEEDS_FLUSH;
  }

  assert(put_status == IO_URING_SUCCESS);
  return IO_SUCCESS;
}

IO_SUBMIT_STATUS io_prepare_write(struct Io *io_ctx, int fd, uint8_t *buff,
                                  size_t len, Completion *completion) {
  IoUring_SubmissionQueue *s_queue = &io_ctx->uring.s_queue;
  memset(completion, 0, sizeof(*completion));

  {
    completion->operation.op = IO_OP_WRITE;
    completion->operation._As.write.fd = fd;
    completion->operation._As.write.buffer = buff;
    completion->operation._As.write.len = len;
  }

  assert(len == (uint32_t)len);
  const IO_URING_STATUS put_status =
      sq_put_write(s_queue, fd, buff, (uint32_t)len, (uintptr_t)completion);
  if (put_status == IO_URING_SQ_FULL) {
    return IO_QUEUE_NEEDS_FLUSH;
  }
  assert(put_status == IO_URING_SUCCESS);

  return IO_SUCCESS;
}

IO_POLL_STATUS io_poll_completion(struct Io *io_ctx, Completion *completion) {
  IoUring_SubmissionQueue *s_queue = &io_ctx->uring.s_queue;
  Completion *curr = io_ctx->completed.head;
  io_run_loop_once(io_ctx, (struct __kernel_timespec){.tv_nsec = 1});

  while (curr != NULL) {
    if (curr == completion) {
      completion_list_remove(&io_ctx->completed, curr);
      return IO_POLL_SUCCESS;
    }
    curr = curr->next;
  }

  for (size_t i = 0; i < sq_ready(s_queue); i++) {
    const struct io_uring_sqe *sqe = sq_get_unsafe(s_queue, i);
    if (sqe->user_data == (uintptr_t)completion) {
      return IO_POLL_QUEUE_REQUIRE_FLUSH;
    }
  }
  return IO_POLL_COMPLETION_RUNNING;
}

int io_queue_flush(struct Io *io_ctx) {
  IoUring *uring = &io_ctx->uring;
  int flushed = io_uring_submit(uring);
  _LOG_DEBUG("enter completed! ret: %d, status: %s", flushed, strerror(errno));
  assert(flushed >= 0 && "we don't handle io_uring_enter error!");
  return flushed;
}

//===============================================================================//
IO_SUBMIT_STATUS io_prepare_read_with_cb(struct Io *io_ctx, int fd,
                                         uint8_t *buff, size_t len,
                                         Completion *completion,
                                         io_completion_cb callback,
                                         void *userdata) {
  completion->completion_cb = callback;
  completion->userdata = (intptr_t)userdata;

  {
    completion->operation.op = IO_OP_READ;
    completion->operation._As.read.fd = fd;
    completion->operation._As.read.buffer = buff;
    completion->operation._As.read.len = len;
  }

  assert(len == (uint32_t)len);
  IO_URING_STATUS put_status = sq_put_read(
      &io_ctx->uring.s_queue, fd, buff, (uint32_t)len, (uintptr_t)completion);
  if (put_status == IO_URING_SQ_FULL) {
    return IO_QUEUE_NEEDS_FLUSH;
  }
  assert(put_status == IO_URING_SUCCESS);
  return IO_SUCCESS;
}

IO_SUBMIT_STATUS io_prepare_write_with_cb(struct Io *io_ctx, int fd,
                                          uint8_t *buff, size_t len,
                                          Completion *completion,
                                          io_completion_cb callback,
                                          void *userdata) {
  completion->completion_cb = callback;
  completion->userdata = (intptr_t)userdata;

  {
    completion->operation.op = IO_OP_WRITE;
    completion->operation._As.write.fd = fd;
    completion->operation._As.write.buffer = buff;
    completion->operation._As.write.len = len;
  }

  assert(len == (uint32_t)len);
  IO_URING_STATUS put_status = sq_put_write(
      &io_ctx->uring.s_queue, fd, buff, (uint32_t)len, (uintptr_t)completion);
  if (put_status == IO_URING_SQ_FULL) {
    return IO_QUEUE_NEEDS_FLUSH;
  }
  return IO_SUCCESS;
}

// gay
void io_run_loop_once(struct Io *io_ctx, struct __kernel_timespec timeout) {
  uint64_t copied_cqes[128] = {0}; // whoever thought that making
                                   // cqe flexible was a good idea has
                                   // intellect beyond my comprehension
  size_t n_completions = 0;

  int ret = io_cq_wait_timeout(&io_ctx->uring, &n_completions, timeout);
  // _LOG_DEBUG("io_cq_peek_timeout returned %d status: %s", ret,
  // strerror(errno)); _LOG_DEBUG("n_completions %lu", n_completions);
  // printf("\n");
  assert(ret == 0 && "we don't handle io_uring_enter fail!");

  while (n_completions >= 1) {
    const size_t n_cqes_copied = cq_copy((struct io_uring_cqe *)&copied_cqes,
                                         &io_ctx->uring.c_queue, 64);
    n_completions -= n_cqes_copied;
    for (size_t i = 0; i < n_cqes_copied; i++) {
      struct io_uring_cqe *as_cqe = &((struct io_uring_cqe *)copied_cqes)[i];

      assert(as_cqe != NULL);
      if (as_cqe->user_data == IO_URING_TIMEOUT) {
        // _LOG_DEBUG("%lu. cqe {timeout}", i);
        continue;
      } else {
        Completion *completion = (Completion *)as_cqe->user_data;
        // _LOG_DEBUG("%lu. completion: %p", i, (void *)completion);

        if (completion == NULL) {
          _LOG_DEBUG("%lu. completion is NULL!", i);
          _LOG_DEBUG("as_cqe: %d %u", as_cqe->res, as_cqe->flags);
        }
        assert(completion != NULL);
        completion->res = as_cqe->res;
        completion_list_append(&io_ctx->completed, completion);
      }
    }
  }
}

bool io_is_queue_empty(struct Io *io_ctx) {
  struct IoUring_SubmissionQueue *s_queue = &io_ctx->uring.s_queue;
  return sq_ready(s_queue) == 0;
}

bool io_queue_require_flush(struct Io *io_ctx) {
  struct IoUring_SubmissionQueue *s_queue = &io_ctx->uring.s_queue;
  return sq_available(s_queue) == 0;
}

// internal bool _io_is_promise_submitted(struct Io *io_ctx, Io_Promise
// *promise) {
//   assert(promise->promise_id < io_ctx->n_promises);
//   IoUring_SubmissionQueue *s_queue = &io_ctx->uring.s_queue;
//   uint32_t n_entries = sq_ready(s_queue);
//   for (size_t i = 0; i < n_entries; i++) {
//     const struct io_uring_sqe *sqe = sq_get_unsafe(s_queue, i);
//     if (sqe->user_data == IO_URING_TIMEOUT) {
//       continue;
//     }
//     if (sqe->user_data == promise->promise_id) {
//       return false;
//     }
//   }
//   return true;
// }
//
// IO_PROMISE_STATE io_promise_poll(struct Io *io_ctx, Io_Promise *promise) {
//   assert(promise->promise_id < io_ctx->n_promises);
//
//   struct io_uring_cqe *cqes = NULL;
//   size_t n_completions = 0;
//
//   if (!_io_is_promise_submitted(io_ctx, promise)) {
//     return PROMISE_NOT_SUBMITTED;
//   }
//
//   cq_peek(&io_ctx->uring.c_queue, &cqes, &n_completions);
//   _LOG_DEBUG("cq_peek returned n_completions: %lu", n_completions);
//
//   // just modify the state of completion and don't give a shit abt the cqes
//   _io_cq_drain(io_ctx, cqes, n_completions);
//   size_t idx = 0;
//   while (idx < io_ctx->completion_array.len) {
//     struct io_uring_cqe cqe_cpy =
//         _completion_array_get(&io_ctx->completion_array, idx);
//     Completion *on_completion = (Completion *)cqe_cpy.user_data;
//     if (cqe_cpy.user_data == IO_URING_TIMEOUT) {
//       da_remove_swap(io_ctx->completion_array, idx);
//     } else if (cqe_cpy.user_data == promise->promise_id) {
//       promise->status = PROMISE_COMPLETED;
//       promise->result.res = io_ctx->completion_array.items[idx].res;
//       da_remove_swap(io_ctx->completion_array, idx);
//       return promise->status;
//     } else {
//       idx++;
//     }
//   }
//   return PROMISE_RUNNING;
// }

// Io_Result io_promise_result(Io_Promise *promise) { return promise->result; }
