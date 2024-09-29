#include <hardware/gpio.h>
#include "Actuator.h"

namespace Actuator {

    Fan::Fan(const std::shared_ptr<ModbusClient>& modbusClient, int server, int regFanPower, int regFanCounter) :
            mFanPower(modbusClient, server, regFanPower),
            mFanCounter(modbusClient, server, regFanCounter)
    {}

    void Fan::set_power(int16_t permille) {
        if (permille > MAX_POWER) permille = MAX_POWER;
        else if (permille < MIN_POWER) permille = MIN_POWER;
        mFanPower.write(permille);
        if (permille != OFF)
            for (uint8_t attempt = 0; !running() && attempt < 3; ++attempt)
                mFanPower.write(permille);
        mSetPower = permille;
    }

    bool Fan::running() {
        mFanCounter.read();
        return mFanCounter.read();
    }

    int16_t Fan::get_power() const {
        return mSetPower;
    }

    int16_t Fan::read_power() {
        return static_cast<int16_t>(mFanPower.read());
    }

    ///

    CO2_Emitter::CO2_Emitter(int gpio) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_OUT);
    }

    void CO2_Emitter::put_state(bool state) {
        gpio_put(mGpio, state);
    }

    bool CO2_Emitter::get_state() const {
        return gpio_get(mGpio);
    }
} // Actuator