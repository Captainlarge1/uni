#include "non_blocking_queue.h"
#include "utilities.h"

#include <stdlib.h>
#include <assert.h>

void non_blocking_queue_create(NonBlockingQueueT* queue) {
  queue->capacity = 10; // Initial capacity
  queue->data = (unsigned int*)malloc(queue->capacity * sizeof(unsigned int));
  queue->front = 0;
  queue->rear = -1;
  queue->size = 0;
}

void non_blocking_queue_destroy(NonBlockingQueueT* queue) {
  free(queue->data);
  queue->data = NULL;
  queue->capacity = 0;
  queue->front = 0;
  queue->rear = -1;
  queue->size = 0;
}

void non_blocking_queue_push(NonBlockingQueueT* queue, unsigned int value) {
  if (queue->size == queue->capacity) {
    // Resize the queue if it's full
    queue->capacity *= 2;
    queue->data = (unsigned int*)realloc(queue->data, queue->capacity * sizeof(unsigned int));
  }
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->data[queue->rear] = value;
  queue->size++;
}

int non_blocking_queue_pop(NonBlockingQueueT* queue, unsigned int* value) {
  if (queue->size == 0) {
    return -1; // Queue is empty, return non-zero value indicating failure
  }
  *value = queue->data[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
  return 0; // Success
}

int non_blocking_queue_empty(NonBlockingQueueT* queue) {
  return queue->size == 0;
}

int non_blocking_queue_length(NonBlockingQueueT* queue) {
  return queue->size;
}
