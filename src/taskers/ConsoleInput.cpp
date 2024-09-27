#include <iostream>
#include "ConsoleInput.h"

/// this is from keijo's example
// just converted

using namespace std;

ConsoleInput::ConsoleInput(shared_ptr<PicoOsUart> uart_sp, uint led_pin) :
        mBlinker(led_pin), mUART(std::move(uart_sp))
{
    if (xTaskCreate(task_console_input,
                    "CONSOLE_INPUT",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 1,
                    &mTaskHandle) == pdPASS) {
        cout << "Created CONSOLE_INPUT task" << endl;
    } else {
        cout << "Failed to create CONSOLE_INPUT task" << endl;
    }
}

void ConsoleInput::console_input() {
    while (true) {
        if(int count = mUART->read(mBuffer, CONSOLE_BUFFER_SIZE - 1, 30); count > 0) {
            mUART->write(mBuffer, count);
            mBuffer[count] = '\0';
            mLine += reinterpret_cast<const char *>(mBuffer);
            if(mLine.find_first_of("\n\r") != std::string::npos){
                mUART->send("\n");
                istringstream input(mLine);
                if(input.str() == "delay") {
                    uint32_t i = 0;
                    input >> i;
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
