#ifndef __LINUX_IO_H__
#define __LINUX_IO_H__
#include "io/internal_completions.h"
#include "io/io_uring.h"
#include <linux/io_uring.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef struct Io {
  struct IoUring uring;
  CompletionList completed;
} Io;

typedef enum IO_SUBMIT_STATUS {
  IO_SUCCESS,
  IO_QUEUE_NEEDS_FLUSH
} IO_SUBMIT_STATUS;
typedef enum IO_POLL_STATUS {
  IO_POLL_SUCCESS,
  IO_POLL_QUEUE_REQUIRE_FLUSH, // same as IO_QUEUE_NEEDS_FLUSH
  IO_POLL_COMPLETION_RUNNING,
} IO_POLL_STATUS;

struct Io io_init(void);
bool io_ring_needs_enter(IoUring *uring);
IO_SUBMIT_STATUS io_prepare_read_with_cb(struct Io *io_ctx, int fd,
                                         uint8_t *buff, size_t len,
                                         Completion *completion,
                                         io_completion_cb callback,
                                         void *userdata);
IO_SUBMIT_STATUS io_prepare_write_with_cb(struct Io *io_ctx, int fd,
                                          uint8_t *buff, size_t len,
                                          Completion *completion,
                                          io_completion_cb callback,
                                          void *userdata);
bool io_queue_require_flush(struct Io *io_ctx);
bool io_is_queue_empty(struct Io *io_ctx);
int io_queue_flush(struct Io *io_ctx);
ssize_t io_get_processed_event_result(struct Io *io_ctx);
struct io_uring_cqe *io_run_loop_until_completion(struct Io *io_ctx,
                                                  Completion *completion);
IO_SUBMIT_STATUS io_prepare_read(struct Io *io_ctx, int fd, uint8_t *buff,
                                 size_t len, Completion *completion);
IO_SUBMIT_STATUS io_prepare_write(struct Io *io_ctx, int fd, uint8_t *buff,
                                  size_t len, Completion *completion);
IO_POLL_STATUS io_poll_completion(struct Io *io_ctx, Completion *completion);
size_t io_completion_list_flush(struct Io *io_ctx);

void io_run_loop_once(struct Io *io_ctx, struct __kernel_timespec timeout);
#endif // __LINUX_IO_H__
