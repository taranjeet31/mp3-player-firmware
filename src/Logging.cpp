// Logging.cpp — FreeRTOS mutex-serialized serial output for dual-core ESP32-S3.
//
// Design constraints enforced here:
//  - Mutex acquire timeout: 10 ms (pdMS_TO_TICKS(10)).  Keeps real-time tasks
//    from stalling on a contested log write.
//  - ISR guard: both public functions return immediately if called from ISR
//    context (xPortInIsrContext() == true).  The logging mutex must never be
//    taken from interrupt context.
//  - Drop-on-fail: if the mutex cannot be acquired within the timeout, the
//    diagnostic message is silently discarded.  Correctness of audio/UI is
//    never sacrificed for a log line.

#include "Logging.h"
#include <Arduino.h>
#include <cstdarg>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Logging timeout: 10 ms.  Generous enough for a clean write; short enough to
// never block a real-time audio callback.
static constexpr TickType_t LOG_MUTEX_TIMEOUT_TICKS = pdMS_TO_TICKS(10);

// Internal mutex — never exposed to callers.
static SemaphoreHandle_t xLogMutex = nullptr;

void initLogging() {
    // Guard against double-initialisation (e.g. if HAL::init() is called twice).
    if (xLogMutex != nullptr) {
        return;
    }
    xLogMutex = xSemaphoreCreateMutex();
    if (xLogMutex == nullptr) {
        // Cannot use logPrintf here — the mutex doesn't exist yet.
        Serial.println("[Logging] FATAL: Failed to create log mutex!");
    }
}

void logPrintf(const char* format, ...) {
    // Rule: never take a mutex from ISR context.
    if (xPortInIsrContext()) {
        return;
    }
    if (xLogMutex == nullptr) {
        return;  // initLogging() was not called; drop silently.
    }

    if (xSemaphoreTake(xLogMutex, LOG_MUTEX_TIMEOUT_TICKS) != pdTRUE) {
        // Mutex not available within timeout — drop the message.
        return;
    }

    // HWCDC (ESP32-S3 USB-CDC) does not expose vprintf(), so format into a
    // stack buffer and write the result.  Lines longer than the buffer are
    // silently truncated — acceptable for diagnostics.
    va_list args;
    va_start(args, format);
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.print(buf);

    xSemaphoreGive(xLogMutex);
}

void logPrintln(const char* message) {
    // Rule: never take a mutex from ISR context.
    if (xPortInIsrContext()) {
        return;
    }
    if (xLogMutex == nullptr) {
        return;
    }

    if (xSemaphoreTake(xLogMutex, LOG_MUTEX_TIMEOUT_TICKS) != pdTRUE) {
        return;
    }

    Serial.println(message);

    xSemaphoreGive(xLogMutex);
}
