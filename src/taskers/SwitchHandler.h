#ifndef FREERTOS_GREENHOUSE_SWITCHHANDLER_H
#define FREERTOS_GREENHOUSE_SWITCHHANDLER_H


#include <cstdio>
#include <map>
#include <vector>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "Switch.h"

class SwitchHandler {
public:
    SwitchHandler();

    static void irq_handler(uint gpio, uint32_t event_mask);

    struct button_irq_event_data {
        uint gpio;
        gpio_irq_level eventMask;
        uint64_t timeStamp;
    };

    static const uint SW_2 = 7;
    static const uint SW_1 = 8;
    static const uint SW_0 = 9;
    static const uint SW_ROT = 12;
    static const uint ROT_B = 11;
    static const uint ROT_A = 10;

    static uint32_t mLostEvents;

    enum state {
        STATUS,
        RELOG,
        CONNECTING
    };

    enum network_phase {
        IP,
        USERNAME,
        PASSWORD
    };

private:
    void state_handler();
    static void task_state_handler(void *params);

    void set_sw_irq(bool state) const;
    void rot_event();
    void button_event();
    bool inc_pending_char();
    bool dec_pending_char();

    TaskHandle_t mTaskHandle{nullptr};
    static QueueHandle_t mIRQ_eventQueue;
    button_irq_event_data mEventData{
            .gpio = SW_2,
            .eventMask = GPIO_IRQ_LEVEL_HIGH,
            .timeStamp = 0
    };

    SW::Button mToggleState;
    SW::Button mInsert;
    SW::Button mNext;
    SW::Button mBackspace;
    SW::Rotor mRotor;

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
    } mEvent{UNKNOWN};

    const uint64_t mPressDebounce_us{400000};
    std::map<uint, uint64_t> mPrevEventTime;

    /// state data
    const char INIT_CHAR{'.'};
    const int16_t CO2_INCREMENT{1};

    state mState{CONNECTING};
    int16_t mCurrCO2Target{0};
    int16_t mPendingCO2Target{0};
    char mPendingChar{INIT_CHAR};
    network_phase mRelogPhase{IP};
    std::vector<std::string> mRelogStrings{"", "", ""};
    uint64_t mPrevBackspace{0};
    swEvent mPrevRotation{UNKNOWN};
};


#endif //FREERTOS_GREENHOUSE_SWITCHHANDLER_H
