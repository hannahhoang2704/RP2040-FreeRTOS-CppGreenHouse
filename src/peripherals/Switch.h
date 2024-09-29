#ifndef RP2040_FREERTOS_IRQ_SWITCH_H
#define RP2040_FREERTOS_IRQ_SWITCH_H

#include <cstdio>
#include <string>
#include <hardware/gpio.h>

#include "FreeRTOS.h"
#include "queue.h"

namespace SW {
    class Button {
    public:
        explicit Button(uint pin, gpio_irq_callback_t irqCallback);
        void set_irq(bool state) const;

        const uint mPin;
    private:
        const gpio_irq_callback_t mIRQCallback;
        uint32_t mEventMask;
    };

    ///

    class Rotor {
    public:
        explicit Rotor(uint pin_irq, uint pin_clock, gpio_irq_callback_t irqCallback);
        void set_irq(bool state) const;

        const uint mPinA;
        const uint mPinB;
    private:
        const gpio_irq_callback_t mIRQCallback;
        uint32_t mEventMask;
    };
}


#endif //RP2040_FREERTOS_IRQ_SWITCH_H
