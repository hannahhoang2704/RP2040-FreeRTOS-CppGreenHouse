//
// Created by Hanh Hoang on 30.9.2024.
//

#ifndef FREERTOS_GREENHOUSE_STORAGE_H
#define FREERTOS_GREENHOUSE_STORAGE_H


#include <memory>
#include "i2c/PicoI2C.h"
#include "queue.h"
#include "EEPROM.h"
#include "RTOS_infrastructure.h"


class Storage {
public:
    Storage(const std::shared_ptr<PicoI2C>& i2c_sp, RTOS_infrastructure RTOS_infrastructure);
private:
    void storage();
    static void task_storage(void * params);
    EEPROM mEEPROM;
    TaskHandle_t mTaskHandle;
    RTOS_infrastructure iRTOS;
};
#endif //FREERTOS_GREENHOUSE_STORAGE_H
