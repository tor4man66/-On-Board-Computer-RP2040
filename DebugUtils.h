// =====================================================================================================================
// --- Debug Utils (DebugUtils.h) ---
// =====================================================================================================================
#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <semphr.h>

class ErrorLogger {
public:
    void log_error(const char* message);
    void log_error_str(String message);
};

extern ErrorLogger project_error_logger;

extern SemaphoreHandle_t xSerialMutex;

void log_debug(const char* message);
void log_debug_str(String message);
void log_init_debug(const char* message);
void log_init_debug_str(String message);

#endif // DEBUG_UTILS_H
