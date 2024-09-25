//
// Created by Hanh Hoang on 25.9.2024.
//

#ifndef FREERTOS_GREENHOUSE_PRESSURE_SENSOR_H
#define FREERTOS_GREENHOUSE_PRESSURE_SENSOR_H

#include <memory>
#include "i2c/PicoI2C.h"
#include "FreeRTOS.h"
#define SDP610_ADDR 0x40
#define MAX_PRESSURE_VALUE 125
#define MIN_PRESSURE_VALUE 0

class PressureSensor {
public:
    PressureSensor(std::shared_ptr<PicoI2C> i2c_sp, uint8_t deviceAddress=SDP610_ADDR);
    int read_pressure();
    int get_pressure_value() const;
private:
    static const uint8_t correction_factor = 0.95;
    static const uint8_t differential_pressure = 240;
    static const int buffer_len = 2;
    const uint8_t trigger_measurement_cmd[1] = {0xF1};
    std::shared_ptr<PicoI2C> mI2C;
    uint8_t mDevAddr;
    uint8_t mBuffer[buffer_len];
    int mPressure_value;
};
#endif //FREERTOS_GREENHOUSE_PRESSURE_SENSOR_H
