#ifndef RP2040_FREERTOS_IRQ_LED_H
#define RP2040_FREERTOS_IRQ_LED_H


#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <string>

#define DIVIDER 125
#define WRAP    99

#define PWM_ON true
#define INIT_LEVEL  2

#define LED1 22
#define LED2 21
#define LED3 20


class LED {
public:
    LED(uint pin, bool PWM_on = PWM_ON);
    void put(bool state);
    void toggle();
    std::string get_name() const;

private:
    uint mPin;
    bool mPWMon;
    uint mSlice;
    uint mChannel;
    const std::string mName;
    uint16_t mLevel{INIT_LEVEL};
    bool mOn{false};
};


#endif //RP2040_FREERTOS_IRQ_LED_H
