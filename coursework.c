#include "simulator.h"
#include "environment.h"
#include "event_source.h"
#include "logger.h"

#ifndef SIMULATOR_THREADS
#define SIMULATOR_THREADS 2
#endif

#ifndef SIMULATOR_MAX_PROCESSES
#define SIMULATOR_MAX_PROCESSES 2048
#endif

#ifndef ENVIRONMENT_THREADS
#define ENVIRONMENT_THREADS 2
#endif

#ifndef ITERATIONS
#define ITERATIONS 5
#endif

#ifndef BATCH_SIZE
#define BATCH_SIZE 10
#endif

#ifndef EVENT_SOURCE_INTERVAL
#define EVENT_SOURCE_INTERVAL 10
#endif

int main() {
  logger_start();
  logger_write("Starting simulator");
  simulator_start(SIMULATOR_THREADS, SIMULATOR_MAX_PROCESSES);
  event_source_start(EVENT_SOURCE_INTERVAL);
  environment_start(ENVIRONMENT_THREADS, ITERATIONS, BATCH_SIZE);
  environment_stop();
  event_source_stop();
  simulator_stop();
  logger_write("Stopping simulator");
  logger_stop();
  return 0;
}
