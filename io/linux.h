#ifndef __LINUX_IO_H__
#define __LINUX_IO_H__
#include "io/io_uring.h"
#include <linux/io_uring.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
typedef struct io {
  struct ioUring uring;
} io;

typedef int(io_write_cb)(uint8_t *buffer, size_t buffer_len);
typedef int(io_read_cb)(uint8_t *buffer, size_t buffer_len);

typedef struct Completion {
  void *completion_cb;
  struct io_uring_cqe *completion;
} Completion;
struct io io_init(void);
int io_send_write_event(struct io *io_ctx, int fd, const uint8_t *buff,
                        size_t len);
int io_send_read_event(struct io *io_ctx, int fd, uint8_t *buff, size_t size);
ssize_t io_get_processed_event_result(struct io *io_ctx);
#endif // __LINUX_IO_H__
