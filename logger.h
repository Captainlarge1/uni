#ifndef _LOGGER_H_
#define _LOGGER_H_

void logger_log(const char* format, ...);

void logger_start();
void logger_stop();
void logger_write(char const* message);

#endif
