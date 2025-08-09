#ifndef __BUFFERED_READER_H__
#define __BUFFERED_READER_H__

#include "bs.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFIO_DEFAULT_BUFFER_SIZE 4 * 1024
// internal

#define BUFIO_ERROR_BUFFER_FULL -2LL
#define BUFIO_ERROR_CONNECTION_CLOSED -3LL
#define IS_BUFFER_EMPTY(buffer) ((buffer).len <= 0)
struct BufferReader {
  intptr_t fd; // only for alignment
  struct BetterString buffer;
};
// wtf is a buffered writer idk
struct BufferWriter {
  intptr_t fd;
  struct BetterString buffer;
};

void BufReader_init(struct BufferReader *reader, int fd, char *buffer,
                    size_t buffer_len);
ssize_t BufReader_read(struct BufferReader *reader, char *buffer, size_t len);
size_t Buf_copy_raw(struct BetterString *src, char *dest, size_t len,
                    size_t offset);
ssize_t BufReader_read_until(struct BufferReader *reader, char *buffer,
                             size_t len, const char *until);

#ifdef BIO_IMPLEMENTATION

internal void Buf__advance(struct BetterString *buffer, size_t by) {
  assert(buffer->len >= by);
  memmove(buffer->data, buffer->data + by, buffer->len - by);
  buffer->len -= by;
}

void BufReader_init(struct BufferReader *reader, int fd, char *buffer,
                    size_t buffer_len) {
  const int flags = fcntl(fd, F_GETFD);

  if (!(flags & O_NONBLOCK)) fcntl(fd, F_SETFD, flags | O_NONBLOCK);

  {
    reader->fd = fd;
    reader->buffer.data = buffer;
    reader->buffer.capacity = buffer_len;
    reader->buffer.len = 0;
  }
}
size_t Buf_copy_raw(struct BetterString *src, char *dest, size_t len,
                    size_t offset) {
  size_t bytes_to_copy = MIN(len, src->len);
  assert(offset <= src->len && offset <= len);
  memcpy(dest, src->data + offset, bytes_to_copy - offset);
  return bytes_to_copy;
}
ssize_t BufReader_read(struct BufferReader *reader, char *buffer, size_t len) {
  if (reader->buffer.len < len || len == 0 || buffer == NULL) {
    const size_t buffer_remaining_len =
        reader->buffer.capacity - reader->buffer.len;

    if (buffer_remaining_len == 0) return BUFIO_ERROR_BUFFER_FULL;

    assert(buffer_remaining_len > 0 &&
           buffer_remaining_len <= reader->buffer.capacity);

    const ssize_t bytes_read =
        read((int)reader->fd, reader->buffer.data + reader->buffer.len,
             buffer_remaining_len);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return bytes_read;
      } else _UNREACHABLE("unimplemented");
    } else if (bytes_read == 0) return BUFIO_ERROR_CONNECTION_CLOSED;

    reader->buffer.len += (size_t)bytes_read;
    if (buffer == NULL || len == 0) return bytes_read;
  }

  if (reader->buffer.len == 0) return -1;

  const size_t bytes_copied = Buf_copy_raw(&reader->buffer, buffer, len, 0);
  Buf__advance(&reader->buffer, bytes_copied);
  return (ssize_t)bytes_copied;
}

ssize_t BufReader_read_until(struct BufferReader *reader, char *buffer,
                             size_t len, const char *until) {
  ssize_t idx = bs_find(*(BetterString_View *)&reader->buffer, BSV(until));
  if (idx < 0) {
    const ssize_t bytes_read = BufReader_read(reader, NULL, 0);
    if (IS_BUFFER_EMPTY(reader->buffer)) {
      return bytes_read;
    }
  }
  idx = bs_find(*(BetterString_View *)&reader->buffer, BSV(until));
  if (idx < 0) return -1;

  assert((size_t)idx < len && "buffer to small");

  Buf_copy_raw(&reader->buffer, buffer, (size_t)idx + strlen(until), 0);
  Buf__advance(&reader->buffer, (size_t)idx + strlen(until));
  return idx + (ssize_t)strlen(until);
}

#endif // BR_IMPLEMENTATION

#endif // __BUFFERED_READER_H__
