#include "linux.h"
#include "./io_uring.h"
#include "libs/utils.h"
#include "unix_internal.c"
#include <assert.h>
#include <bits/types/sigset_t.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/poll.h>

// NOTE(shahzad)!!!: no async io rn
struct io io_init(void) {
  return (struct io){.uring = io_uring_init(&(struct io_uring_params){0})};
}
int io_send_write_event(struct io *io_ctx, int fd, const uint8_t *buff,
                        size_t len) {
  _LOG_TODO("add userdata to identify events!!!!\n");
  struct io_uring_sqe sqe = {.opcode = IORING_OP_WRITE,
                             .fd = fd,
                             .addr = (uint64_t)buff,
                             .len = (uint32_t)len};
  sq_submit(&io_ctx->uring.s_queue, sqe);
  // this is blocking :skull:
  return io_uring_enter(io_ctx->uring.ring_fd, 1, 1, IORING_ENTER_GETEVENTS,
                        &sqe);
}
int io_send_read_event(struct io *io_ctx, int fd, uint8_t *buff, size_t size) {
  struct io_uring_sqe sqe = {.opcode = IORING_OP_READ,
                             .fd = fd,
                             .addr = (uint64_t)buff,
                             .len = (uint32_t)size};
  sq_submit(&io_ctx->uring.s_queue, sqe);
  return io_uring_enter(io_ctx->uring.ring_fd, 1, 1, IORING_ENTER_GETEVENTS,
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
    _LOG_DEBUG("read/write failed with %s\n", strerror((int)-bytes_processed));
  }
  return bytes_processed;
}
