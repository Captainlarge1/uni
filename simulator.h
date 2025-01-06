#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include "evaluator.h"
#include <pthread.h>

typedef unsigned int ProcessIdT;

// Core process states
typedef enum ProcessState {
  unallocated,
  ready,
  running,
  blocked,
  terminated
} ProcessStateT;

// Process control block structure
typedef struct {
    ProcessIdT pid;
    EvaluatorCodeT code;
    ProcessStateT state;
    unsigned int PC;
} ProcessControlBlockT;

// Core simulator functions
void simulator_start(int threads, int max_processes);
void simulator_stop();
ProcessIdT simulator_create_process(EvaluatorCodeT code);
void simulator_wait(ProcessIdT pid);
void simulator_kill(ProcessIdT pid);
void simulator_event();

#endif
