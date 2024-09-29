#include "Switch.h"

using namespace std;

namespace SW {

    Button::Button(uint pin, gpio_irq_callback_t irqCallback) :
            mPin(pin),
            mEventMask(GPIO_IRQ_EDGE_FALL),
            mIRQCallback(irqCallback) {
        gpio_pull_up(mPin);
        gpio_init(mPin);
        gpio_set_irq_enabled_with_callback(mPin,
                                           mEventMask,
                                           false,
                                           mIRQCallback);
    }

    void Button::set_irq(bool state) const {
        gpio_set_irq_enabled_with_callback(mPin, mEventMask, state, mIRQCallback);
    }

    ///

    Rotor::Rotor(uint pin_irq, uint pin_clk, gpio_irq_callback_t irqCallback) :
            mPinA(pin_irq), mPinB(pin_clk), mIRQCallback(irqCallback),
            mEventMask(GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL) {
        gpio_set_irq_enabled_with_callback(mPinA,
                                           mEventMask,
                                           false,
                                           mIRQCallback);
        gpio_init(mPinB);
    }

    void Rotor::set_irq(bool state) const {
        gpio_set_irq_enabled_with_callback(mPinA, mEventMask, state, mIRQCallback);
    }
}
