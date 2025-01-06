// Student: Eitan Papp ID: 20573981

#include "event_source.h"
#include "utilities.h"
#include "simulator.h"
#include "logger.h"
#include <pthread.h>

static pthread_t event_thread;
static int event_running = 0;
static pthread_mutex_t event_running_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *event_thread_routine(void *arg) {
    useconds_t interval = *((useconds_t *)arg);
    free(arg);
    while (1) {
        pthread_mutex_lock(&event_running_mutex);
        int running = event_running;
        pthread_mutex_unlock(&event_running_mutex);
        if (!running) break;

        simulator_event();
        usleep(interval);
    }
    return NULL;
}

void event_source_start(useconds_t interval) {
    if (event_running) return;
    pthread_mutex_lock(&event_running_mutex);
    event_running = 1;
    pthread_mutex_unlock(&event_running_mutex);
    useconds_t *interval_ptr = malloc(sizeof(useconds_t));
    if (interval_ptr == NULL) {
        logger_write("Failed to allocate memory for interval");
        pthread_mutex_lock(&event_running_mutex);
        event_running = 0;
        pthread_mutex_unlock(&event_running_mutex);
        return;
    }
    *interval_ptr = interval;
    if (pthread_create(&event_thread, NULL, event_thread_routine, interval_ptr) != 0) {
        logger_write("Failed to create event thread");
        free(interval_ptr);
        pthread_mutex_lock(&event_running_mutex);
        event_running = 0;
        pthread_mutex_unlock(&event_running_mutex);
        return;
    }
}

void event_source_stop() {
    pthread_mutex_lock(&event_running_mutex);
    if (!event_running) {
        pthread_mutex_unlock(&event_running_mutex);
        return;
    }
    event_running = 0;
    pthread_mutex_unlock(&event_running_mutex);
    pthread_join(event_thread, NULL);
    
    // ...existing cleanup code...
}

