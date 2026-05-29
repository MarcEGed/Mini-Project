#include "debug.h"

#include <Arduino.h>
#include <config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <stdio.h>

void LOG(const char* type, const char* msg, va_list args) {
    // do stuff
#if DEBUG
    if (xPortInIsrContext()) {
        return;
    }
    char logBuffer[256];
    vsnprintf(logBuffer, sizeof(logBuffer), msg, args);

    DEBUG_SERIAL.print(millis());
    DEBUG_SERIAL.print(" ");
    DEBUG_SERIAL.print(type);
    DEBUG_SERIAL.print(": ");
    DEBUG_SERIAL.print(logBuffer);
    DEBUG_SERIAL.println("");
    // Causing Guru Meditation Error: Core  1 panic'ed (Interrupt wdt timeout on CPU1). 
    // DEBUG_SERIAL.flush();
#endif
}

void LOG_ERROR(const char* msg, ...) {
    // do stuff
#if DEBUG
    va_list args;
    va_start(args, msg);
    LOG("ERROR", msg, args);
    va_end(args);
#endif
}

void LOG_INFO(const char* msg, ...) {
#if DEBUG
    va_list args;
    va_start(args, msg);
    LOG("INFO", msg, args);
    va_end(args);

#endif
}

void LOG_BLANK(const char* msg, ...) {
#if DEBUG
    va_list args;
    va_start(args, msg);
    LOG("", msg, args);
    va_end(args);

#endif
}

void loggerSetup() {
#if DEBUG
    DEBUG_SERIAL.begin(DEBUG_BAUD);
#endif
}