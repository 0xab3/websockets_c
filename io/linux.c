#include "linux.h"
#include "libs/utils.h"
#include "unix_internal.c"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

struct io io_init(int fd) { return (struct io){.fd = fd}; }
int io_set_nonblocking(struct io *io_ctx) {
  return unix__enable_nonblocking_io(io_ctx->fd);
}
// note(shahzad): remove poll use io_uring
ssize_t io_write_exact(struct io *io_ctx, const uint8_t *buff, size_t len) {
  struct pollfd poll_fd[1] = {0};
  poll_fd[0].fd = io_ctx->fd;
  poll_fd[0].events = POLLOUT;

  size_t total = 0;
  while (total < len) {
    int ready = poll(poll_fd, 1, -1);
    if (ready == -1) {
      return -1;
    }

    assert(poll_fd[0].revents & POLLOUT);
    ssize_t wrote = write(io_ctx->fd, buff + total, len - total);
    assert(wrote != -1);
    total += (size_t)wrote;
  }

  return (ssize_t)total;
}

ssize_t io_read_with_timeout(struct io *io_ctx, uint8_t *buff, size_t size,
                             int timeout) {
  struct pollfd poll_fd[1] = {0};
  poll_fd[0].fd = io_ctx->fd;
  poll_fd[0].events = POLLIN;
  ssize_t total = 0;
  while ((size_t)total < size) {
    int ready = poll(
        poll_fd, 1,
        timeout); // timeout will be a lot than 1 ik that (this is throw away)
    if (ready == -1) {
      return total == 0 ? -1 : total;
    }
    if (!(poll_fd[0].revents & POLLIN)) {
      return total;
    }
    ssize_t read_bytes = read(io_ctx->fd, buff + total, size - (size_t)total);
    if (read_bytes == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        _UNREACHABLE("we already polled this can't return EAGAIN");
        continue;
      }
      return -1;
    }

    total += (size_t)read_bytes;
  }

  return total;
}
