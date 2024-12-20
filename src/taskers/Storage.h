//
// Created by Hanh Hoang on 30.9.2024.
//

#ifndef FREERTOS_GREENHOUSE_STORAGE_H
#define FREERTOS_GREENHOUSE_STORAGE_H


#include <memory>
#include <string>
#include "i2c/PicoI2C.h"
#include "EEPROM.h"
#include "RTOS_infrastructure.h"


class Storage {
public:
    Storage(const std::shared_ptr<PicoI2C>& i2c_sp,
            const RTOS_infrastructure * RTOS_infrastructure);
    static void store(storage_data command);
private:
    void storage();
    static void task_storage(void * params);
    static uint mLostStores;
    static QueueHandle_t qStorage;
    EEPROM mEEPROM;
    TaskHandle_t mTaskHandle;
    const RTOS_infrastructure * iRTOS;
    storage_data storageData;
    int16_t mCO2Target;
    char mAPI[MAX_CREDENTIAL_STRING_LEN + 1];
    char mPW[MAX_CREDENTIAL_STRING_LEN + 1];
    char mSSID[MAX_CREDENTIAL_STRING_LEN + 1];
    static Fmutex mStoreAccess;
};
#endif //FREERTOS_GREENHOUSE_STORAGE_H
