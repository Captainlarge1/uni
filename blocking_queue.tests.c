#include "blocking_queue.h"
#include "utilities.h"

#include <assert.h>

void test_success_example() {
  BlockingQueueT queue;
  blocking_queue_create(&queue);

  blocking_queue_push(&queue, 42);
  unsigned int value;
  int result = blocking_queue_pop(&queue, &value);

  assert(result == 0);
  assert(value == 42);

  blocking_queue_destroy(&queue);
}

void test_failure_example() {
  BlockingQueueT queue;
  blocking_queue_create(&queue);

  blocking_queue_terminate(&queue);
  unsigned int value;
  int result = blocking_queue_pop(&queue, &value);

  assert(result == -1); // Expect failure due to termination

  blocking_queue_destroy(&queue);
}

int main() {
  test_failure_example();
  test_success_example();
  return 0;
}
