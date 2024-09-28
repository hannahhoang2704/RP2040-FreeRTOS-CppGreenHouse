//
// Created by Keijo Länsikunnas on 10.9.2024.
//

#include "Blinker.h"

using namespace std;

Blinker::Blinker(uint led_pin) :
        mLED(led_pin),
        mName("BLINKER_D" + to_string(abs(static_cast<int>(led_pin) - 23))) {
    xTaskCreate(Blinker::task_blinker,
                mName.c_str(),
                512,
                (void *) this,
                tskIDLE_PRIORITY + 1,
                &mTaskHandle);
}

void Blinker::off() {
    xTaskNotify(mTaskHandle, 0xFFFFFFFF, eSetValueWithOverwrite);
}

void Blinker::on(uint delay) {
    xTaskNotify(mTaskHandle, delay, eSetValueWithOverwrite);
}

void Blinker::blinker() {
    bool on = false;
    while (true) {
        uint32_t cmd = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(mDelay));
        switch (cmd) {
            case 0:
                // timed out --> do nothing
                break;
            case 0xFFFFFFFF:
                on = false;
                mLED.put(false);
                break;
            default:
                mDelay = cmd;
                on = true;
                break;
        }
        if (on) {
            mLED.toggle();
        }
    }
}

TaskHandle_t Blinker::get_task_handle() const {
    return mTaskHandle;
}
