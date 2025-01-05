#include "blocking_queue.h"
#include "utilities.h"
#include <string.h>

void blocking_queue_terminate(BlockingQueueT* queue) {
  pthread_mutex_lock(&queue->mutex);
  queue->terminated = 1;
  pthread_cond_broadcast(&queue->cond);
  pthread_mutex_unlock(&queue->mutex);
}

int blocking_queue_create(BlockingQueueT* queue) {
  // Zero out the queue struct
  memset(queue, 0, sizeof(BlockingQueueT));

  queue->list = list_create();
  if (queue->list == NULL) {
    return -1;
  }
  if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
    list_destroy(queue->list);
    return -1;
  }
  if (pthread_cond_init(&queue->cond, NULL) != 0) {
    pthread_mutex_destroy(&queue->mutex);
    list_destroy(queue->list);
    return -1;
  }
  return 0;
}

void blocking_queue_destroy(BlockingQueueT* queue) {
  list_destroy(queue->list);
  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->cond);
}

int blocking_queue_push(BlockingQueueT* queue, unsigned int value) {
  pthread_mutex_lock(&queue->mutex);
  list_append(queue->list, value); // Removed return value check
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->mutex);
  return 0; // Always indicate success
}

int blocking_queue_pop(BlockingQueueT* queue, unsigned int* value) {
  pthread_mutex_lock(&queue->mutex);
  while (list_empty(queue->list) && !queue->terminated) { // Correct function name
    pthread_cond_wait(&queue->cond, &queue->mutex);
  }
  if (queue->terminated) {
    pthread_mutex_unlock(&queue->mutex);
    return -1; // Indicate failure due to termination
  }
  *value = list_pop_front(queue->list); // Correct function name
  pthread_mutex_unlock(&queue->mutex);
  return 0; // Indicate success
}

int blocking_queue_empty(BlockingQueueT* queue) {
  pthread_mutex_lock(&queue->mutex);
  int is_empty = list_empty(queue->list); // Correct function name
  pthread_mutex_unlock(&queue->mutex);
  return is_empty;
}

int blocking_queue_length(BlockingQueueT* queue) {
  pthread_mutex_lock(&queue->mutex);
  int length = list_length(queue->list);
  pthread_mutex_unlock(&queue->mutex);
  return length;
}
