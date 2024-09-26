#include <iostream>
#include "ConsoleInput.h"

/// this is from keijo's example
// just converted

using namespace std;

ConsoleInput::ConsoleInput(shared_ptr<PicoOsUart> uart_sp, uint led_pin) :
        mBlinker(led_pin), mCLI_UART(std::move(uart_sp))
{
    if (xTaskCreate(task_console_input,
                    "CONSOLE_INPUT",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 1,
                    &mTaskHandle) == pdPASS) {
        mCLI_UART->send("Created CONSOLE_INPUT task\n");
    } else {
        mCLI_UART->send("Failed to created CONSOLE_INPUT task\n");
    }
}

void ConsoleInput::console_input() {
    mCLI_UART->send("Initiated CONSOLE_INPUT task\n");
    while (true) {
        if(int count = mCLI_UART->read(mBuffer, CONSOLE_BUFFER_SIZE - 1, 30); count > 0) {
            mCLI_UART->write(mBuffer, count);
            mBuffer[count] = '\0';
            mLine += reinterpret_cast<const char *>(mBuffer);
            if(mLine.find_first_of("\n\r") != std::string::npos){
                mCLI_UART->send("\n");
                istringstream input(mLine);
                if(input.str() == "delay") {
                    uint32_t i = 1000;
                    //input >> i;
                    mBlinker.on(i);
                }
                else if (input.str() == "off") {
                    mBlinker.off();
                }
                mLine.clear();
            }
        }
    }
}
