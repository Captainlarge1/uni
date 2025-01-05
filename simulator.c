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
static ProcessControlBlockT *process_table = NULL;
static NonBlockingQueueT *ready_queue = NULL;
static int simulator_running = 1; // Add a flag to control the simulator's running state

static void *simulator_routine(void *arg)
{
  int thread_id = *((int *)arg);
  char message[100];
  sprintf(message, "Thread %d started", thread_id);
  logger_write(message); // Add back the logging call
  free(arg);

  while (simulator_running)
  {
    ProcessIdT pid;
    if (non_blocking_queue_pop(ready_queue, &pid) == 0)
    {
      ProcessControlBlockT *pcb = &process_table[pid];
      pcb->state = running;
      
      sprintf(message, "Thread %d running process %u", thread_id, pid);
      logger_write(message);

      EvaluatorResultT result = evaluator_evaluate(pcb->code, 0); // Assume second parameter as needed

      if (result.reason == reason_terminated)
      {
        pcb->state = terminated;
        blocking_queue_push(available_pids, pid);
        sprintf(message, "Process %u terminated", pid);
        logger_write(message);
      }
      else if (result.reason == reason_timeslice_ended)
      {
        pcb->state = ready;
        non_blocking_queue_push(ready_queue, pid);
        sprintf(message, "Process %u timeslice ended", pid);
        logger_write(message);
      }
    }
    else
    {
      // Optionally sleep or yield to prevent busy waiting
      // sleep(1);
    }
  }

  return NULL;
}

void simulator_start(int thread_count_param, int max_processes)
{
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

  // Initialize process table
  process_table = malloc(sizeof(ProcessControlBlockT) * max_processes);
  if (process_table == NULL)
  {
    logger_write("Failed to allocate process table");
    return;
  }
  memset(process_table, 0, sizeof(ProcessControlBlockT) * max_processes);

  // Initialize ready queue
  ready_queue = malloc(sizeof(NonBlockingQueueT));
  if (ready_queue == NULL)
  {
    logger_write("Failed to allocate ready queue");
    return;
  }
  non_blocking_queue_create(ready_queue);

  // Initialize threads
  thread_count = thread_count_param;
  threads = malloc(sizeof(pthread_t) * thread_count);
  if (threads == NULL)
  {
    logger_write("Failed to allocate memory for threads");
    return;
  }

  simulator_running = 1; // Initialize the running flag

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
    // Signal the simulator to stop
    simulator_running = 0;

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

    // Clean up ready_queue
    if (ready_queue != NULL)
    {
        non_blocking_queue_destroy(ready_queue);
        free(ready_queue);
        ready_queue = NULL;
    }

    // Clean up process_table
    if (process_table != NULL)
    {
        free(process_table);
        process_table = NULL;
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

  // Initialize process control block
  process_table[pid].pid = pid;
  process_table[pid].code = code;
  process_table[pid].state = ready;

  // Add process to ready queue
  non_blocking_queue_push(ready_queue, pid);

  // Log process creation
  char message[100];
  sprintf(message, "Created process with PID %u", pid);
  logger_write(message);

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
    // Log that the simulator is waiting for the specified process
    char message[100];
    sprintf(message, "Waiting for process with PID %u", pid);
    logger_write(message);

    // Wait until the process state is terminated
    while (process_table[pid].state != terminated)
    {
        // Optionally sleep to prevent busy waiting
        // sleep(1);
    }

    // Clean up process data
    process_table[pid].state = unallocated;
    process_table[pid].code.implementation = NULL; // Reset the code implementation

    // Recycle the process ID
    blocking_queue_push(available_pids, pid);

    // Log that the process has been waited on and recycled
    sprintf(message, "Process %u has been waited for, cleaned up, and recycled", pid);
    logger_write(message);
}

void simulator_event()
{
}
