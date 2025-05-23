#ifndef __LINUX_IO_H__
#define __LINUX_IO_H__
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
typedef struct io {
  int fd;
} io;
struct io io_init(int fd);
int io_set_nonblocking(struct io *io_ctx);
ssize_t io_write_exact(struct io *io_ctx, const uint8_t *buff, size_t len);
ssize_t io_read_with_timeout(struct io *io_ctx, uint8_t *buff, size_t size,
                             int timeout);
#endif // __LINUX_IO_H__
