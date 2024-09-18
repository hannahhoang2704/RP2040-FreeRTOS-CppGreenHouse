//
// Created by Keijo Länsikunnas on 10.9.2024.
// Cleaned by Roni
//

#ifndef RP2040_FREERTOS_IRQ_BLINKER_H
#define RP2040_FREERTOS_IRQ_BLINKER_H

#include <string>
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "LED.h"

#define INIT_DELAY 300

class Blinker {
public:
    Blinker(uint led_pin);
    void off();
    void on(uint delay);
    TaskHandle_t get_task_handle() const;
private:
    void blinker();
    static void task_blinker(void *params) {
        auto object_ptr = static_cast<Blinker *>(params);
        object_ptr->blinker();
    };
    const std::string mName;
    LED mLED;
    uint mDelay{INIT_DELAY};
    TaskHandle_t mTaskHandle;
};

#endif //RP2040_FREERTOS_IRQ_BLINKER_H
