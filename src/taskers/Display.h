#ifndef RP2040_FREERTOS_IRQ_DISPLAY_H
#define RP2040_FREERTOS_IRQ_DISPLAY_H


#include <memory>
#include <sstream>
#include <vector>
#include "FreeRTOS.h"

#include "i2c/PicoI2C.h"
#include "display/ssd1306os.h"
#include "RTOS_infrastructure.h"

class Display {
public:
    Display(const std::shared_ptr<PicoI2C> &i2c_sp,
            RTOS_infrastructure RTOSi);

private:
    void display();
    static void task_display(void *params);

    void update();

    void print_status_base();
    void print_CO2_target();
    void print_CO2_measurement();
    void print_pressure();
    void print_fan();
    void print_hum();
    void print_temp();
    void print_network_base();
    void print_network_input();
    void print_network_pending_char();
    void print_connection();

    TaskHandle_t mTaskHandle;
    ssd1306os mSSD1306;

    /// printing axioms
    static const uint8_t OLED_WIDTH {128};
    static const uint8_t OLED_HEIGHT{64};
    static const uint8_t CHAR_WIDTH{8};
    static const uint8_t CHAR_HEIGHT{8};
    static const uint8_t MAX_OLED_STR_WIDTH {128 / CHAR_WIDTH};
    static const uint8_t MAX_OLED_STR_HEIGHT{64 / CHAR_HEIGHT};

    static const uint8_t LINE_GAP{1};
    static const uint8_t LINE_0_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 0};
    static const uint8_t LINE_1_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 1};
    static const uint8_t LINE_2_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 2};
    static const uint8_t LINE_3_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 3};
    static const uint8_t LINE_4_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 4};
    static const uint8_t LINE_5_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 5};
    static const uint8_t LINE_6_Y{1 + (CHAR_HEIGHT + LINE_GAP) * 6};

    static const uint8_t STATUS_VALUE_X{CHAR_WIDTH * 6 - 1};
    static const uint8_t STATUS_VALUE_W{6};

    /// program state data
    uint8_t mState{STATUS};
    int16_t mCO2TargetCurrent{0};
    int16_t mCO2TargetPending{0};
    float mCO2Measurement{0};
    float mPressure{0};
    int16_t mFan{0};
    float mHumidity{0};
    float mTemperature{0};
    char mCharPending{INIT_CHAR};
    uint8_t mNetworkPhase{NEW_IP};
    uint8_t mCharAction{bNONE};
    uint8_t mConnectionState{bCONNECTING};
    std::vector<std::string> mNetworkStrings{"", "", ""};

    std::stringstream ssValue;

    /// RTOS infrastructure
    RTOS_infrastructure iRTOS;
};


#endif //RP2040_FREERTOS_IRQ_DISPLAY_H
