#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

// Environment control functions
void environment_start(unsigned int thread_count,
                      unsigned int iterations,
                      unsigned int batch_size);
void environment_stop();

#endif
