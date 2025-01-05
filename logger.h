#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <pthread.h>

extern pthread_mutex_t global_print_mutex;

void logger_log(const char* format, ...);

void logger_start();
void logger_stop();
void logger_write(char const* message);

#endif
