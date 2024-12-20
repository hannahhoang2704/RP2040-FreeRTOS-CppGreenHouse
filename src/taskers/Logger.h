//
// Created by Hanh Hoang on 24.9.2024.
//

#ifndef FREERTOS_GREENHOUSE_LOGGER_H
#define FREERTOS_GREENHOUSE_LOGGER_H
#include <memory>
#include <utility>
#include <iostream>
#include <cstring>
#include <hardware/timer.h>
#include <cstdarg>

#include "uart/PicoOsUart.h"
#include "Fmutex.h"
#include "RTOS_infrastructure.h"

#define BUFFER_SIZE 256

class Logger{
public:
    Logger(std::shared_ptr<PicoOsUart> uart_sp, const RTOS_infrastructure * RTOSi);
    static void log(const char* format, ...);
    struct debugEvent {
        char message[BUFFER_SIZE];
        uint64_t timestamp;
        const char *taskName;
    };
private:
    void run();
    static void logger_task(void * params);
    static const char *get_task_name();

    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mCLI_UART;
    static QueueHandle_t mSyslog_queue;
    static Fmutex mLogAccess;
    char buffer[BUFFER_SIZE];
    int offset;
    static uint32_t mLost_Log_event;
    debugEvent mDebugEvent;

};
#endif //FREERTOS_GREENHOUSE_LOGGER_H
