//
// Created by Hanh Hoang on 25.9.2024.
//
#include "Pressure_sensor.h"
#include "Logger.h"

PressureSensor::PressureSensor(std::shared_ptr<PicoI2C> i2c_sp, uint8_t deviceAddress):
    mI2C(std::move(i2c_sp)), mDevAddr(deviceAddress) {}

int PressureSensor::read_pressure() {
    mI2C->write(mDevAddr, trigger_measurement_cmd, 1);
    vTaskDelay(10);
    mI2C->read(mDevAddr, mBuffer, buffer_len);
    vTaskDelay(100);
    mPressure_value = ((mBuffer[0] << 8) | mBuffer[1]) / differential_pressure * correction_factor;
    mPressure_value = (mPressure_value <= MIN_PRESSURE_VALUE) ? 0 : (mPressure_value >= MAX_PRESSURE_VALUE) ? 0 : mPressure_value;
    Logger::log("Pressure value: %d\n", mPressure_value);
    return mPressure_value;
}

int PressureSensor::get_pressure_value() const {
    //probably need mutex
    return mPressure_value;
}
