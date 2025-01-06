#ifndef _NON_BLOCKING_QUEUE_H_
#define _NON_BLOCKING_QUEUE_H_

#include "list.h"
#include <pthread.h>

// Queue structure
typedef struct NonBlockingQueue {
  unsigned int* data;
  int capacity;
  int front;
  int rear;
  int size;
  pthread_mutex_t mutex;
} NonBlockingQueueT;

// Queue operations
int non_blocking_queue_create(NonBlockingQueueT* queue);
void non_blocking_queue_destroy(NonBlockingQueueT* queue);
int non_blocking_queue_push(NonBlockingQueueT* queue, unsigned int value);
int non_blocking_queue_pop(NonBlockingQueueT* queue, unsigned int* value);
int non_blocking_queue_empty(NonBlockingQueueT* queue);
int non_blocking_queue_length(NonBlockingQueueT* queue);

#endif
