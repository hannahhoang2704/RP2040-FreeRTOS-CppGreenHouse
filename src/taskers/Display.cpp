#include <utility>
#include <iostream>

#include "Logger.h"
#include "Display.h"

using namespace std;

Display::Display(shared_ptr<PicoI2C> i2c_sp) :
        mSSD1306(std::move(i2c_sp))
{
    if (xTaskCreate(task_display,
                    "DISPLAY",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        // log
        Logger::log("Created DISPLAY task.\n");
    } else {
        // log
        Logger::log("Failed to create DISPLAY task.\n");
    }
}

void Display::display() {
    // log
    mSSD1306.init();
    bool color = true;
    while (true) {
        mSSD1306.fill(color ? 1 : 0);
        mSSD1306.text("Boot!", 46, 28, color ? 0 : 1);
        mSSD1306.show();
        color = !color;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
