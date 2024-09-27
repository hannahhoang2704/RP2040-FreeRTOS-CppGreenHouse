#include <utility>
#include <iostream>

#include "Logger.h"
#include "Display.h"

using namespace std;

Display::Display(shared_ptr<PicoI2C> i2c_sp) :
        mSSD1306(i2c_sp)
{
    if (xTaskCreate(task_display,
                    "DISPLAY",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created DISPLAY task.\n");
    } else {
        Logger::log("Failed to create DISPLAY task.\n");
    }
}

void Display::display() {
    Logger::log("Initiated DISPLAY task.\n");
    mSSD1306.init();
    bool color = true;
    while (true) {
        mSSD1306.fill(color);
        mSSD1306.text("Boot!", 46, 28, !color);
        mSSD1306.show();
        color = !color;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
