#ifndef _EVALUATOR_H_
#define _EVALUATOR_H_

#define TIME_SLICE_LENGTH 100

typedef enum EvaluatorReason {
    reason_timeslice_ended,
    reason_terminated,
    reason_blocked // Added if used in evaluator.c
} EvaluatorReasonT;

typedef struct EvaluatorResult {
    EvaluatorReasonT reason;
    unsigned int PC;         // Added field
    unsigned int cpu_time;   // Added field
} EvaluatorResultT;

typedef struct EvaluatorCode {
    unsigned int termination_step; // Step at which to terminate
    unsigned int current_step;     // Current step count
} EvaluatorCodeT;

// Initialize an EvaluatorCodeT to terminate after 'steps' steps
EvaluatorCodeT evaluator_terminates_after(unsigned int steps);

// Evaluate the process code, increment step count
EvaluatorResultT evaluator_evaluate(EvaluatorCodeT* code, unsigned int param); // Changed to pointer

#endif
