#ifndef DEBUG_H
#define DEBUG_H

#include <config.h>

void LOG_BLANK(const char *msg, ...);
void LOG_INFO(const char *msg, ...);
void LOG_ERROR(const char *msg, ...);
void loggerSetup();

#endif