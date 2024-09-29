//
// Created by Hanh Hoang on 30.9.2024.
//

#ifndef FREERTOS_GREENHOUSE_STORAGE_H
#define FREERTOS_GREENHOUSE_STORAGE_H


#include <memory>
#include "i2c/PicoI2C.h"
#include "EEPROM.h"

class Storage {
public:
    Storage(const std::shared_ptr<PicoI2C>& i2c_sp);
private:
    void storage();
    static void task_storage(void * params);
    EEPROM mEEPROM;
    TaskHandle_t mTaskHandle;
};
#endif //FREERTOS_GREENHOUSE_STORAGE_H
