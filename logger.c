#include "logger.h"
#include "utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

static int message_count = 0;
static pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;

void logger_start() {
}

void logger_stop() {
}

void logger_write(char const* message) {
    pthread_mutex_lock(&logger_mutex);

    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);

    printf("%d : %02d:%02d:%02d : %s\n", message_count++, local_time->tm_hour, local_time->tm_min, local_time->tm_sec, message);

    pthread_mutex_unlock(&logger_mutex);
}
