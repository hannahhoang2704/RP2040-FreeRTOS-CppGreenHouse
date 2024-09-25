#ifndef FREERTOS_GREENHOUSE_SENSOR_H
#define FREERTOS_GREENHOUSE_SENSOR_H

#include <cstdint>
#include "modbus/ModbusRegister.h"

namespace Sensor {

    const struct {
        int servAddr = 240;
        int regAddrCO2 = 0x0000;
        int regAddrTemp = 0x0004;
    } GMP252_s;

    const struct {
        int servAddr = 241;
        int regAddrRelHum = 256;
        int regAddrTemp = 257;
    } HMP60_s;

    class GMP252 {
    public:
        GMP252(const std::shared_ptr<ModbusClient>& modbusClient);
        float update_co2();
        float update_temperature();
        float co2() const;
        float temperature() const;

    private:
        ModbusRegister mCO2_low;
        ModbusRegister mCO2_high;
        ModbusRegister mTemp_low;
        ModbusRegister mTemp_high;

        union {
            uint32_t u32;
            float f = 0;
        } mCO2;

        union {
            uint32_t u32;
            float f = 0;
        } mTemp;

    };

    class HMP60 {
    public:
        HMP60(const std::shared_ptr<ModbusClient>& modbusClient);
        float update_relHum();
        float update_temperature();
        float relHum() const;
        float temperature() const;

    private:
        ModbusRegister mRelHum_mr;
        ModbusRegister mTemp_mr;

        float mRelHum;
        float mTemp;
    };

    // SDP6X0 missing

} // Sensor

#endif //FREERTOS_GREENHOUSE_SENSOR_H
