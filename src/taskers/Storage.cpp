//
// Created by Hanh Hoang on 30.9.2024.
//
#include "Storage.h"
#include "Logger.h"

Storage::Storage(const std::shared_ptr<PicoI2C>&i2c_sp): mEEPROM(i2c_sp) {
    xTaskCreate(task_storage, "Storage", 256, this, 1, &mTaskHandle);
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
//    Logger::log("Read value %d\n", mEEPROM.get(EEPROM::CO2_TARGET_ADDR));
//    uint16_t val = 2000;
    std::string pw = "pw";
    std::string second_str = "test";
    std::string str = "test the log2";

//    mEEPROM.put(EEPROM::CO2_TARGET_ADDR, val);
//    Logger::log("Read value %d\n", mEEPROM.get(EEPROM::CO2_TARGET_ADDR));

    mEEPROM.put(20, pw);
    mEEPROM.put(EEPROM::USERNAME_ADDR, second_str);
    std::string pw_from_storage = mEEPROM.get_str(20);
    Logger::log("pw get from storage is %s\n", pw_from_storage.c_str());
//    std::string get_str = mEEPROM.get_str(125);
//    Logger::log("read str: %s\n", get_str.c_str());
//    mEEPROM.put_log_entry(pw.c_str());
//    mEEPROM.put_log_entry(second_str.c_str());
//    mEEPROM.put_log_entry(str.c_str());

//    mEEPROM.get_log_entry();
    while (true) {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}