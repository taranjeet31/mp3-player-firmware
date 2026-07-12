// Logging.h — Thread-safe serial logging service for dual-core ESP32-S3.
//
// Rules:
//  1. Call initLogging() once after Serial.begin(), before any task creation.
//  2. Use logPrintf() / logPrintln() from any non-ISR task context.
//  3. NEVER call these from the LCD DMA ISR (notify_dma_done) or from any
//     context where xGuiSemaphore is already held and the call could re-enter
//     LVGL.  Both functions return immediately if called from ISR context.
//  4. On mutex acquisition failure the message is silently dropped — real-time
//     audio and UI processing are never blocked by logging.

#pragma once
#include <Arduino.h>

// Creates the internal FreeRTOS mutex.  Must be called once, from setup(),
// after Serial.begin() and before the FreeRTOS tasks are created.
void initLogging();

// printf-style formatted log.  Internally uses Serial.vprintf().
// Acquires xLogMutex with a 10 ms timeout; drops the message on failure.
void logPrintf(const char* format, ...);

// Single-line log with automatic newline.
// Acquires xLogMutex with a 10 ms timeout; drops the message on failure.
void logPrintln(const char* message);
