#define _GNU_SOURCE
#include "io_uring.h"
#include "libs/utils.h"
#include <assert.h>
#include <errno.h>
#include <linux/io_uring.h>
#include <linux/time_types.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define QUEUE_DEPTH 1

IoUring io_uring_init(struct io_uring_params *params) {
  assert(!(params->flags & IORING_SETUP_CQE32) && "no");
  struct IoUring ring = {0};
  ring.ring_fd = io_uring_setup(QUEUE_DEPTH, params);
  ring.flags = params->flags;
  // stolen from the man page
  size_t sring_sz =
      params->sq_off.array + params->sq_entries * sizeof(uint32_t);
  size_t cring_sz =
      params->cq_off.cqes + params->cq_entries * sizeof(struct io_uring_cqe);
  // we don't give a single shit about kernel version < 5.14
  // todo(shahzad): add support for kernel version < 5.14
  assert(params->features & IORING_FEAT_SINGLE_MMAP);

  if (cring_sz > sring_sz)
    sring_sz = cring_sz;
  cring_sz = sring_sz;
  void *sq_ptr =
      mmap(0, sring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
           ring.ring_fd, IORING_OFF_SQ_RING);

  if (sq_ptr == MAP_FAILED) {
    _TODO("handle this frfr!");
  }
  void *cq_ptr = sq_ptr;

  ring.s_queue.head = (uint32_t *)((size_t)sq_ptr + params->sq_off.head);
  ring.s_queue.tail = (uint32_t *)((size_t)sq_ptr + params->sq_off.tail);
  ring.s_queue.mask = (uint32_t *)((size_t)sq_ptr + params->sq_off.ring_mask);
  ring.s_queue.array = (uint32_t *)((size_t)sq_ptr + params->sq_off.array);
  ring.s_queue.ring_entries =
      (uint32_t *)((size_t)sq_ptr + params->sq_off.ring_entries);
  ring.s_queue.sqes = mmap(0, params->sq_entries * sizeof(struct io_uring_sqe),
                           PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                           ring.ring_fd, IORING_OFF_SQES);
  if (ring.s_queue.sqes == MAP_FAILED) {
    _TODO("handle this frfr!");
  }
  ring.c_queue.head = (uint32_t *)((size_t)cq_ptr + params->cq_off.head);
  ring.c_queue.tail = (uint32_t *)((size_t)cq_ptr + params->cq_off.tail);
  ring.c_queue.mask = (uint32_t *)((size_t)cq_ptr + params->cq_off.ring_mask);
  ring.c_queue.cqes =
      (struct io_uring_cqe *)((size_t)cq_ptr + params->cq_off.cqes);
  ring.c_queue.ring_entries =
      (uint32_t *)((size_t)cq_ptr + params->cq_off.ring_entries);

  return ring;
}
int io_uring_submit(IoUring *uring) {
  IoUring_SubmissionQueue *s_queue = &uring->s_queue;
  uint32_t n_entries = sq_ready(s_queue);

  _LOG_DEBUG("entering uring! n_entries: %u", n_entries);
  const int ret = io_uring_enter(uring->ring_fd, n_entries, 0,
                                 IORING_ENTER_GETEVENTS, &s_queue);
  _LOG_DEBUG("enter completed! ret: %d, status: %s", ret, strerror(errno));
  _LOG_TODO("make sure that this function almost always succeeds!");
  return ret;
}

// idea stolen from liburing
size_t cq_ready(const IoUring_CompletionQueue *c_queue) {

  uint32_t tail = __atomic_load_n(
      (uint32_t *)c_queue->tail,
      __ATOMIC_ACQUIRE); // we not popping shit with this one :fire:
  uint32_t head = __atomic_load_n(
      (uint32_t *)c_queue->head,
      __ATOMIC_ACQUIRE); // we not popping shit with this one :fire:

  uint32_t available = tail - head;
  // _LOG_DEBUG("number of cqes current in cq: %u", available);
  assert(available <= INT32_MAX);
  return (size_t)available;
}
void cq_advance(const IoUring_CompletionQueue *c_queue, size_t by) {
  uint32_t head = __atomic_load_n(c_queue->head, __ATOMIC_ACQUIRE);
  head += by;
  assert(head >= by); // overflow check
  __atomic_store_n(c_queue->head, head, __ATOMIC_RELEASE);
  return;
}
size_t cq_copy(struct io_uring_cqe *out_cqes,
               const IoUring_CompletionQueue *c_queue, size_t max_count) {
  uint32_t head = __atomic_load_n(c_queue->head, __ATOMIC_ACQUIRE);
  uint32_t tail = __atomic_load_n(c_queue->tail, __ATOMIC_ACQUIRE);
  uint32_t mask = *c_queue->mask;
  uint32_t entries = __atomic_load_n(c_queue->ring_entries, __ATOMIC_ACQUIRE);

  size_t available = MIN(cq_ready(c_queue), max_count);
  const size_t to_wrap = // even if the subtraction underflows, we don't care
      MIN((size_t)(entries - head), available);
  // _LOG_DEBUG("available: %lu", available);
  // _LOG_DEBUG("to_wrap: %lu", to_wrap);

  // memcpy doesn't work for some reason
  for (size_t i = 0; i < to_wrap; i++) {
    out_cqes[i] = c_queue->cqes[(i + head) & mask];
  }

  if (to_wrap < available) {
    const size_t remaining = available - to_wrap;
    assert(remaining < tail);
    for (size_t i = 0; i < remaining; i++) {
      out_cqes[i + to_wrap] = c_queue->cqes[i & mask];
    }
  }

  cq_advance(c_queue, available);
  return available;
}
// returns 0 on success
int io_cq_wait_timeout(IoUring *uring, size_t *n_completions,
                       struct __kernel_timespec timespec) {
  IoUring_CompletionQueue *c_queue = &uring->c_queue;
  IoUring_SubmissionQueue *s_queue = &uring->s_queue;
  IO_URING_STATUS status =
      sq_put(s_queue, (struct io_uring_sqe){.opcode = IORING_OP_TIMEOUT,
                                            .addr = (uintptr_t)&timespec,
                                            .fd = -1,
                                            .user_data = IO_URING_TIMEOUT});
  assert(status == IO_URING_SUCCESS);

  uint32_t n_entries = sq_ready(s_queue);

  // _LOG_DEBUG("entering uring! n_entries: %u", n_entries);
  const int ret = io_uring_enter(uring->ring_fd, n_entries, 1,
                                 IORING_ENTER_GETEVENTS, &s_queue);
  // _LOG_DEBUG("enter completed! ret: %d, status: %s", ret, strerror(errno));
  if (ret < 0) {
    return ret;
  }
  *n_completions = cq_ready(c_queue);
  return 0;
}
void cqe_clone(struct io_uring_cqe *dest, struct io_uring_cqe *cqe) {
  memcpy(dest, cqe, sizeof(*cqe));
}
uint32_t sq_ready(IoUring_SubmissionQueue *s_queue) {
  const uint32_t tail =
      __atomic_load_n((uint32_t *)s_queue->tail, __ATOMIC_ACQUIRE);
  const uint32_t head =
      __atomic_load_n((uint32_t *)s_queue->head, __ATOMIC_ACQUIRE);
  const uint32_t entries = tail - head;

  assert(entries <= INT32_MAX);
  return entries;
}
uint32_t sq_available(struct IoUring_SubmissionQueue *s_queue) {
  const uint32_t ready =
      __atomic_load_n(s_queue->ring_entries, __ATOMIC_ACQUIRE) -
      sq_ready(s_queue);
  assert(ready <= INT32_MAX);
  return ready;
}

IO_URING_STATUS sq_put(struct IoUring_SubmissionQueue *s_queue,
                       struct io_uring_sqe sqe) {
  if (sq_available(s_queue) == 0) {
    return IO_URING_SQ_FULL;
  }
  uint32_t tail = __atomic_load_n((uint32_t *)s_queue->tail, __ATOMIC_ACQUIRE);
  uint32_t idx = tail & (*s_queue->mask);
  struct io_uring_sqe *sqe_ptr = &s_queue->sqes[idx];
  s_queue->array[idx] = idx;
  *sqe_ptr = sqe;
  tail++;
  __atomic_store_n(s_queue->tail, tail, __ATOMIC_RELEASE);
  return IO_URING_SUCCESS;
}
IO_URING_STATUS sq_put_read(struct IoUring_SubmissionQueue *s_queue, int fd,
                            uint8_t *buffer, uint32_t len, uintptr_t userdata) {
  const IO_URING_STATUS status =
      sq_put(s_queue, (struct io_uring_sqe){
                          .opcode = (uint8_t)IORING_OP_READ,
                          .fd = fd,
                          .addr = (uintptr_t)buffer,
                          .len = len,
                          .user_data = userdata,
                      });
  assert(status == IO_URING_SUCCESS);
  return status;
}
IO_URING_STATUS sq_put_write(struct IoUring_SubmissionQueue *s_queue, int fd,
                             uint8_t *buffer, uint32_t size,
                             uintptr_t userdata) {
  const IO_URING_STATUS status =
      sq_put(s_queue, (struct io_uring_sqe){
                          .opcode = (uint8_t)IORING_OP_WRITE,
                          .fd = fd,
                          .addr = (uintptr_t)buffer,
                          .len = size,
                          .user_data = userdata,
                      });
  assert(status == IO_URING_SUCCESS);
  return status;
}

// get's the unsubmitted entry
struct io_uring_sqe *sq_get(struct IoUring_SubmissionQueue *s_queue,
                            size_t idx) {
  size_t n_entries = sq_ready(s_queue);
  assert(idx <= n_entries);
  return sq_get_unsafe(s_queue, idx);
}
struct io_uring_sqe *sq_get_unsafe(struct IoUring_SubmissionQueue *s_queue,
                                   size_t idx) {
  uint32_t head = __atomic_load_n((uint32_t *)s_queue->head, __ATOMIC_ACQUIRE);
  uint32_t array_idx = (head + idx) & (*s_queue->mask);
  struct io_uring_sqe *sqe_ptr = &s_queue->sqes[array_idx];
  return sqe_ptr;
}

internal bool __attribute__((__unused__)) io_ring_needs_enter(IoUring *uring) {
  if (!(uring->flags & IORING_SETUP_SQPOLL)) {
    _TODO("implement for IORING_SETUP_SQPOLL");
  }
  return true;
  _UNREACHABLE();
}

// mfw i figured out that cstdlib doesn't provide syscalls for io_uring:
// https://media1.tenor.com/m/9eivCozZGasAAAAC/kid-raging-tokyojns.gif
int io_uring_setup(uint16_t entries, struct io_uring_params *params) {
  assert(params != NULL);
  return (int)syscall(SYS_io_uring_setup, entries, params);
}
int io_uring_register(int fd, uint32_t opcode, const void *arg,
                      unsigned int nr_args) {
  return (int)syscall(SYS_io_uring_register, fd, opcode, arg, nr_args);
}

int io_uring_enter2(int fd, unsigned int to_submit, unsigned int min_complete,
                    unsigned int flags, const void *arg, size_t sz) {
  return (int)syscall(SYS_io_uring_enter, fd, to_submit, min_complete, flags,
                      arg, sz);
}
int io_uring_enter(int fd, unsigned int to_submit, unsigned int min_complete,
                   unsigned int flags, const void *arg) {
  return io_uring_enter2(fd, to_submit, min_complete, flags, arg, NSIG);
}
