#include "simulator.h"
#include "list.h"
#include "non_blocking_queue.h"
#include "blocking_queue.h"
#include "utilities.h"
#include "logger.h"
#include "evaluator.h" // Include evaluator header
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h> // Add for sleep

pthread_mutex_t process_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t simulator_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ready_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t event_queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Add mutex for event queue
extern pthread_mutex_t global_print_mutex; // Declare the global_print_mutex

static pthread_t *threads = NULL;
static int thread_count = 0;
static BlockingQueueT *available_pids = NULL;
static ProcessIdT max_pid_count = 0;
static ProcessControlBlockT *process_table = NULL;
static NonBlockingQueueT *ready_queue = NULL;
static NonBlockingQueueT *event_queue = NULL; // Declare event queue
static int simulator_running = 1; // Add a flag to control the simulator's running state

static void *simulator_routine(void *arg)
{
    int thread_id = *((int *)arg);
    char message[100];
    sprintf(message, "Thread %d started", thread_id);
    logger_write(message); // Add back the logging call
    free(arg);

    while (1)
    {
        pthread_mutex_lock(&simulator_state_mutex);
        int running_copy = simulator_running;
        pthread_mutex_unlock(&simulator_state_mutex);
        if (!running_copy) break;

        ProcessIdT pid;
        pthread_mutex_lock(&ready_queue_mutex);
        int ret = non_blocking_queue_pop(ready_queue, &pid);
        pthread_mutex_unlock(&ready_queue_mutex);

        if (ret == 0)
        {
            pthread_mutex_lock(&process_table_mutex);
            ProcessControlBlockT *pcb = &process_table[pid];
            pcb->state = running;

            // Pass current PC to evaluator and store result
            EvaluatorResultT result = evaluator_evaluate(pcb->code, pcb->PC);
            pcb->PC = result.PC;  // Update PC with result

            if (result.reason == reason_terminated)
            {
                pcb->state = terminated;
                pthread_mutex_unlock(&process_table_mutex); // Unlock before blocking operation
                blocking_queue_push(available_pids, pid);

            }
            else if (result.reason == reason_timeslice_ended)
            {
                pcb->state = ready;
                pthread_mutex_unlock(&process_table_mutex); // Unlock before re-queuing
                pthread_mutex_lock(&ready_queue_mutex);
                non_blocking_queue_push(ready_queue, pid);
                pthread_mutex_unlock(&ready_queue_mutex);

            }
            else if (result.reason == reason_blocked)
            {
                pcb->state = blocked;
                pthread_mutex_unlock(&process_table_mutex); // Unlock before handling blocked state
                // Handle blocked state as needed
                sprintf(message, "Process %u is blocked", pid);
                logger_write(message);

                // Add blocked process to event queue
                pthread_mutex_lock(&event_queue_mutex);
                non_blocking_queue_push(event_queue, pid);
                pthread_mutex_unlock(&event_queue_mutex);
            }
            else
            {
                pthread_mutex_unlock(&process_table_mutex); // Unlock in case of unexpected result
                pthread_mutex_lock(&global_print_mutex);
                printf("Unexpected result.reason: %d for process %u\n", result.reason, pid);
                pthread_mutex_unlock(&global_print_mutex);
            }
        }
        else
        {
            // Optionally sleep to prevent busy waiting
            usleep(1000); // Sleep for 1ms
        }
    }

    sprintf(message, "Thread %d exiting", thread_id);
    logger_write(message);
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
    if (blocking_queue_create(available_pids) != 0)
    {
        logger_write("Failed to create blocking PID queue");
        free(available_pids);
        available_pids = NULL;
        return;
    }

    // Fill queue with available PIDs
    for (ProcessIdT pid = 0; pid < max_processes; pid++)
    {
        if (blocking_queue_push(available_pids, pid) != 0)
        {
            logger_write("Failed to push PID to blocking PID queue");
            // Handle partial initialization if needed
            // For example, stop initialization and clean up
            simulator_stop();
            return;
        }
    }

    // Initialize process table
    process_table = malloc(sizeof(ProcessControlBlockT) * max_processes);
    if (process_table == NULL)
    {
        logger_write("Failed to allocate process table");
        // Clean up previously allocated resources
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
        return;
    }
    memset(process_table, 0, sizeof(ProcessControlBlockT) * max_processes);

    // Initialize ready queue
    ready_queue = malloc(sizeof(NonBlockingQueueT));
    if (ready_queue == NULL)
    {
        logger_write("Failed to allocate ready queue");
        // Clean up previously allocated resources
        free(process_table);
        process_table = NULL;
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
        return;
    }
    if (non_blocking_queue_create(ready_queue) != 0)
    {
        logger_write("Failed to create ready queue");
        free(ready_queue);
        ready_queue = NULL;
        free(process_table);
        process_table = NULL;
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
        return;
    }

    // Initialize event queue
    event_queue = malloc(sizeof(NonBlockingQueueT));
    if (event_queue == NULL)
    {
        logger_write("Failed to allocate event queue");
        // Clean up previously allocated resources
        non_blocking_queue_destroy(ready_queue);
        free(ready_queue);
        ready_queue = NULL;
        free(process_table);
        process_table = NULL;
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
        return;
    }
    if (non_blocking_queue_create(event_queue) != 0)
    {
        logger_write("Failed to create event queue");
        free(event_queue);
        event_queue = NULL;
        non_blocking_queue_destroy(ready_queue);
        free(ready_queue);
        ready_queue = NULL;
        free(process_table);
        process_table = NULL;
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
        return;
    }

    // Verify mutex initialization
    if (pthread_mutex_init(&process_table_mutex, NULL) != 0 ||
        pthread_mutex_init(&simulator_state_mutex, NULL) != 0 ||
        pthread_mutex_init(&ready_queue_mutex, NULL) != 0 ||
        pthread_mutex_init(&event_queue_mutex, NULL) != 0) {
        logger_write("Failed to initialize mutexes");
        return;
    }

    // Initialize threads
    thread_count = thread_count_param;
    threads = malloc(sizeof(pthread_t) * thread_count);
    if (threads == NULL)
    {
        logger_write("Failed to allocate memory for threads");
        // Clean up previously allocated resources
        non_blocking_queue_destroy(event_queue);
        free(event_queue);
        event_queue = NULL;
        non_blocking_queue_destroy(ready_queue);
        free(ready_queue);
        ready_queue = NULL;
        free(process_table);
        process_table = NULL;
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
        return;
    }

    simulator_running = 1; // Initialize the running flag

    for (int i = 0; i < thread_count; i++)
    {
        int *thread_id = malloc(sizeof(int));
        if (thread_id == NULL)
        {
            logger_write("Failed to allocate thread ID memory");
            continue;
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
    pthread_mutex_lock(&simulator_state_mutex);
    simulator_running = 0; 
    pthread_mutex_unlock(&simulator_state_mutex);

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

    // Clean up event_queue
    if (event_queue != NULL)
    {
        non_blocking_queue_destroy(event_queue);
        free(event_queue);
        event_queue = NULL;
    }

    // Clean up process_table
    pthread_mutex_lock(&process_table_mutex); // Lock before cleanup
    if (process_table != NULL)
    {
        free(process_table);
        process_table = NULL;
    }
    pthread_mutex_unlock(&process_table_mutex); // Unlock after cleanup

    // Destroy all dynamically initialized mutexes
    pthread_mutex_destroy(&event_queue_mutex);
    pthread_mutex_destroy(&ready_queue_mutex);
    pthread_mutex_destroy(&simulator_state_mutex);
    pthread_mutex_destroy(&process_table_mutex);
}

ProcessIdT simulator_create_process(EvaluatorCodeT code) // Changed to accept by value
{
    unsigned int tmp_pid = 0;
    if (blocking_queue_pop(available_pids, &tmp_pid) != 0)
    {
        logger_write("Failed to pop from PID queue");
        return 0;
    }
    ProcessIdT pid = (ProcessIdT)tmp_pid;

    pthread_mutex_lock(&process_table_mutex); // Lock mutex
    // Initialize process control block
    process_table[pid].pid = pid;
    process_table[pid].code = code; // Copy EvaluatorCodeT
    process_table[pid].state = ready;
    process_table[pid].PC = 0;  // Initialize PC to 0
    pthread_mutex_unlock(&process_table_mutex); // Unlock mutex

    // Add process to ready queue
    pthread_mutex_lock(&ready_queue_mutex);
    non_blocking_queue_push(ready_queue, pid);
    pthread_mutex_unlock(&ready_queue_mutex);

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
    while (1)
    {
        pthread_mutex_lock(&process_table_mutex); // Lock mutex
        ProcessStateT state = process_table[pid].state;
        pthread_mutex_unlock(&process_table_mutex); // Unlock mutex

        if (state == terminated)
            break;

        // Sleep to prevent busy waiting
        usleep(1000); // Sleep for 1ms
    }

    // Clean up process data
    pthread_mutex_lock(&process_table_mutex); // Lock mutex
    process_table[pid].state = unallocated;
    pthread_mutex_unlock(&process_table_mutex); // Unlock mutex

    // Recycle the process ID
    blocking_queue_push(available_pids, pid);
}

void simulator_event()
{
    ProcessIdT pid;

    // Attempt to pop a PID from the event queue
    if (non_blocking_queue_pop(event_queue, &pid) == 0)
    {
        // Add the PID to the ready queue
        non_blocking_queue_push(ready_queue, pid);

        // Log the action
        char message[100];
        sprintf(message, "Moved process %u to the ready queue", pid);
        logger_write(message);
    }
    else
    {
        // No process to move; optionally log or handle as needed
        // For brevity, no action is taken here
    }
}
