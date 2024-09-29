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
    Greenhouse(const std::shared_ptr<ModbusClient> &modbus_client, const std::shared_ptr<PicoI2C> &pressure_sensor_I2C);

private:
    void automate_greenhouse();

    static void task_automate_greenhouse(void *params);

    TaskHandle_t mTaskHandle;
    Sensor::CO2 mCO2;
    Sensor::Humidity mHumidity;
    Sensor::Temperature mTemperature;
    Sensor::PressureSensor mPressure;
    ModbusRegister mMIO12_V;
};

#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
