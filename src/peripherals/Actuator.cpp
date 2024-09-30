#include <hardware/gpio.h>
#include "Actuator.h"

namespace Actuator {

    Fan::Fan(const std::shared_ptr<ModbusClient>& modbusClient, int server, int regFanPower, int regFanCounter) :
            mFanPower(modbusClient, server, regFanPower),
            mFanCounter(modbusClient, server, regFanCounter)
    {}

    void Fan::set_power(int16_t permilles) {
        if (permilles > MAX_POWER) permilles = MAX_POWER;
        else if (permilles < MIN_POWER) permilles = MIN_POWER;
        // the test-fan needs a bit of a kick from complete stillness
        if (mSetPower != OFF) mFanPower.write(MAX_POWER);
        mSetPower = permilles;
        mFanPower.write(mSetPower);
    }

    bool Fan::running() {
        mFanCounter.read();
        return mFanCounter.read();
    }

    int16_t Fan::get_power() const {
        return mSetPower;
    }

    int16_t Fan::read_power() {
        return (mSetPower = static_cast<int16_t>(mFanPower.read()));
    }

    ///

    CO2_Emitter::CO2_Emitter(int gpio) : mPin(gpio) {
        gpio_init(mPin);
        gpio_set_dir(mPin, GPIO_OUT);
    }

    void CO2_Emitter::put_state(bool state) const {
        gpio_put(mPin, state);
    }

    bool CO2_Emitter::get_state() const {
        return gpio_get(mPin);
    }
} // Actuator