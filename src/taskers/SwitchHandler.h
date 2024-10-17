#ifndef FREERTOS_GREENHOUSE_SWITCHHANDLER_H
#define FREERTOS_GREENHOUSE_SWITCHHANDLER_H


#include <cstdio>
#include <map>
#include <vector>

#include "Switch.h"
#include "RTOS_infrastructure.h"

class SwitchHandler {
public:
    SwitchHandler(const RTOS_infrastructure * RTOSi);

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

    button_irq_event_data mEventData{
            .gpio = SW_2,
            .eventMask = GPIO_IRQ_LEVEL_HIGH,
            .timeStamp = 0
    };
private:
    void switch_handler();
    static void task_switch_handler(void *params);

    void rot_event();
    void button_event();

    void state_toggle();
    void insert();
    void next_phase();
    void omit_CO2_target();
    void backspace();

    bool inc_pending_char();
    bool dec_pending_char();

    void set_sw_irq(bool state) const;

    TaskHandle_t mTaskHandle{nullptr};
    static QueueHandle_t mIRQ_eventQueue;


    SW::Button sw2;
    SW::Button sw1;
    SW::Button sw0;
    SW::Button swRotPress;
    SW::Rotor swRotor;

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

    static const uint64_t BUTTON_DEBOUNCE{400000};
    std::map<uint, uint64_t> mPrevEventTimeMap;
    uint64_t mPrevSW_ROT{0};
    swEvent mPrevRotation{UNKNOWN};

    /// state data
    const int16_t CO2_INCREMENT{1};

    uint8_t mState{STATUS};
    int16_t mCO2TargetCurrent{0};
    int16_t mCO2TargetPending{mCO2TargetCurrent};
    char mCharPending{INIT_CHAR};
    uint8_t mNetworkPhase{NEW_API};
    storage_data storageData;
    std::vector<std::string> mNetworkStrings{"", "", ""};

    /// RTOS infrastructure
    const RTOS_infrastructure * iRTOS;
};


#endif //FREERTOS_GREENHOUSE_SWITCHHANDLER_H
