#include "Display.h"

#include <utility>
#include <iostream>

using namespace std;

Display::Display(shared_ptr<PicoI2C> i2c_sp) :
        mDisplay(std::move(i2c_sp))
{
    if (xTaskCreate(task_display,
                    "DISPLAY",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        cout << "Created DISPLAY task" << endl;
    } else {
        cout << "Failed to create DISPLAY task" << endl;
    }
}

void Display::display() {
    cout << "Initiating DISPLAY task" << endl;
    mDisplay.fill(1);
    mDisplay.text("Boot!", 60, 28, 0);
    mDisplay.show();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
