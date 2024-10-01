#ifndef FREERTOS_GREENHOUSE_SWITCHHANDLER_H
#define FREERTOS_GREENHOUSE_SWITCHHANDLER_H


#include <cstdio>
#include <map>
#include <vector>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "Switch.h"
#include "RTOS_infrastructure.h"

class SwitchHandler {
public:
    SwitchHandler(RTOS_infrastructure RTOSi);

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

    /// sw event handling data
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

    static const uint64_t TASK_NOTIFICATION_RATE_LIMIT_US{30000}; // RTOS task notification doesn't like bombardment
    static const uint64_t BUTTON_DEBOUNCE{400000};
    std::map<uint, uint64_t> mPrevEventTimeMap;
    uint64_t mPrevBackspace{0};
    swEvent mPrevRotation{UNKNOWN};
    uint64_t mPrevDisplayNotificationTime{0};
    uint32_t mDisplayNote;

    /// state data
    static const int16_t CO2_MAX{1500};
    static const int16_t CO2_MIN{0};

    program_state mState{STATUS};
    int16_t mCO2TargetCurr{0};
    int16_t mCO2TargetPending{0};
    char mCharPending{INIT_CHAR};
    network_phase mRelogPhase{NEW_IP};
    std::vector<std::string> mRelogStrings{"", "", ""};

    /// RTOS infrastructure
    RTOS_infrastructure iRTOS;
};


#endif //FREERTOS_GREENHOUSE_SWITCHHANDLER_H
