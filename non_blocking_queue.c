#include "non_blocking_queue.h"
#include "utilities.h"

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void non_blocking_queue_create(NonBlockingQueueT* queue) {
  // Preallocate a larger capacity to avoid repeated malloc calls
  queue->capacity = 1024; // Adjust as needed
  queue->data = (unsigned int*)malloc(queue->capacity * sizeof(unsigned int));
  queue->front = 0;
  queue->rear = -1;
  queue->size = 0;
  pthread_mutex_init(&queue_mutex, NULL);
}

void non_blocking_queue_destroy(NonBlockingQueueT* queue) {
  free(queue->data);
  queue->data = NULL;
  queue->capacity = 0;
  queue->front = 0;
  queue->rear = -1;
  queue->size = 0;
  pthread_mutex_destroy(&queue_mutex);
}

int non_blocking_queue_push(NonBlockingQueueT* queue, unsigned int value) {
  pthread_mutex_lock(&queue_mutex);
  // Check if full; do not resize
  if (queue->size == queue->capacity) {
    pthread_mutex_unlock(&queue_mutex);
    return -1; // Indicate failure
  }
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->data[queue->rear] = value;
  queue->size++;
  pthread_mutex_unlock(&queue_mutex);
  return 0; // Success
}

int non_blocking_queue_pop(NonBlockingQueueT* queue, unsigned int* value) {
  pthread_mutex_lock(&queue_mutex);
  if (queue->size == 0) {
    pthread_mutex_unlock(&queue_mutex);
    return -1; // Queue is empty, return non-zero value indicating failure
  }
  *value = queue->data[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
  pthread_mutex_unlock(&queue_mutex);
  return 0; // Success
}

int non_blocking_queue_empty(NonBlockingQueueT* queue) {
  return queue->size == 0;
}

int non_blocking_queue_length(NonBlockingQueueT* queue) {
  return queue->size;
}
