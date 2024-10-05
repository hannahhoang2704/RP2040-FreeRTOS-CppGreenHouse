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

    uint16_t stored_co2_target;
    if(mEEPROM.get(EEPROM::CO2_TARGET_ADDR, stored_co2_target)){
        Logger::log("Stored CO2 target is %u\n", stored_co2_target);
        if(stored_co2_target >= CO2_MIN && stored_co2_target <= CO2_MAX){
            xQueueOverwrite(iRTOS.qCO2TargetCurrent, &stored_co2_target);
        }
    }

    std::string stored_ip;
    std::string stored_pw;
    std::string stored_ssid;
    if(mEEPROM.get_str(EEPROM::IP_ADDR, stored_ip)){
        Logger::log("IP is stored in eeprom is %s\n", stored_ip.c_str());
    }
    if(mEEPROM.get_str(EEPROM::PW_ADDR, stored_pw)){
        Logger::log("PW is stored in eeprom is %s\n", stored_pw.c_str());
    }
    if(mEEPROM.get_str(EEPROM::USERNAME_ADDR, stored_ssid)){
        Logger::log("PW is stored in eeprom is %s\n", stored_ssid.c_str());
    }



    // below is just to test the eeprom storage
//    std::string pw = "pw2024";
    std::string second_str = "usernameIoTabced";
//    std::string ip = "192.168.10.10";
//
//    mEEPROM.put(EEPROM::PW_ADDR, pw);
//    mEEPROM.put(EEPROM::USERNAME_ADDR, second_str);
//    mEEPROM.put(EEPROM::IP_ADDR, ip);
//
//    std::string stored_ip_addr;
//    mEEPROM.get_str(EEPROM::IP_ADDR, stored_ip_addr);

//    std::string pw_from_storage = mEEPROM.get_str(EEPROM::PW_ADDR);
//    Logger::log("pw get from storage is %s\n", pw_from_storage.c_str());
//    std::string usrname = mEEPROM.get_str(EEPROM::USERNAME_ADDR);
//    Logger::log("usrname get from storage is %s\n", usrname.c_str());
//    std::string ip_addr = mEEPROM.get_str(EEPROM::IP_ADDR);
//    Logger::log("ip addr get from storage is %s\n", ip_addr.c_str());

//    mEEPROM.put_log_entry(pw.c_str());
    mEEPROM.put_log_entry(second_str.c_str());
    mEEPROM.put_log_entry(second_str.c_str());

    mEEPROM.get_log_entry();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}