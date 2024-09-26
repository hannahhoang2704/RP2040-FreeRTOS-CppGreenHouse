#ifndef FREERTOS_GREENHOUSE_SWITCHHANDLER_H
#define FREERTOS_GREENHOUSE_SWITCHHANDLER_H


#include <cstdio>
#include <hardware/gpio.h>
#include <map>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "uart/PicoOsUart.h"
#include <memory>

class SwitchHandler {
public:
    SwitchHandler(std::shared_ptr<PicoOsUart> uart);

    static void irq_handler(uint gpio, uint32_t event_mask);

    struct button_irq_event_data {
        uint gpio;
        gpio_irq_level eventMask;
        uint64_t timeStamp;
    };

    static const uint SW_2 = 7;
    static const uint SW_1 = 8;
    static const uint SW_0 = 9;
    static const uint ROT_B = 11;
    static const uint ROT_SW = 12;
    static const uint ROT_A = 10;

    static uint lostEvents;

private:
    void event_handler();
    static void task_event_handler(void *params) {
        auto object_ptr = static_cast<SwitchHandler *>(params);
        object_ptr->event_handler();
    }

    TaskHandle_t mTaskHandle{nullptr};
    static QueueHandle_t mIRQ_eventQueue;
    button_irq_event_data mEventData{
            .gpio = SW_2,
            .eventMask = GPIO_IRQ_LEVEL_HIGH,
            .timeStamp = 0
    };

    enum swEvent {
        UNKNOWN = -1,
        SW_2_PRESS,
        SW_2_RELEASE,
        SW_1_PRESS,
        SW_1_RELEASE,
        SW_0_PRESS,
        SW_0_RELEASE,
        ROT_CLOCKWISE,
        ROT_COUNTER_CLOCKWISE,
        ROT_PRESS,
        ROT_RELEASE
    } mEvent;

    const uint64_t mPressDebounce_us{100000};

    std::map<swEvent, uint64_t> mPrevEventTime;

    std::shared_ptr<PicoOsUart> mUart;
};


#endif //FREERTOS_GREENHOUSE_SWITCHHANDLER_H
