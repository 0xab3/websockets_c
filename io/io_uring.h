#ifndef __IO_URING_H__
#define __IO_URING_H__
#include <linux/io_uring.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>

#ifndef internal
#define internal static inline
#endif // internal

typedef struct submissionQueue {
  uint32_t *tail;
  uint32_t *array;
  atomic_size_t n_submissions; 
  struct io_uring_sqe *sqes;
  uint32_t *mask;
} submissionQueue;
typedef struct completionQueue {
  uint32_t *head;
  uint32_t *tail;
  atomic_size_t n_completions;
  struct io_uring_cqe *cqes;
  uint32_t *mask;
} completionQueue;
typedef struct ioUring {
  int ring_fd;
  char padding[4];
  submissionQueue s_queue;
  completionQueue c_queue;
} ioUring;

ioUring io_uring_init(struct io_uring_params *params);
struct io_uring_cqe *cq_pop(struct completionQueue *c_queue);
void sq_submit(struct submissionQueue *s_queue, struct io_uring_sqe sqe);

int io_uring_setup(uint16_t entries, struct io_uring_params *p);
int io_uring_register(int fd, uint32_t opcode, const void *arg,
                      unsigned int nr_args);

int io_uring_enter2(int fd, unsigned int to_submit, unsigned int min_complete,
                    unsigned int flags, const void *arg, size_t sz);
int io_uring_enter(int fd, unsigned int to_submit, unsigned int min_complete,
                   unsigned int flags, const void *arg);
void *_mmap_alloc_shared_mem(void);
#endif // __IO_URING_H__
