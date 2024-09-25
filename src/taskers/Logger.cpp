//
// Created by Hanh Hoang on 24.9.2024.
//
#include "Logger.h"

QueueHandle_t Logger::mSyslog_queue = xQueueCreate(30, sizeof(debugEvent));

Logger::Logger(std::shared_ptr<PicoOsUart> uart_sp): mCLI_UART(std::move(uart_sp)){
    vQueueAddToRegistry(mSyslog_queue, "Syslog");
    if(xTaskCreate(Logger::logger_task, "logger_task", 512, (void *) this,
                   tskIDLE_PRIORITY + 1,&mTaskHandle) == pdPASS){
        mCLI_UART->send("Created LOGGER task.\n");
    } else {
        mCLI_UART->send("Failed to create LOGGER task.\n");
    }
}

void Logger::logger_task(void *params) {
    Logger *logger = static_cast<Logger *>(params);
    logger->run();
}

void Logger::log(const char *format, uint32_t d1, uint32_t d2) {
    uint32_t timestamp = time_us_64() / 1000;
    debugEvent event = {.format = format, .data = {timestamp, d1, d2}};
    xQueueSendToBack(mSyslog_queue, &event, 0);
}

void Logger::run(){
    char buffer[BUFFER_SIZE];
    while(true){
        if(xQueueReceive(mSyslog_queue, &mDebugEvent, portMAX_DELAY) == pdTRUE){
            int offset = snprintf(buffer, sizeof(buffer), "[%u ms] ", mDebugEvent.data[0]);
            snprintf(buffer + offset, sizeof(buffer) - offset, mDebugEvent.format, mDebugEvent.data[1], mDebugEvent.data[2]);
            mCLI_UART->send(buffer);
        }
    }
}