#define _GNU_SOURCE
#include "io_uring.h"
#include "libs/utils.h"
#include <assert.h>
#include <linux/io_uring.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define QUEUE_DEPTH 1

ioUring io_uring_init(struct io_uring_params *params) {
  struct ioUring ring = {0};
  ring.ring_fd = io_uring_setup(QUEUE_DEPTH, params);
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

  ring.s_queue.tail = (uint32_t *)((size_t)sq_ptr + params->sq_off.tail);
  ring.s_queue.mask = (uint32_t *)((size_t)sq_ptr + params->sq_off.ring_mask);
  ring.s_queue.array = (uint32_t *)((size_t)sq_ptr + params->sq_off.array);
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

  return ring;
}

struct io_uring_cqe *cq_pop(struct completionQueue *c_queue) {
  struct io_uring_cqe *cqe;
  uint32_t head;
  head = __atomic_load_n((uint32_t *)c_queue->head, __ATOMIC_ACQUIRE);
  if (head == *c_queue->tail) {
    return NULL;
  }

  cqe = &c_queue->cqes[head & (*c_queue->mask)];
  head++;
  __atomic_store_n(c_queue->head, head, __ATOMIC_RELEASE);
  return cqe;
}
void sq_submit(struct submissionQueue *s_queue, struct io_uring_sqe sqe) {
  uint32_t tail = __atomic_load_n((uint32_t *)s_queue->tail, __ATOMIC_ACQUIRE);
  uint32_t idx = tail & (*s_queue->mask);
  struct io_uring_sqe *sqe_ptr = &s_queue->sqes[idx];
  s_queue->array[idx] = idx;
  *sqe_ptr = sqe;
  tail++;
  __atomic_store_n(s_queue->tail, tail, __ATOMIC_RELEASE);
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
