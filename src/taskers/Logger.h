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

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


#include "uart/PicoOsUart.h"
#define BUFFER_SIZE 128

class Logger{
public:
    Logger(std::shared_ptr<PicoOsUart> uart_sp);
    static void log(const char* format, ...);

private:
    void run();
    static void logger_task(void * params);
    static const char *get_task_name();
    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mCLI_UART;
    static QueueHandle_t mSyslog_queue;
    char buffer[256];
    int offset;
    struct debugEvent {
        char message[BUFFER_SIZE];
        uint64_t timestamp;
        const char *taskName;
    }mDebugEvent;

};
#endif //FREERTOS_GREENHOUSE_LOGGER_H
