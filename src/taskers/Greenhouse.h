#ifndef RP2040_FREERTOS_IRQ_GREENHOUSE_H
#define RP2040_FREERTOS_IRQ_GREENHOUSE_H


#include <memory>
#include <utility>
#include <iostream>

#include "uart/PicoOsUart.h"
#include "LED.h"
#include "modbus/ModbusRegister.h"
#include "Sensor.h"

class Greenhouse {
public:
    Greenhouse(const std::shared_ptr<ModbusClient> &modbus_client);

    const std::string mTaskName;
private:
    void automate_greenhouse();

    static void task_automate_greenhouse(void *params) {
        auto object_ptr = static_cast<Greenhouse *>(params);
        object_ptr->automate_greenhouse();
    }

    TaskHandle_t mTaskHandle;
    Sensor::CO2 mCO2;
    Sensor::Humidity mHumidity;
    Sensor::Temperature mTemperature;
    // missing pressure sensor
    ModbusRegister mMIO12_V;
};

#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
