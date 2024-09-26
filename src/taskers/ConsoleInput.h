#ifndef RP2040_FREERTOS_IRQ_CONSOLEINPUT_H
#define RP2040_FREERTOS_IRQ_CONSOLEINPUT_H

// wrote all this before I noticed he wasn't even using these

#include <memory>
#include <sstream>
#include <utility>
#include "uart/PicoOsUart.h"
#include "Blinker.h"

#define CONSOLE_BUFFER_SIZE 64

class ConsoleInput {
public:
    ConsoleInput(std::shared_ptr<PicoOsUart> uart_sp, uint led_pin);
private:
    void console_input();
    static void task_console_input(void * params) {
        auto object_ptr = static_cast<ConsoleInput *>(params);
        object_ptr->console_input();
    }

    std::shared_ptr<PicoOsUart> mUART;
    uint8_t mBuffer[CONSOLE_BUFFER_SIZE];
    std::string mLine;
    Blinker mBlinker;

    TaskHandle_t mTaskHandle;
};

#endif //RP2040_FREERTOS_IRQ_CONSOLEINPUT_H
