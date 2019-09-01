#ifndef LOGGER_H
#define LOGGER_H
#include <stdio.h>
#include <stdarg.h>

enum LOG_LEVEL{L_DEBUG, L_INFO, L_CRITICAL, L_ERROR};
static const char* LOG_LEVEL_MAPPING[] = {
  "DEBUG", "INFO", "CRITICAL", "ERROR",
};

static int current_level = L_DEBUG;

void log(LOG_LEVEL level, const char* msg, va_list args) {
  if (level >= current_level) {
    printf("%s:\t ", LOG_LEVEL_MAPPING[level]);
    vprintf(msg, args);
    printf("\n");
  }
}

void logDebug(const char* msg, ...) { 
  va_list argptr;
  va_start(argptr, msg);
  log(L_DEBUG, msg, argptr);
  va_end(argptr);
}
void logInfo(const char* msg, ...) { 
  va_list argptr;
  va_start(argptr, msg);
  log(L_INFO, msg, argptr);
  va_end(argptr);
}
void logCritical(const char* msg, ...) { 
  va_list argptr;
  va_start(argptr, msg);
  log(L_CRITICAL, msg, argptr);
  va_end(argptr);
}
void logError(const char* msg, ...) { 
  va_list argptr;
  va_start(argptr, msg);
  log(L_ERROR, msg, argptr);
  va_end(argptr);
}

void log_set_level(LOG_LEVEL level) {
  current_level = level;
}


#endif
