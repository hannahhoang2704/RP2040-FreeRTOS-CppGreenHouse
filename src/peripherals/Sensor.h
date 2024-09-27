#ifndef FREERTOS_GREENHOUSE_SENSOR_H
#define FREERTOS_GREENHOUSE_SENSOR_H

#include <cstdint>
#include "modbus/ModbusRegister.h"

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

    union u32_f_u {
        uint32_t u32;
        float f = 0;
    };

    union u16_u32_f_u {
        uint16_t u16;
        uint32_t u32;
        float f = 0;
    };

    class CO2 {
    public:
        CO2(const std::shared_ptr<ModbusClient>& modbusClient);
        float update();
        float f() const;
        uint32_t u32() const;

    private:
        ModbusRegister mGMP252_low;
        ModbusRegister mGMP252_high;

        u32_f_u mCO2;
    };

    class Humidity {
    public:
        Humidity(const std::shared_ptr<ModbusClient>& modbusClient);
        float update();
        float f() const;
        uint32_t u32() const;

    private:
        ModbusRegister mHMP60;
        float mRelHum;
        u16_u32_f_u mRH;
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
    };

    // SDP610 missing

} // Sensor

#endif //FREERTOS_GREENHOUSE_SENSOR_H
