#include "linux.h"
#include "./io_uring.h"
#include "libs/dyn_array.h"
#include "libs/utils.h"
#include "unix_internal.c"
#include <assert.h>
#include <bits/types/sigset_t.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <time.h>

// NOTE(shahzad)!!!: no async io rn
struct io io_init(void) {
  return (struct io){.uring = io_uring_init(&(struct io_uring_params){0})};
}
int io_send_write_event(struct io *io_ctx, int fd, const uint8_t *buff,
                        size_t len, io_completion_cb callback, void *userdata) {
  const void *end_ptr = NULL;
  onCompletion completion = {.completion_cb = callback, .userdata = userdata};

  da_append(io_ctx->on_completions, completion, end_ptr);
  assert(end_ptr != NULL);
  struct io_uring_sqe sqe = {.user_data = (uint64_t)end_ptr,
                             .opcode = IORING_OP_WRITE,
                             .fd = fd,
                             .addr = (uint64_t)buff,
                             .len = (uint32_t)len};
  sq_submit(&io_ctx->uring.s_queue, sqe);
  return io_uring_enter(io_ctx->uring.ring_fd, 1, 0, IORING_ENTER_GETEVENTS,
                        &sqe);
}
int io_send_read_event(struct io *io_ctx, int fd, const uint8_t *buff,
                       size_t len, io_completion_cb callback, void *userdata) {
  onCompletion completion = {.completion_cb = callback, .userdata = userdata};

  void *end_ptr = NULL;
  da_append(io_ctx->on_completions, completion, end_ptr);
  assert(end_ptr != NULL);
  struct io_uring_sqe sqe = {.user_data = (uint64_t)end_ptr,
                             .opcode = IORING_OP_READ,
                             .fd = fd,
                             .addr = (uint64_t)buff,
                             .len = (uint32_t)len};
  sq_submit(&io_ctx->uring.s_queue, sqe);
  return io_uring_enter(io_ctx->uring.ring_fd, 1, 0, IORING_ENTER_GETEVENTS,
                        &sqe);
}

// useless function
ssize_t io_get_processed_event_result(struct io *io_ctx) {
  struct io_uring_cqe *cqe = cq_pop(&io_ctx->uring.c_queue);
  // this is bad
  while (cqe == NULL) {
    cqe = cq_pop(&io_ctx->uring.c_queue);
  }
  assert(cqe != NULL);
  ssize_t bytes_processed = cqe->res;
  if (bytes_processed < 0) {
    _LOG_DEBUG("read/write failed with %s", strerror((int)-bytes_processed));
  }
  return bytes_processed;
}

// gay
static void io_run_loop_once(struct io *io_ctx) {
  struct io_uring_cqe *cqe = cq_pop(&io_ctx->uring.c_queue);
  // this is bad
  while (cqe != NULL) {
    onCompletion *cmp_userdata = (onCompletion *)cqe->user_data;
    cmp_userdata->completion_cb(NULL, 0, cmp_userdata->userdata);
    cqe = cq_pop(&io_ctx->uring.c_queue);
  }
}
struct io_uring_cqe *io_run_loop_until_completion(struct io *io_ctx,
                                                  onCompletion *completion) {
  struct io_uring_cqe *cqe = cq_pop(&io_ctx->uring.c_queue);
  // this is bad
  while (true) {
    cqe = cq_pop(&io_ctx->uring.c_queue);
    if (cqe == NULL) {
      continue;
    }
    const onCompletion *cmp = (onCompletion *)cqe->user_data;
    if (cmp != completion) {
      cmp->completion_cb(NULL, 0, cmp->userdata);
    } else {
      return cqe;
    }
  }
  return NULL;
}

// await like async read
Promise io_async_read(struct io *io_ctx, int fd, const uint8_t *buff,
                      size_t len) {
  uint64_t promise_id =
      atomic_fetch_add_explicit(&io_ctx->n_promises, 1, __ATOMIC_ACQ_REL);
  struct io_uring_sqe sqe = {.user_data = promise_id,
                             .opcode = IORING_OP_READ,
                             .fd = fd,
                             .addr = (uint64_t)buff,
                             .len = (uint32_t)len};
  sq_submit(&io_ctx->uring.s_queue, sqe);
  const int result =
      io_uring_enter(io_ctx->uring.ring_fd, 1, 0, IORING_ENTER_GETEVENTS, &sqe);
  if (result <= 0) {
    return (Promise){
        .result = result, .status = PROMISE_FAILED, .promise_id = promise_id};
  } else {
    return (Promise){
        .result = result, .status = PROMISE_RUNNING, .promise_id = promise_id};
  }
  _UNREACHABLE();
}
// await like async read
Promise io_async_write(struct io *io_ctx, int fd, const uint8_t *buff,
                       size_t len) {
  uint64_t promise_id =
      atomic_fetch_add_explicit(&io_ctx->n_promises, 1, __ATOMIC_ACQ_REL);
  struct io_uring_sqe sqe = {.user_data = promise_id,
                             .opcode = IORING_OP_WRITE,
                             .fd = fd,
                             .addr = (uint64_t)buff,
                             .len = (uint32_t)len};
  sq_submit(&io_ctx->uring.s_queue, sqe);
  const int result =
      io_uring_enter(io_ctx->uring.ring_fd, 1, 0, IORING_ENTER_GETEVENTS, &sqe);
  if (result <= 0) {
    return (Promise){
        .result = result, .status = PROMISE_FAILED, .promise_id = promise_id};
  } else {
    return (Promise){
        .result = result, .status = PROMISE_RUNNING, .promise_id = promise_id};
  }
  _UNREACHABLE();
}

PROMISE_STATE io_promise_poll(struct io *io_ctx, Promise *promise) {
  assert(promise->promise_id < io_ctx->n_promises);
  struct io_uring_cqe *cqe_ptr = cq_pop_try(&io_ctx->uring.c_queue);
  if (cqe_ptr == NULL) {
    return PROMISE_RUNNING;
  }

  // TODO(shahzad): copy all the cqe's directly
  struct io_uring_cqe cqe = cqe_clone(cqe_ptr);
  struct io_uring_cqe *cqe_leaked = NULL;

  // TODO(shahzad): this is not good add counters and sanity checks before this
  while (cqe.user_data != promise->promise_id) {
    da_append(io_ctx->completion_array, cqe, cqe_leaked);
    assert(cqe_leaked != NULL);
    cqe_ptr = cq_pop_try(&io_ctx->uring.c_queue);
    if (cqe_ptr == NULL) {
      return PROMISE_RUNNING;
    }
    cqe = cqe_clone(cqe_ptr);
  }

  assert(cqe.user_data == promise->promise_id);

  da_append(io_ctx->completion_array, cqe, cqe_leaked);
  assert(cqe_leaked != NULL);
  assert(cqe_leaked->user_data == promise->promise_id);

  promise->status = PROMISE_COMPLETED;
  promise->result = (intptr_t)cqe_leaked;
  return promise->status;
}

intptr_t io_promise_result(Promise *promise) { return promise->result; }
