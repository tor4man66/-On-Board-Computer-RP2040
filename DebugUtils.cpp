// =====================================================================================================================
// --- Debug Utils Implementation (DebugUtils.cpp) ---
// =====================================================================================================================
#include "DebugUtils.h"
#include "Config.h"

ErrorLogger project_error_logger;

SemaphoreHandle_t xSerialMutex = NULL;

static void ensureSerialMutexCreated() {
    if (xSerialMutex == NULL) {
        xSerialMutex = xSemaphoreCreateMutex();
    }
}

void log_debug(const char* message) {
    if (DEBUG_ENABLED) {
        if (Serial) {
            ensureSerialMutexCreated();
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("[DEBUG] ");
                Serial.println(message);
                xSemaphoreGive(xSerialMutex);
            }
        }
    }
}

void log_debug_str(String message) {
    if (DEBUG_ENABLED) {
        if (Serial) {
            ensureSerialMutexCreated();
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("[DEBUG] ");
                Serial.println(message);
                xSemaphoreGive(xSerialMutex);
            }
        }
    }
}

void log_init_debug(const char* message) {
    if (DEBUG_INITIALIZATION_ENABLED) {
        if (Serial) {
            ensureSerialMutexCreated();
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("[ІНІЦІАЛІЗАЦІЯ] ");
                Serial.println(message);
                xSemaphoreGive(xSerialMutex);
            }
        }
    }
}

void log_init_debug_str(String message) {
    if (DEBUG_INITIALIZATION_ENABLED) {
        if (Serial) {
            ensureSerialMutexCreated();
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("[ІНІЦІАЛІЗАЦІЯ] ");
                Serial.println(message);
                xSemaphoreGive(xSerialMutex);
            }
        }
    }
}

void ErrorLogger::log_error(const char* message) {
    if (Serial) {
        ensureSerialMutexCreated();
        if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
            Serial.print("[ERROR] ");
            Serial.println(message);
            xSemaphoreGive(xSerialMutex);
        }
    }
}

void ErrorLogger::log_error_str(String message) {
    if (Serial) {
        ensureSerialMutexCreated();
        if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
            Serial.print("[ERROR] ");
            Serial.println(message);
            xSemaphoreGive(xSerialMutex);
        }
    }
}
