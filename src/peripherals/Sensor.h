#ifndef FREERTOS_GREENHOUSE_SENSOR_H
#define FREERTOS_GREENHOUSE_SENSOR_H

#include <cstdint>
#include "modbus/ModbusRegister.h"
#include "i2c/PicoI2C.h"

namespace Sensor {

    static const struct {
        int servAddr = 240;
        int regAddrCO2 = 0x0000;
        int regAddrTemp = 0x0004;
    } GMP252_s;

    static const struct {
        int servAddr = 241;
        int regAddrRelHum = 256;
        int regAddrTemp = 257;
    } HMP60_s;

    static const struct {
        int regAddrPressure = 0x40;
        int maxPressure = 125;
        int minPressure = 0;
    } SDP610_s;

    union u32_f_u {
        uint32_t u32;
        float f = 0;
    };

    class CO2 {
    public:
        CO2(const std::shared_ptr<ModbusClient>& modbusClient);
        float update();
        float value() const;

    private:
        ModbusRegister mGMP252_low;
        ModbusRegister mGMP252_high;

        u32_f_u mCO2;
    };

    class Humidity {
    public:
        Humidity(const std::shared_ptr<ModbusClient>& modbusClient);
        float update();
        float value() const;

    private:
        ModbusRegister mHMP60;
        float mRelHum;
    };

    class Temperature {
    public:
        Temperature(const std::shared_ptr<ModbusClient>& modbusClient);
        float update_GMP252();
        float update_HMP60();
        float update_all();

        float value_GMP252() const;
        float value_HMP60() const;
        float value_average() const;
    private:
        ModbusRegister mGMP252_low;
        ModbusRegister mGMP252_high;
        ModbusRegister mHMP60;

        u32_f_u mTempGMP252;
        uint16_t mTempHMP60{0};
        float mTempAvg{0};
    };

    class PressureSensor {
    public:
        PressureSensor(const std::shared_ptr<PicoI2C> &i2c_sp);
        int update_SDP610();
        int value_SDP610() const;
    private:
        const float correction_factor = 0.95;
        const uint8_t differential_pressure = 240;
        static const int buffer_len = 2;
        const uint8_t trigger_measurement_cmd[1] = {0xF1};
        std::shared_ptr<PicoI2C> mI2C;
        uint8_t mDevAddr;
        uint8_t mBuffer[buffer_len];
        int mPressure_value;
    };

} // Sensor

#endif //FREERTOS_GREENHOUSE_SENSOR_H
