#include <hardware/timer.h>
#include <memory>
#include "SwitchHandler.h"
#include "LED.h"
#include "Switch.h"
#include "Logger.h"

QueueHandle_t SwitchHandler::mIRQ_eventQueue = xQueueCreate(10, sizeof(button_irq_event_data));;
uint32_t SwitchHandler::mLostEvents = 0;

void SwitchHandler::irq_handler(uint gpio, uint32_t event_mask) {
    BaseType_t xHigherPriorityWoken = pdFALSE;
    button_irq_event_data eventData{
            .gpio = gpio == ROT_A ? gpio_get(ROT_B) ? ROT_A : ROT_B : gpio,
            .eventMask = static_cast<gpio_irq_level>(event_mask),
            .timeStamp = time_us_64()
    };
    if (xQueueSendToBackFromISR(mIRQ_eventQueue,
                                static_cast<void *>(&eventData),
                                &xHigherPriorityWoken) == errQUEUE_FULL) {
        ++mLostEvents;
    }
    portYIELD_FROM_ISR(xHigherPriorityWoken);
}

SwitchHandler::SwitchHandler() {
    if (xTaskCreate(task_event_handler,
                    "SW_HANDLER",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created SW_HANDLER task\n");
    } else {
        Logger::log("Failed to create SW_HANDLER task\n");
    }

}

void SwitchHandler::task_event_handler(void *params) {
    auto object_ptr = static_cast<SwitchHandler *>(params);
    object_ptr->event_handler();
}

void SwitchHandler::event_handler() {
    Logger::log("Initiated SwitchHandler\n");
    new SW::Button(SwitchHandler::SW_2);
    new SW::Button(SwitchHandler::SW_1);
    new SW::Button(SwitchHandler::SW_0);
    new SW::Button(SwitchHandler::ROT_SW);
    new SW::Rotor();

    while (true) {
        while (xQueueReceive(mIRQ_eventQueue,
                             static_cast<void *>(&mEventData),
                             portMAX_DELAY) == pdTRUE) {
            switch (mEventData.gpio) {
                case SW_2:
                    // confirm -- at least for MQTT credential input
                    if (mEventData.timeStamp - mPrevEventTime[SW_2_PRESS] > mPressDebounce_us) {
                        Logger::log("SWH: SW_2\n");
                        mPrevEventTime[SW_2_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case SW_1:
                    // insert
                    if (mEventData.timeStamp - mPrevEventTime[SW_1_PRESS] > mPressDebounce_us) {
                        mEvent = SW_1_PRESS;
                        Logger::log("SWH: SW_1\n");
                        mPrevEventTime[SW_1_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case SW_0:
                    // change screen
                    if (mEventData.timeStamp - mPrevEventTime[SW_2_PRESS] > mPressDebounce_us) {
                        mEvent = SW_0_PRESS;
                        Logger::log("SWH: SW_0\n");
                        mPrevEventTime[SW_0_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case ROT_SW:
                    // backspace
                    if (mEventData.timeStamp - mPrevEventTime[SW_2_PRESS] > mPressDebounce_us) {
                        mEvent = ROT_PRESS;
                        Logger::log("SWH: SW_ROT\n");
                        mPrevEventTime[ROT_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case ROT_A:
                case ROT_B:
                    if (mEventData.eventMask == GPIO_IRQ_EDGE_FALL) {
                        mEvent = mEventData.gpio == ROT_A ? ROT_COUNTER_CLOCKWISE : ROT_CLOCKWISE;
                    } else if (mEventData.eventMask == GPIO_IRQ_EDGE_RISE) {
                        mEvent = mEventData.gpio == ROT_A ? ROT_CLOCKWISE : ROT_COUNTER_CLOCKWISE;
                    }
                    if (mEvent == ROT_CLOCKWISE) {
                        Logger::log("SWH: ROT ++\n");
                    } else if (mEvent == ROT_COUNTER_CLOCKWISE) {
                        Logger::log("SWH: ROT --\n");
                    }
                    break;
                default:
                    Logger::log("ERROR: SwitchHandler: gpio %u\n", mEventData.gpio);
            }
        }
        if (mLostEvents) {
            Logger::log("ERROR: SwitchHandler: %u sw events lost\n", mLostEvents);
            mLostEvents = 0;
        }
    }
}
