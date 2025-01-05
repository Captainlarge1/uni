#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include "evaluator.h"

typedef unsigned int ProcessIdT;

typedef enum ProcessState {
  unallocated,
  ready,
  running,
  blocked,
  terminated
} ProcessStateT;

typedef struct {
    ProcessIdT pid;
    EvaluatorCodeT code;
    ProcessStateT state;
    unsigned int PC;  // Add PC tracking
} ProcessControlBlockT;

// Declare an array for storing processes and a ready queue
// The actual size will be set at runtime

#include <pthread.h> // Add pthread for mutex

// extern pthread_mutex_t process_table_mutex; // Declare mutex for process table

void simulator_start(int threads, int max_processes);
void simulator_stop();

ProcessIdT simulator_create_process(EvaluatorCodeT code); // Changed to pass by value
void simulator_wait(ProcessIdT pid);
void simulator_kill(ProcessIdT pid);
void simulator_event();

#endif
