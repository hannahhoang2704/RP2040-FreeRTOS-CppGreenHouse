//
// Created by Hanh Hoang on 24.9.2024.
//
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

void Logger::log(const char *format, uint32_t d1, uint32_t d2) {
    uint64_t timestamp = time_us_32() / 1000000;
    debugEvent event = {.format = format, .timestamp = timestamp, .data = { d1, d2}};
    xQueueSendToBack(mSyslog_queue, &event, 0);
}

void Logger::run() {
    while(true){
        if(xQueueReceive(mSyslog_queue, &mDebugEvent, portMAX_DELAY) == pdTRUE){
            offset = snprintf(buffer, sizeof(buffer), "[%llu ms] ", mDebugEvent.timestamp);
            snprintf(buffer + offset, sizeof(buffer) - offset, mDebugEvent.format, mDebugEvent.data[0], mDebugEvent.data[1]);
            mCLI_UART->send(buffer);
        }
    }
}