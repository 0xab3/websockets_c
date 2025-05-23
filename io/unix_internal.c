#include <fcntl.h>

static inline int unix__enable_nonblocking_io(int fd) {
  const int current_flags = fcntl(fd, F_GETFL);
  if (current_flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFL, current_flags | O_NONBLOCK);
}
