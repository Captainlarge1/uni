#include "evaluator.h"
#include "simulator.h"

#include <assert.h>
#include <unistd.h>

#define SMALL_DURATION (unsigned int)(TIME_SLICE_LENGTH / 10)
#define MEDIUM_DURATION (unsigned int)(TIME_SLICE_LENGTH / 2)

#ifndef SLEEP_PER_CPU_CYCLE
#define SLEEP_PER_CPU_CYCLE 5
#endif

EvaluatorResultT evaluator_evaluate(EvaluatorCodeT* code, unsigned int param) {
    EvaluatorResultT result;
    if (code->current_step >= code->termination_step) {
        result.reason = reason_terminated;
        result.PC = code->current_step;
        result.cpu_time = SMALL_DURATION; // Assign appropriate value
    } else {
        result.reason = reason_timeslice_ended;
        code->current_step++;
        result.PC = code->current_step;
        result.cpu_time = TIME_SLICE_LENGTH; // Assign appropriate value
    }
    return result;
}

EvaluatorResultT implementation_cpu_bound(unsigned int PC, unsigned int steps) {
    assert(steps);
    EvaluatorResultT result;
    result.PC = PC + 1;
    if(result.PC == steps) {
        result.reason = reason_terminated;
        result.cpu_time = SMALL_DURATION;
    } else {
        result.reason = reason_timeslice_ended;
        result.cpu_time = TIME_SLICE_LENGTH;
    }
    return result;
}

EvaluatorCodeT evaluator_terminates_after(unsigned int steps) {
    EvaluatorCodeT code;
    code.termination_step = steps;
    code.current_step = 0;
    return code;
}

EvaluatorResultT implementation_infinite_loop(unsigned int PC, unsigned int unused) {
    assert(PC < 2);
    EvaluatorResultT result;
    result.cpu_time = TIME_SLICE_LENGTH;
    result.reason = reason_timeslice_ended;
    result.PC = (PC + 1) % 2; // Cycle between 0 and 1
    return result;
}

EvaluatorResultT implementation_blocking(unsigned int PC, unsigned int PC_max) {
    assert(PC_max);
    assert(PC < PC_max); // Does PC_max steps of computation
    EvaluatorResultT result;
    result.PC = PC + 1;
    if(result.PC == PC_max) {
        result.reason = reason_terminated;
        result.cpu_time = SMALL_DURATION;
    } else if(result.PC % 2) { // even steps block
        result.reason = reason_blocked;
        result.cpu_time = MEDIUM_DURATION;
    } else { // odd steps cpu bound
        result.reason = reason_timeslice_ended;
        result.cpu_time = TIME_SLICE_LENGTH;
    }
    return result;
}

EvaluatorCodeT evaluator_blocking_terminates_after(unsigned int steps) {
    EvaluatorCodeT code;
    code.termination_step = steps;
    code.current_step = 0;
    return code;
}
