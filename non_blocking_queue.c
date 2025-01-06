// Student: Eitan Papp ID: 20573981

#include "non_blocking_queue.h"
#include "utilities.h"

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

int non_blocking_queue_create(NonBlockingQueueT* queue) {
  // Initialize the mutex
  if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
    // Handle mutex initialization failure
    return -1;
  }

  // Preallocate a larger capacity to avoid repeated malloc calls
  queue->capacity = 1024; // Adjust as needed
  queue->data = (unsigned int*)malloc(queue->capacity * sizeof(unsigned int));
  if (queue->data == NULL) {
    pthread_mutex_destroy(&queue->mutex);
    return -1;
  }
  queue->front = 0;
  queue->rear = -1;
  queue->size = 0;

  return 0;
}

void non_blocking_queue_destroy(NonBlockingQueueT* queue) {
  // Destroy the mutex
  pthread_mutex_destroy(&queue->mutex);

  free(queue->data);
  queue->data = NULL;
  queue->capacity = 0;
  queue->front = 0;
  queue->rear = -1;
  queue->size = 0;
}

int non_blocking_queue_push(NonBlockingQueueT* queue, unsigned int value) {
  pthread_mutex_lock(&queue->mutex);
  // Check if full; do not resize
  if (queue->size == queue->capacity) {
    pthread_mutex_unlock(&queue->mutex);
    return -1; // Indicate failure
  }
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->data[queue->rear] = value;
  queue->size++;
  pthread_mutex_unlock(&queue->mutex);
  return 0; // Success
}

int non_blocking_queue_pop(NonBlockingQueueT* queue, unsigned int* value) {
  pthread_mutex_lock(&queue->mutex);
  if (queue->size == 0) {
    pthread_mutex_unlock(&queue->mutex);
    return -1; // Queue is empty, return non-zero value indicating failure
  }
  *value = queue->data[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
  pthread_mutex_unlock(&queue->mutex);
  return 0; // Success
}

int non_blocking_queue_empty(NonBlockingQueueT* queue) {
  return queue->size == 0;
}

int non_blocking_queue_length(NonBlockingQueueT* queue) {
  return queue->size;
}
