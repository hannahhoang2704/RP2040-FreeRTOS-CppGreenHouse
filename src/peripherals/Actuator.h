#ifndef FREERTOS_GREENHOUSE_ACTUATOR_H
#define FREERTOS_GREENHOUSE_ACTUATOR_H

#include <memory>
#include "modbus/ModbusRegister.h"

namespace Actuator {

    const struct {
        int servAddr = 1;
        int regAddrInput = 0x000;
        int regAddrOutput = 0x004;
    } MIO12V;

    class Fan {
    public:
        Fan(const std::shared_ptr<ModbusClient>& modbusClient,
            int server = MIO12V.servAddr,
            int regFanPower = MIO12V.regAddrInput,
            int regFanCounter = MIO12V.regAddrOutput);

        void set_power(int16_t permille);
        bool running();
        int16_t get_power() const;
        int16_t read_power();
    private:
        const int16_t MAX_POWER{1000};
        const int16_t MIN_POWER{0};
        const int16_t OFF{0};
        int16_t mSetPower;

        ModbusRegister mFanPower;
        ModbusRegister mFanCounter;
    };

    ///

    const struct {
        int gpio = 27;
    } SodaStream;

    class CO2_Emitter {
    public:
        CO2_Emitter(int gpio = SodaStream.gpio);
        void put_state(bool state);
        bool get_state() const;
    private:
        uint mGpio;
    };

} // Actuator

#endif //FREERTOS_GREENHOUSE_ACTUATOR_H
