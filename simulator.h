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
} ProcessControlBlockT;

// Declare an array for storing processes and a ready queue
// The actual size will be set at runtime

void simulator_start(int threads, int max_processes);
void simulator_stop();

ProcessIdT simulator_create_process(EvaluatorCodeT const code);
void simulator_wait(ProcessIdT pid);
void simulator_kill(ProcessIdT pid);
void simulator_event();

#endif
