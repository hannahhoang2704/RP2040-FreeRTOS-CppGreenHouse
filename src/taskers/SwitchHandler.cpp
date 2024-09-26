#include <hardware/timer.h>
#include <memory>
#include "SwitchHandler.h"
#include "LED.h"

QueueHandle_t SwitchHandler::mIRQ_eventQueue = xQueueCreate(10, sizeof(button_irq_event_data));;
uint SwitchHandler::lostEvents = 0;

void SwitchHandler::irq_handler(uint gpio, uint32_t event_mask) {
    button_irq_event_data eventData{
            .gpio = gpio == ROT_A ? gpio_get(ROT_B) ? ROT_A : ROT_B : gpio,
            .eventMask = static_cast<gpio_irq_level>(event_mask),
            .timeStamp = time_us_64()
    };
    BaseType_t xHigherPriorityWoken = pdFALSE;
    if (xQueueSendToBackFromISR(mIRQ_eventQueue,
                                static_cast<void *>(&eventData),
                                &xHigherPriorityWoken) == errQUEUE_FULL) {
        ++lostEvents;
    }
    // some check for queue integrity -- maybe semaphore
    portYIELD_FROM_ISR(xHigherPriorityWoken);
}

SwitchHandler::SwitchHandler(std::shared_ptr<PicoOsUart> uart) : mUart(uart) {
    xTaskCreate(task_event_handler,
                "SW_HANDLER",
                512,
                (void *) this,
                tskIDLE_PRIORITY + 2,
                &mTaskHandle);

}

void SwitchHandler::event_handler() {
    // leds for testing -- at least
    LED leds[3]{LED1,LED2,LED3};
    uint8_t led_i = 2;

    while (true) {
        while (xQueueReceive(mIRQ_eventQueue,
                             static_cast<void *>(&mEventData),
                             portMAX_DELAY) == pdTRUE) {
            mEvent = UNKNOWN;
            switch (mEventData.gpio) {
                case SW_2:
                    // confirm -- at least for MQTT credential input
                    if (mEventData.timeStamp - mPrevEventTime[SW_2_PRESS] > mPressDebounce_us) {
                        mEvent = SW_2_PRESS;
                        mPrevEventTime[SW_2_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case SW_1:
                    // insert
                    if (mEventData.timeStamp - mPrevEventTime[SW_1_PRESS] > mPressDebounce_us) {
                        mEvent = SW_1_PRESS;
                        mPrevEventTime[SW_1_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case SW_0:
                    // change screen
                    if (mEventData.timeStamp - mPrevEventTime[SW_2_PRESS] > mPressDebounce_us) {
                        mEvent = SW_0_PRESS;
                        mPrevEventTime[SW_0_PRESS] = mEventData.timeStamp;
                    }
                    break;
                case ROT_A:
                case ROT_B:
                    if (mEventData.eventMask == GPIO_IRQ_EDGE_FALL) {
                        mEvent = mEventData.gpio == ROT_A ? ROT_COUNTER_CLOCKWISE : ROT_CLOCKWISE;
                    } else if (mEventData.eventMask == GPIO_IRQ_EDGE_RISE) {
                        mEvent = mEventData.gpio == ROT_A ? ROT_CLOCKWISE : ROT_COUNTER_CLOCKWISE;
                    }
                    break;
                case ROT_SW:
                    // backspace
                    if (mEventData.timeStamp - mPrevEventTime[SW_2_PRESS] > mPressDebounce_us) {
                        mEvent = ROT_PRESS;
                        mPrevEventTime[ROT_PRESS] = mEventData.timeStamp;
                    }
                    break;
                default:
                    // log ERROR ...
                    ;
            }

            switch (mEvent) {
                case SW_2_PRESS:
                    if (led_i < 2) ++led_i;
                    mUart->send("SW_2\n");
                    break;
                case SW_1_PRESS:
                    leds[led_i].toggle();
                    mUart->send("SW_1\n");
                    break;
                case SW_0_PRESS:
                    if (led_i > 0) --led_i;
                    mUart->send("SW_0\n");
                    break;
                case ROT_PRESS:
                    mUart->send("SW_ROT");
                    break;
                case ROT_CLOCKWISE:
                    leds[led_i].adjust(1);
                    mUart->send("CW\n");
                    break;
                case ROT_COUNTER_CLOCKWISE:
                    leds[led_i].adjust(-1);
                    mUart->send("CCW\n");
                    break;
                case UNKNOWN:
                default:
                    // wut
                    mUart->send("Unknown\n");
                    ;
            }
        }
        if (lostEvents) {
            // log lostEvents ERROR msg
            lostEvents = 0;
        }
    }
}
