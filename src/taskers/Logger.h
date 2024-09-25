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

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


#include "uart/PicoOsUart.h"
#define BUFFER_SIZE 125

class Logger{
public:
    Logger(std::shared_ptr<PicoOsUart> uart_sp);
    static void log(const char *format, uint32_t d1, uint32_t d2);

private:
    void run();
    static void logger_task(void * params);
    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mCLI_UART;
    static QueueHandle_t mSyslog_queue;
    struct debugEvent {
        const char *format;
        uint32_t data[3];
    }mDebugEvent;

};
#endif //FREERTOS_GREENHOUSE_LOGGER_H
