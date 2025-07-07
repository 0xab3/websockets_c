#ifndef __IO_URING_H__
#define __IO_URING_H__
#include <linux/io_uring.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>

#ifndef internal
#define internal static inline
#endif // internal

#define IO_URING_TIMEOUT -1ULL
typedef enum IO_URING_STATUS {
  IO_URING_SUCCESS,
  IO_URING_SQ_FULL
} IO_URING_STATUS;
typedef struct IoUring_SubmissionQueue {
  uint32_t *head;
  uint32_t *tail;
  uint32_t *array;
  uint32_t *mask;
  uint32_t *ring_entries;
  struct io_uring_sqe *sqes;
} IoUring_SubmissionQueue;

typedef struct IoUring_completionQueue {
  uint32_t *head;
  uint32_t *tail;
  uint32_t *mask;
  uint32_t *ring_entries;
  struct io_uring_cqe *cqes;
} IoUring_CompletionQueue;

typedef struct IoUring {
  int ring_fd;
  uint32_t flags;
  char padding[4];
  IoUring_SubmissionQueue s_queue;
  IoUring_CompletionQueue c_queue;
} IoUring;

IoUring io_uring_init(struct io_uring_params *params);
int io_uring_submit(IoUring *uring);
size_t cq_ready(const IoUring_CompletionQueue *c_queue);
void cq_advance(const IoUring_CompletionQueue *c_queue, size_t by);
int io_cq_wait_timeout(IoUring *uring, size_t *n_completions,
                       struct __kernel_timespec timespec);
void cqe_clone(struct io_uring_cqe *dest, struct io_uring_cqe *cqe);
size_t cq_copy(struct io_uring_cqe *out_cqes,
               const IoUring_CompletionQueue *c_queue, size_t max_count);

IO_URING_STATUS sq_put(struct IoUring_SubmissionQueue *s_queue,
                       struct io_uring_sqe sqe);
IO_URING_STATUS sq_put_read(struct IoUring_SubmissionQueue *s_queue, int fd,
                            uint8_t *buffer, uint32_t size, uintptr_t userdata);
IO_URING_STATUS sq_put_write(struct IoUring_SubmissionQueue *s_queue, int fd,
                             uint8_t *buffer, uint32_t size,
                             uintptr_t userdata);
struct io_uring_sqe *sq_get(struct IoUring_SubmissionQueue *s_queue,
                            size_t idx);
struct io_uring_sqe *sq_get_unsafe(struct IoUring_SubmissionQueue *s_queue,
                                   size_t idx);

uint32_t sq_ready(IoUring_SubmissionQueue *s_queue);
uint32_t sq_available(struct IoUring_SubmissionQueue *s_queue);

int io_uring_setup(uint16_t entries, struct io_uring_params *p);
int io_uring_register(int fd, uint32_t opcode, const void *arg,
                      unsigned int nr_args);

int io_uring_enter2(int fd, unsigned int to_submit, unsigned int min_complete,
                    unsigned int flags, const void *arg, size_t sz);
int io_uring_enter(int fd, unsigned int to_submit, unsigned int min_complete,
                   unsigned int flags, const void *arg);
void *_mmap_alloc_shared_mem(void);
#endif // __IO_URING_H__
