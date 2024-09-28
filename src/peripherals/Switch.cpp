#include "Switch.h"

using namespace std;

namespace SW {

    Button::Button(uint pin) :
            mPin(pin),
            mName("SW" + to_string(abs(static_cast<int>(mPin) - 9))) {
        gpio_pull_up(mPin);
        gpio_init(mPin);

        gpio_set_irq_enabled_with_callback(mPin,
                                           GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                           true,
                                           SwitchHandler::irq_handler);
    }

    Rotor::Rotor(uint pin_irq, uint pin_clk) :
            mPinIRQ(pin_irq), mPinCLK(pin_clk) {
        gpio_set_irq_enabled_with_callback(mPinIRQ,
                                           GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                           true,
                                           SwitchHandler::irq_handler);
        gpio_init(mPinCLK);
    }
}
