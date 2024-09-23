#ifndef RP2040_FREERTOS_IRQ_GREENHOUSE_H
#define RP2040_FREERTOS_IRQ_GREENHOUSE_H


#include <memory>
#include <utility>
#include <iostream>

#include "uart/PicoOsUart.h"
#include "LED.h"
#include "modbus/ModbusRegister.h"

class Greenhouse {
public:
    Greenhouse(std::shared_ptr<PicoOsUart> uart_sp, std::shared_ptr<ModbusClient> modbus_client, uint lep_pin);
private:
    void automate_greenhouse();
    static void task_automate_greenhouse(void * params) {
        auto object_ptr = static_cast<Greenhouse *>(params);
        object_ptr->automate_greenhouse();
    }

    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mCLI_UART; // only for prints -- switch to logger at some point
    ModbusRegister mRH_RH;
    ModbusRegister mT_RH;
    ModbusRegister mFan;
    std::shared_ptr<LED> mLED;
};


#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
