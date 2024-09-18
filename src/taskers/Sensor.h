#ifndef RP2040_FREERTOS_IRQ_SENSOR_H
#define RP2040_FREERTOS_IRQ_SENSOR_H


#include <memory>
#include "uart/PicoOsUart.h"
#include "LED.h"
#include "modbus/ModbusRegister.h"

class Sensor {
public:
    Sensor(std::shared_ptr<PicoOsUart> uart_sp, std::shared_ptr<ModbusClient> modbus_client, uint lep_pin);
private:
    void sense();
    static void task_sense(void * params) {
        auto object_ptr = static_cast<Sensor *>(params);
        object_ptr->sense();
    }

    TaskHandle_t mTaskHandle;
    std::shared_ptr<PicoOsUart> mUART;
    ModbusRegister mRH_RH;
    ModbusRegister mT_RH;
    LED mLED;
};


#endif //RP2040_FREERTOS_IRQ_SENSOR_H
