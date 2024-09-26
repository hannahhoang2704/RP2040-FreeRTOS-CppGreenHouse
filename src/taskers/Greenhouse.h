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

    const int HMP60_ADDRESS{241};
    const int GMP252_ADDRESS{240};
private:
    void automate_greenhouse();
    static void task_automate_greenhouse(void * params) {
        auto object_ptr = static_cast<Greenhouse *>(params);
        object_ptr->automate_greenhouse();
    }

    float update_CO2();
    float update_RH();
    float update_T();

    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mCLI_UART; // only for prints -- switch to logger at some point
    ModbusRegister mHMP60_rh;
    ModbusRegister mHMP60_t;
    ModbusRegister mGMP252_co2_low;
    ModbusRegister mGMP252_co2_high;
    ModbusRegister mGMP252_t_low;
    ModbusRegister mGMP252_t_high;
    ModbusRegister mMIO12_V;
    std::shared_ptr<LED> mLED;

    union u32_to_f_union {
        uint32_t u32;
        float f;
    } mCO2{0}, mTempGMP252{0};

    union u16_to_f_union {
        uint16_t u16;
        float f;
    } mTempHMP60{0}, mRelHum{0};
};


#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
