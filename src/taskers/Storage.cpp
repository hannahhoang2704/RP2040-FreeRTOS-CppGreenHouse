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

    uint16_t index = mEEPROM.get(EEPROM::LOG_INDEX_ADDR);
    Logger::log("Index stored in eeprom is %lu\n", index);
    index = (index > EEPROM::MAX_ENTRIES) ? 0 : index;
    Logger::log("Index after verify from eeprom is %lu\n", index);
    mEEPROM.set_log_index_value(index);

    Logger::log("Read value %d\n", mEEPROM.get(EEPROM::CO2_TARGET_ADDR));
    uint16_t val = 1200;
    std::string pw = "pw2024";
    std::string second_str = "usernameIoT";
    std::string ip = "192.168.10.10";

    mEEPROM.put(EEPROM::CO2_TARGET_ADDR, val);
    Logger::log("Read value %d\n", mEEPROM.get(EEPROM::CO2_TARGET_ADDR));

    mEEPROM.put(EEPROM::PW_ADDR, pw);
    mEEPROM.put(EEPROM::USERNAME_ADDR, second_str);
    mEEPROM.put(EEPROM::IP_ADDR, ip);

    std::string pw_from_storage = mEEPROM.get_str(EEPROM::PW_ADDR);
    Logger::log("pw get from storage is %s\n", pw_from_storage.c_str());
    std::string usrname = mEEPROM.get_str(EEPROM::USERNAME_ADDR);
    Logger::log("usrname get from storage is %s\n", usrname.c_str());
    std::string ip_addr = mEEPROM.get_str(EEPROM::IP_ADDR);
    Logger::log("ip addr get from storage is %s\n", ip_addr.c_str());

    mEEPROM.put_log_entry(pw.c_str());
    mEEPROM.put_log_entry(second_str.c_str());

    mEEPROM.get_log_entry();
    while (true) {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}