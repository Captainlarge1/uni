// Student: Eitan Papp ID: 20573981

#include "non_blocking_queue.h"
#include "utilities.h"

#include <assert.h>

void test_success_example()
{
  NonBlockingQueueT queue;
  non_blocking_queue_create(&queue);

  assert(non_blocking_queue_empty(&queue) == 1);
  assert(non_blocking_queue_length(&queue) == 0);

  non_blocking_queue_push(&queue, 42);
  assert(non_blocking_queue_empty(&queue) == 0);
  assert(non_blocking_queue_length(&queue) == 1);

  unsigned int value;
  assert(non_blocking_queue_pop(&queue, &value) == 0);
  assert(value == 42);
  assert(non_blocking_queue_empty(&queue) == 1);
  assert(non_blocking_queue_length(&queue) == 0);

  non_blocking_queue_destroy(&queue);
}

void test_failure_example()
{
  NonBlockingQueueT queue;
  non_blocking_queue_create(&queue);

  unsigned int value;
  assert(non_blocking_queue_pop(&queue, &value) == -1); // Should fail because the queue is empty

  non_blocking_queue_destroy(&queue);
}

int main()
{
  test_failure_example();
  test_success_example();
  return 0;
}
