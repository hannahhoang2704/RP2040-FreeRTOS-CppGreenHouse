#ifndef RP2040_FREERTOS_IRQ_SWITCH_H
#define RP2040_FREERTOS_IRQ_SWITCH_H

#include <cstdio>
#include <string>
#include <hardware/gpio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "SwitchHandler.h"

namespace SW {
    class Button {
    public:
        Button(uint pin);

        const std::string mName;
    private:
        uint mPin;
    };

    class Rotor {
    public:
        explicit Rotor(uint pin_irq = SwitchHandler::ROT_A, uint pin_clock = SwitchHandler::ROT_B);

    private:
        uint mPinIRQ;
        uint mPinCLK;
    };
}


#endif //RP2040_FREERTOS_IRQ_SWITCH_H
