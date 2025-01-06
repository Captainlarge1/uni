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
// Remove the check_counter_mutex since we'll make check_counter thread-local
extern pthread_mutex_t global_print_mutex;                            // Declare the global_print_mutex

static pthread_t *threads = NULL;
static int thread_count = 0;
static BlockingQueueT *available_pids = NULL;
static ProcessIdT max_pid_count = 0;
static ProcessControlBlockT *process_table = NULL;
static NonBlockingQueueT *ready_queue = NULL;
static NonBlockingQueueT *event_queue = NULL; // Declare event queue
static int simulator_running = 1;             // Add a flag to control the simulator's running state

static void *simulator_routine(void *arg)
{
    int thread_id = *((int *)arg);
    // Fix order: static must come before __thread
    static __thread int check_counter = 0;

    // Log thread start with thread type identification
    char message[100];
    sprintf(message, "Simulator thread %d started", thread_id);
    logger_write(message);

    free(arg);

    while (1)
    {
        // No need for mutex since check_counter is now thread-local
        check_counter++;
        if (check_counter % 1000 == 0) {
            pthread_mutex_lock(&simulator_state_mutex);
            int running_copy = simulator_running;
            pthread_mutex_unlock(&simulator_state_mutex);
            if (!running_copy)
                break;
        }

        ProcessIdT pid;
        int has_work = 0;
        
        // Try ready queue first
        pthread_mutex_lock(&ready_queue_mutex);
        int ret = non_blocking_queue_pop(ready_queue, &pid);
        pthread_mutex_unlock(&ready_queue_mutex);

        if (ret == 0) {
            has_work = 1;
            pthread_mutex_lock(&process_table_mutex);
            ProcessControlBlockT *pcb = &process_table[pid];
            
            if (pcb->state != terminated) {
                pcb->state = running;
                pthread_mutex_unlock(&process_table_mutex);

                EvaluatorResultT result = evaluator_evaluate(pcb->code, pcb->PC);
                
                pthread_mutex_lock(&process_table_mutex);
                pcb->PC = result.PC;

                switch (result.reason) {
                    case reason_terminated:
                        pcb->state = terminated;
                        pthread_mutex_unlock(&process_table_mutex);
                        blocking_queue_push(available_pids, pid);
                        break;
                        
                    case reason_timeslice_ended:
                        pcb->state = ready;
                        pthread_mutex_unlock(&process_table_mutex);
                        pthread_mutex_lock(&ready_queue_mutex);
                        non_blocking_queue_push(ready_queue, pid);
                        pthread_mutex_unlock(&ready_queue_mutex);
                        break;
                        
                    case reason_blocked:
                        pcb->state = blocked;
                        pthread_mutex_unlock(&process_table_mutex);
                        pthread_mutex_lock(&event_queue_mutex);
                        non_blocking_queue_push(event_queue, pid);
                        pthread_mutex_unlock(&event_queue_mutex);
                        break;
                }
            } else {
                pthread_mutex_unlock(&process_table_mutex);
            }
        }

        // If no work was found, sleep for longer
        if (!has_work) {
            usleep(10000); // Sleep for 10ms instead of 1ms
        }
    }

    return NULL;
}

void simulator_start(int thread_count_param, int max_processes)
{

    max_pid_count = max_processes;

    // Calculate initial queue sizes based on max_processes
    int init_queue_size = max_processes * 2; // Allow for growth

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

    // Pre-fill available PIDs
    for (ProcessIdT pid = 0; pid < max_processes; pid++)
    {
        if (blocking_queue_push(available_pids, pid) != 0)
        {
            logger_write("Failed to push PID to blocking PID queue");
            simulator_stop();
            return;
        }
    }

    // Initialize process table with null values
    process_table = malloc(sizeof(ProcessControlBlockT) * max_processes);
    if (process_table == NULL)
    {
        logger_write("Failed to allocate process table");
        simulator_stop();
        return;
    }
    memset(process_table, 0, sizeof(ProcessControlBlockT) * max_processes);

    // Initialize all PCBs to unallocated state
    for (ProcessIdT pid = 0; pid < max_processes; pid++)
    {
        process_table[pid].state = unallocated;
        process_table[pid].pid = pid;
        process_table[pid].PC = 0;
    }

    // Initialize ready queue
    ready_queue = malloc(sizeof(NonBlockingQueueT));
    if (ready_queue == NULL || non_blocking_queue_create(ready_queue) != 0)
    {
        logger_write("Failed to create ready queue");
        simulator_stop();
        return;
    }

    // Initialize event queue
    event_queue = malloc(sizeof(NonBlockingQueueT));
    if (event_queue == NULL || non_blocking_queue_create(event_queue) != 0)
    {
        logger_write("Failed to create event queue");
        simulator_stop();
        return;
    }

    // Initialize all mutexes
    if (pthread_mutex_init(&process_table_mutex, NULL) != 0 ||
        pthread_mutex_init(&simulator_state_mutex, NULL) != 0 ||
        pthread_mutex_init(&ready_queue_mutex, NULL) != 0 ||
        pthread_mutex_init(&event_queue_mutex, NULL) != 0)
    {
        logger_write("Failed to initialize mutexes");
        simulator_stop();
        return;
    }

    // Set up simulator threads
    thread_count = 2; // Use exactly 2 worker threads
    threads = malloc(sizeof(pthread_t) * thread_count);
    if (threads == NULL)
    {
        logger_write("Failed to allocate thread array");
        simulator_stop();
        return;
    }

    // Set simulator to running state before starting threads
    simulator_running = 1;

    // Start worker threads
    for (int i = 0; i < thread_count; i++)
    {
        int *thread_id = malloc(sizeof(int));
        if (thread_id == NULL)
        {
            logger_write("Failed to allocate thread ID");
            simulator_stop();
            return;
        }
        *thread_id = i;
        if (pthread_create(&threads[i], NULL, simulator_routine, thread_id) != 0)
        {
            logger_write("Failed to create worker thread");
            free(thread_id);
            simulator_stop();
            return;
        }
    }
}

void simulator_stop()
{

    // Signal threads to stop
    pthread_mutex_lock(&simulator_state_mutex);
    simulator_running = 0;
    pthread_mutex_unlock(&simulator_state_mutex);

    // Wait for all simulator threads to finish
    if (threads != NULL)
    {
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }
        free(threads);
        threads = NULL;
    }

    // Clean up any remaining processes
    pthread_mutex_lock(&process_table_mutex);
    for (ProcessIdT pid = 0; pid < max_pid_count; pid++)
    {
        if (process_table[pid].state != unallocated)
        {
            process_table[pid].state = terminated;
            char message[100];
        }
    }
    pthread_mutex_unlock(&process_table_mutex);

    // Clean up queues in order
    if (ready_queue != NULL)
    {
        non_blocking_queue_destroy(ready_queue);
        free(ready_queue);
        ready_queue = NULL;
    }

    if (event_queue != NULL)
    {
        non_blocking_queue_destroy(event_queue);
        free(event_queue);
        event_queue = NULL;
    }

    if (available_pids != NULL)
    {
        blocking_queue_destroy(available_pids);
        free(available_pids);
        available_pids = NULL;
    }

    // Clean up process table
    pthread_mutex_lock(&process_table_mutex);
    if (process_table != NULL)
    {
        free(process_table);
        process_table = NULL;
    }
    pthread_mutex_unlock(&process_table_mutex);

    // Destroy mutexes in reverse order of creation
    pthread_mutex_destroy(&event_queue_mutex);
    pthread_mutex_destroy(&ready_queue_mutex);
    pthread_mutex_destroy(&simulator_state_mutex);
    pthread_mutex_destroy(&process_table_mutex);
}

ProcessIdT simulator_create_process(EvaluatorCodeT code) // Changed to accept by value
{
    // Check if we have available PIDs before trying to create process
    if (available_pids == NULL) {
        logger_write("PID queue not initialized");
        return 0;
    }

    unsigned int tmp_pid = 0;
    if (blocking_queue_pop(available_pids, &tmp_pid) != 0) {
        logger_write("No available PIDs - system may be overloaded");
        return 0;
    }
    ProcessIdT pid = (ProcessIdT)tmp_pid;

    pthread_mutex_lock(&process_table_mutex); // Lock mutex
    // Initialize process control block
    process_table[pid].pid = pid;
    process_table[pid].code = code; // Copy EvaluatorCodeT
    process_table[pid].state = ready;
    process_table[pid].PC = 0;                  // Initialize PC to 0
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
    pthread_mutex_lock(&process_table_mutex);
    if (process_table[pid].state != terminated)
    {
        process_table[pid].state = terminated;
        char message[100];
        sprintf(message, "Killed process with PID %u", pid);
        logger_write(message);
    }
    pthread_mutex_unlock(&process_table_mutex);
}

void simulator_wait(ProcessIdT pid)
{
    // Log that the simulator is waiting for the specified process
    char message[100];
    sprintf(message, "Waiting for process with PID %u", pid);
    logger_write(message);

    // Use more aggressive exponential backoff with timeout
    int sleep_time = 100; // Start with 0.1ms
    const int max_sleep = 10000; // Max 10ms
    const int timeout = 5000000; // 5 second timeout
    int total_wait = 0;

    while (1) {
        pthread_mutex_lock(&process_table_mutex);
        ProcessStateT state = process_table[pid].state;
        pthread_mutex_unlock(&process_table_mutex);

        if (state == terminated)
            break;

        if (total_wait >= timeout) {
            // Force terminate if timeout reached
            logger_write("Wait timeout - forcing process termination");
            simulator_kill(pid);
            break;
        }

        usleep(sleep_time);
        total_wait += sleep_time;
        
        // More aggressive backoff
        sleep_time = (sleep_time * 3/2 > max_sleep) ? max_sleep : sleep_time * 3/2;
        
        // Periodically trigger event processing
        if (total_wait % 100000 == 0) { // Every 100ms
            simulator_event();
        }
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
        sprintf(message, "Moved process %u to the ready queue from event queue", pid);
        logger_write(message);
    }
    else
    {
        // No process to move; optionally log or handle as needed
        // For brevity, no action is taken here
    }
}
