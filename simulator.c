#include "simulator.h"
#include "list.h"
#include "non_blocking_queue.h"
#include "blocking_queue.h"
#include "utilities.h"
#include "logger.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static pthread_t *threads = NULL;
static int thread_count = 0;
static BlockingQueueT *available_pids = NULL;
static ProcessIdT max_pid_count = 0;

static void *simulator_routine(void *arg)
{
  int thread_id = *((int *)arg);
  char message[100];
  sprintf(message, "Thread %d started", thread_id);
  logger_write(message); // Add back the logging call
  free(arg);
  return NULL;
}

void simulator_start(int thread_count_param, int max_processes)
{
  logger_write("Starting simulator"); // Add startup message
  max_pid_count = max_processes;

  // Initialize PID queue
  available_pids = malloc(sizeof(BlockingQueueT));
  if (available_pids == NULL)
  {
    logger_write("Failed to allocate PID queue");
    return;
  }
  blocking_queue_create(available_pids);

  // Fill queue with available PIDs
  for (ProcessIdT pid = 0; pid < max_processes; pid++)
  {
    blocking_queue_push(available_pids, pid);  // Use unsigned int directly
  }

  // Initialize threads
  thread_count = thread_count_param;
  threads = malloc(sizeof(pthread_t) * thread_count);
  if (threads == NULL)
  {
    logger_write("Failed to allocate memory for threads");
    return;
  }

  for (int i = 0; i < thread_count; i++)
  {
    int *thread_id = malloc(sizeof(int));
    if (thread_id == NULL)
    {
      logger_write("Failed to allocate thread ID memory");
      return;
    }
    *thread_id = i;
    if (pthread_create(&threads[i], NULL, simulator_routine, thread_id) != 0)
    {
      logger_write("Failed to create thread");
      free(thread_id);
      continue;
    }
  }
}

void simulator_stop()
{
  if (threads != NULL)
  {
    for (int i = 0; i < thread_count; i++)
    {
      pthread_join(threads[i], NULL);
    }
    free(threads);
    threads = NULL;
  }
  if (available_pids != NULL)
  {
    blocking_queue_destroy(available_pids);
    free(available_pids);
    available_pids = NULL;
  }
}

ProcessIdT simulator_create_process(EvaluatorCodeT const code)
{
  unsigned int tmp_pid = 0;
  if (blocking_queue_pop(available_pids, &tmp_pid) != 0)
  {
    logger_write("Failed to pop from PID queue");
    return 0;
  }
  ProcessIdT pid = (ProcessIdT)tmp_pid;
  return pid;
}

void simulator_kill(ProcessIdT pid)
{
  if (pid < max_pid_count)
  {
    blocking_queue_push(available_pids, pid);  // Use unsigned int directly
  }
}

void simulator_wait(ProcessIdT pid)
{
}

void simulator_event()
{
}
