//
// Created by Hanh Hoang on 30.9.2024.
//
#include "Storage.h"
#include "Logger.h"

Storage::Storage(const std::shared_ptr<PicoI2C>&i2c_sp,  RTOS_infrastructure RTOS_infrastructure):
    mEEPROM(i2c_sp),
    iRTOS(RTOS_infrastructure) {
    xTaskCreate(task_storage, "Storage", 256, (void *) this,
                tskIDLE_PRIORITY + 1, &mTaskHandle);
    if(mTaskHandle != nullptr){
        Logger::log("Created STORAGE task.\n");
    } else{
        Logger::log("Failed to create STORAGE task.\n");
    }
}

void Storage::task_storage(void *params) {
    auto object_ptr = static_cast<Storage *>(params);
    object_ptr->storage();
}

void Storage::storage() {
    mEEPROM.set_log_index_value();
    if(mEEPROM.get(EEPROM::CO2_TARGET_ADDR, mCO2Target)){
        Logger::log("Stored CO2 target is %u\n", mCO2Target);
        if(mCO2Target >= CO2_MIN && mCO2Target <= CO2_MAX){
            xQueueOverwrite(iRTOS.qCO2TargetCurrent, &mCO2Target);
        }
    }

    std::string stored_api;
    std::string stored_pw;
    std::string stored_ssid;
    if(mEEPROM.get_str(EEPROM::API_ADDR, stored_api)){
        Logger::log("IP is stored in eeprom is %s\n", stored_api.c_str());
        xQueueOverwrite(iRTOS.qNetworkStrings[NEW_API], stored_api.c_str());
    }
    if(mEEPROM.get_str(EEPROM::PW_ADDR, stored_pw)){
        Logger::log("PW is stored in eeprom is %s\n", stored_pw.c_str());
        xQueueOverwrite(iRTOS.qNetworkStrings[NEW_PW], stored_pw.c_str());
    }
    if(mEEPROM.get_str(EEPROM::USERNAME_ADDR, stored_ssid)){
        Logger::log("SSID is stored in eeprom is %s\n", stored_ssid.c_str());
        xQueueOverwrite(iRTOS.qNetworkStrings[NEW_SSID], stored_ssid.c_str());
    }

    while (true) {
        if(xQueueReceive(iRTOS.qStorageQueue, &storageData, portMAX_DELAY) == pdTRUE){
            switch(storageData){
                case CO2_target:
                    if(xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2Target, 0) == pdTRUE){
                        Logger::log("new CO2 target %u will be stored in EEPROM\n", mCO2Target);
                        mEEPROM.put(EEPROM::CO2_TARGET_ADDR, (uint16_t)mCO2Target);
                    }else{
                        Logger::log("CO2 target queue is empty\n");
                    }
                    break;
                case API_str:
                    if(xQueuePeek(iRTOS.qNetworkStrings[NEW_API], &mAPI, 0) == pdTRUE) {
                        Logger::log("new API %s will be stored in EEPROM\n", mAPI);
                        mEEPROM.put(EEPROM::API_ADDR, mAPI);
                    }else{
                        Logger::log("API queue is empty\n");
                    }
                    break;
                case PW_str:
                    if(xQueuePeek(iRTOS.qNetworkStrings[NEW_PW], &mPW, 0) == pdTRUE) {
                        Logger::log("new PW %s will be stored in EEPROM\n", mPW);
                        mEEPROM.put(EEPROM::PW_ADDR, mPW);
                    }else{
                        Logger::log("PW queue is empty\n");
                    }
                    break;
                case SSID_str:
                    if(xQueuePeek(iRTOS.qNetworkStrings[NEW_SSID], &mSSID, 0) == pdTRUE) {
                        Logger::log("new SSID %s will be stored in EEPROM\n", mSSID);
                        mEEPROM.put(EEPROM::USERNAME_ADDR, mSSID);
                    }else{
                        Logger::log("SSID queue is empty\n");
                    }
                    break;
                default:
                    Logger::log("Unknown storage data\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}