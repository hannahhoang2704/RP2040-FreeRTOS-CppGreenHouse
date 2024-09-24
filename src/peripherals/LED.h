#ifndef RP2040_FREERTOS_IRQ_LED_H
#define RP2040_FREERTOS_IRQ_LED_H


#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <string>

#define LED1 22
#define LED2 21
#define LED3 20


class LED {
public:
    LED(uint pin, bool PWM_on = true);
    void put(bool state);
    void toggle();
    std::string get_name() const;

private:
    static const uint16_t DIVIDER{125};
    static const uint16_t WRAP{99};
    static const uint16_t INIT_LEVEL{2};

    uint mPin;
    bool mPWMon;
    uint mSlice;
    uint mChannel;
    const std::string mName;
    uint16_t mLevel{INIT_LEVEL};
    bool mOn{false};
};


#endif //RP2040_FREERTOS_IRQ_LED_H
