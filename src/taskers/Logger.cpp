//
// Created by Hanh Hoang on 24.9.2024.
//
#include <cstdarg>
#include "Logger.h"

QueueHandle_t Logger::mSyslog_queue = xQueueCreate(30, sizeof(debugEvent));

Logger::Logger(std::shared_ptr<PicoOsUart> uart_sp): mCLI_UART(std::move(uart_sp)){
    vQueueAddToRegistry(mSyslog_queue, "Syslog");
    if(xTaskCreate(Logger::logger_task, "logger_task", 512, (void *) this,
                   tskIDLE_PRIORITY + 1,&mTaskHandle) == pdPASS){
        Logger::log("Created LOGGER task.\n");
    } else {
        Logger::log("Failed to create LOGGER task.\n");
    }
}

void Logger::logger_task(void *params) {
    Logger *logger = static_cast<Logger *>(params);
    logger->run();
}

const char* Logger::get_task_name() {
    const char *taskName;
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        if (currentTask != nullptr) {
            taskName = pcTaskGetName(currentTask);
        } else {
            taskName = "Unknown";
        }
    } else {
        taskName = "Pre-scheduler";
    }
    return taskName;
}

void Logger::log(const char *format, ...) {
    uint64_t timestamp = time_us_64() / 1000000;
    const char *taskName = get_task_name();
    char buf[BUFFER_SIZE];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    debugEvent event = {.timestamp = timestamp, .taskName = taskName};
    strncpy(event.message, buf, sizeof(event.message) - 1);
    event.message[sizeof(event.message) - 1] = '\0';
    xQueueSendToBack(mSyslog_queue, &event, 0);
}

void Logger::run() {
    while (true) {
        if (xQueueReceive(mSyslog_queue, &mDebugEvent, portMAX_DELAY) == pdTRUE) {
            int offset = snprintf(buffer, sizeof(buffer), "[%llu s] [%s] ", mDebugEvent.timestamp, mDebugEvent.taskName);
            strncat(buffer, mDebugEvent.message, sizeof(buffer) - offset);
            mCLI_UART->send(buffer);
        }
    }
}
