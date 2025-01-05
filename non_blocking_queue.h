#ifndef _NON_BLOCKING_QUEUE_H_
#define _NON_BLOCKING_QUEUE_H_

#include "list.h"
#include <pthread.h>

typedef struct NonBlockingQueue {
  unsigned int* data; // Pointer to the queue's data
  int capacity;       // Maximum capacity of the queue
  int front;          // Index of the front element
  int rear;           // Index of the rear element
  int size;           // Current size of the queue
  pthread_mutex_t mutex; // Add mutex to protect the queue
} NonBlockingQueueT;

// Change return type from void to int
int non_blocking_queue_create(NonBlockingQueueT* queue);
void non_blocking_queue_destroy(NonBlockingQueueT* queue);

int non_blocking_queue_push(NonBlockingQueueT* queue, unsigned int value);
int non_blocking_queue_pop(NonBlockingQueueT* queue, unsigned int* value);

int non_blocking_queue_empty(NonBlockingQueueT* queue);
int non_blocking_queue_length(NonBlockingQueueT* queue);

#endif
