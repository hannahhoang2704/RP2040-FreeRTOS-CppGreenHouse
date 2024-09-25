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
    Greenhouse(std::shared_ptr<PicoOsUart> uart_sp, const std::shared_ptr<ModbusClient>& modbus_client, uint lep_pin);

    const int HMP60_ADDRESS{241};
    const int GMP252_ADDRESS{240};
private:
    void automate_greenhouse();
    static void task_automate_greenhouse(void * params) {
        auto object_ptr = static_cast<Greenhouse *>(params);
        object_ptr->automate_greenhouse();
    }

    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mCLI_UART; // only for prints -- switch to logger at some point
    Sensor::GMP252 mGMP252;
    Sensor::HMP60 mHMP60;
    ModbusRegister mMIO12_V;

    std::shared_ptr<LED> mLED;
};


#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
