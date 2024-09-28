#include "LED.h"


using namespace std;

LED::LED(uint pin, bool PWM_on) :
        mPin(pin),
        mPWMon(PWM_on),
        mSlice(pwm_gpio_to_slice_num(pin)),
        mChannel(pwm_gpio_to_channel(pin)),
        mName("D" + to_string(static_cast<int>(mPin) - 23)) {
    if (PWM_on) {
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv_int(&config, DIVIDER);
        pwm_config_set_wrap(&config, WRAP);

        pwm_set_enabled(mSlice, false);
        pwm_init(mSlice, &config, false);
        pwm_set_chan_level(mSlice, mChannel, mOn ? mLevel : 0);
        gpio_set_function(mPin, GPIO_FUNC_PWM);

        pwm_set_enabled(pwm_gpio_to_slice_num(mPin), true);
    } else {
        gpio_init(mPin);
        gpio_set_dir(mPin, GPIO_OUT);
    }
}

void LED::put(bool state) {
    mOn = state;
    if (mPWMon) {
        pwm_set_chan_level(mSlice,
                           mChannel,
                           mOn ? mLevel : 0);
    } else {
        gpio_put(mPin, state);
    }
}

void LED::toggle() {
    put(!mOn);
}

string LED::get_name() const {
    return mName;
}

void LED::set(uint level) {
    if (level <= WRAP) mLevel = level;
    else mLevel = WRAP;
    pwm_set_chan_level(mSlice,
                       mChannel,
                       mOn ? mLevel : 0);
}

void LED::adjust(int increment) {
    if (mLevel + increment < WRAP) {
        mLevel += increment;
    } else {
        increment < 0 ? mLevel = 0 : mLevel = WRAP;
    }
    set(mLevel);
}
