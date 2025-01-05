#include "environment.h"
#include "simulator.h"
#include "utilities.h"
#include "evaluator.h"
#include "list.h"
#include <pthread.h> // Add pthread include
#include <stdio.h>	 // Add for logging
#include "logger.h"  // Include logger header for global_print_mutex

static pthread_t *threads = NULL;								// Declare thread handles
static unsigned int thread_count_var = 0;						// Add thread count storage
static unsigned int iterations_var = 0;							// Add iterations storage
static unsigned int batch_size_var = 0;							// Add batch size storage
static pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to ensure single start message
static unsigned int original_thread_count = 0;					// Add original thread count storage

static void *terminating_routine(void *arg)
{
    unsigned int thread_id = *((unsigned int *)arg);
    free(arg);

    // Each thread creates batch_size * iterations processes
    for (unsigned int i = 0; i < iterations_var; i++)
    {
        for (unsigned int j = 0; j < batch_size_var; j++)
        {
            EvaluatorCodeT code = evaluator_terminates_after(5);
            ProcessIdT pid = simulator_create_process(code);
            simulator_wait(pid);
        }
    }

    return NULL;
}

static void *blocking_routine(void *arg)
{
    unsigned int thread_id = *((unsigned int *)arg);
    free(arg);

    // Each thread creates batch_size * iterations processes
    for (unsigned int i = 0; i < iterations_var; i++)
    {
        for (unsigned int j = 0; j < batch_size_var; j++)
        {
            EvaluatorCodeT code = evaluator_blocking_terminates_after(5);
            ProcessIdT pid = simulator_create_process(code);
            simulator_wait(pid);
        }
    }

    return NULL;
}

void environment_start(unsigned int thread_count,
                       unsigned int iterations,
                       unsigned int batch_size)
{
    original_thread_count = thread_count; // Store original count
    thread_count_var = thread_count * 2;  // Double for both types of threads
    iterations_var = iterations;          // Store iterations
    batch_size_var = batch_size;          // Store batch size
    threads = malloc(sizeof(pthread_t) * thread_count_var);

    // Create regular terminating threads
    for (unsigned int i = 0; i < thread_count; i++)
    {
        unsigned int *thread_id = malloc(sizeof(unsigned int));
        *thread_id = i;
        pthread_create(&threads[i], NULL, terminating_routine, thread_id);
    }

    // Create blocking threads
    for (unsigned int i = 0; i < thread_count; i++)
    {
        unsigned int *thread_id = malloc(sizeof(unsigned int));
        *thread_id = i + thread_count;
        pthread_create(&threads[i + thread_count], NULL, blocking_routine, thread_id);
    }
}

void environment_stop()
{
	// Wait for all threads to finish
	for (unsigned int i = 0; i < thread_count_var; i++)
	{
		pthread_join(threads[i], NULL);
	}
	free(threads); // Clean up thread handles
	threads = NULL;
	
	char message[100];
}
