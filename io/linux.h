#ifndef __LINUX_IO_H__
#define __LINUX_IO_H__
#include "io/io_uring.h"
#include "libs/dyn_array.h"
#include <linux/io_uring.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef int(io_completion_cb)(uint8_t *buffer, size_t buffer_len,
                              void *userdata);
typedef struct onCompletion {
  io_completion_cb *completion_cb;
  void *userdata;
} onCompletion;

typedef DA_TYPE_NEW(onCompletion) onCompletions;
typedef DA_TYPE_NEW(struct io_uring_cqe) ioUringCompletionArray;

typedef struct io {
  struct ioUring uring;
  atomic_uint_least64_t n_promises;
  ioUringCompletionArray completion_array;
  onCompletions on_completions;
} io;

// TODO(shahzad): move this shit to sm async specific thingy idk
typedef enum PROMISE_STATE {
  PROMISE_FAILED,
  PROMISE_RUNNING,
  PROMISE_COMPLETED
} PROMISE_STATE;

typedef struct Promise {
  uint64_t promise_id;
  PROMISE_STATE status;
  intptr_t result;
} Promise;

struct io io_init(void);
int io_send_write_event(struct io *io_ctx, int fd, const uint8_t *buff,
                        size_t len, io_completion_cb callback, void *userdata);
int io_send_read_event(struct io *io_ctx, int fd, const uint8_t *buff,
                       size_t len, io_completion_cb callback, void *userdata);
ssize_t io_get_processed_event_result(struct io *io_ctx);
struct io_uring_cqe *io_run_loop_until_completion(struct io *io_ctx,
                                                  onCompletion *completion);
Promise io_async_read(struct io *io_ctx, int fd, const uint8_t *buff,
                      size_t len);
Promise io_async_write(struct io *io_ctx, int fd, const uint8_t *buff,
                       size_t len);
PROMISE_STATE io_promise_poll(struct io *io_ctx, Promise *promise);
intptr_t io_promise_result(Promise *promise);
#endif // __LINUX_IO_H__
