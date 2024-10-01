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

    TaskHandle_t get_handle() const;

private:
    void display();
    static void task_display(void *params);

    void update();

    void print_new_state();
    void print_status_base();
    void reprint_CO2_target();
    void reprint_CO2_measurement();
    void reprint_pressure();
    void reprint_fan();
    void reprint_hum();
    void reprint_temp();
    void print_network_base();
    void reprint_network_input();
    void reprint_network_pending_char();

    TaskHandle_t mTaskHandle;
    ssd1306os mSSD1306;

    static const uint8_t OLED_WIDTH {128};
    static const uint8_t OLED_HEIGHT{64};
    static const uint8_t CHAR_WIDTH{8};
    static const uint8_t CHAR_HEIGHT{8};
    static const uint8_t MAX_OLED_STR_WIDTH {128 / CHAR_WIDTH};
    static const uint8_t MAX_OLED_STR_HEIGHT{64 / CHAR_HEIGHT};

    static const uint8_t STATUS_GAP{1};
    static const uint8_t STATUS_CO2T_Y{1 + (CHAR_HEIGHT + STATUS_GAP) * 0};
    static const uint8_t STATUS_CO2M_Y{1 + (CHAR_HEIGHT + STATUS_GAP) * 1};
    static const uint8_t STATUS_PRES_Y{1 + (CHAR_HEIGHT + STATUS_GAP) * 2};
    static const uint8_t STATUS_FAN_Y {1 + (CHAR_HEIGHT + STATUS_GAP) * 3};
    static const uint8_t STATUS_HUM_Y {1 + (CHAR_HEIGHT + STATUS_GAP) * 4};
    static const uint8_t STATUS_TEMP_Y{1 + (CHAR_HEIGHT + STATUS_GAP) * 5};

    static const uint8_t STATUS_VALUE_X{CHAR_WIDTH * 6};

    static const uint8_t NETWORK_DESC_IP_Y  {22 * 0};
    static const uint8_t NETWORK_DESC_SSID_Y{22 * 1};
    static const uint8_t NETWORK_DESC_PW_Y  {22 * 2};
    static const uint8_t NETWORK_INPUT_IP_Y  {9 + NETWORK_DESC_IP_Y};
    static const uint8_t NETWORK_INPUT_SSID_Y{9 + NETWORK_DESC_SSID_Y};
    static const uint8_t NETWORK_INPUT_PW_Y  {9 + NETWORK_DESC_PW_Y};

    static const uint8_t CONNECTION_Y{57};

    /// program state data
    uint32_t mNotification;
    program_state mState{STATUS};
    int16_t mCO2TargetCurr{0};
    int16_t mCO2TargetPending{0};
    float mCO2Measurement{0};
    float mPressure{0};
    int16_t mFan{0};
    float mRelHum{0};
    float mTemp{0};
    char mCharPending{};
    network_phase mRelogPhase{NEW_IP};
    std::vector<std::string> mRelogStrings{"", "", ""};

    std::stringstream mCO2T_ss;
    std::stringstream mCO2M_ss;
    std::stringstream mPressure_ss;
    std::stringstream mFan_ss;
    std::stringstream mRelHum_ss;
    std::stringstream mTemp_ss;

    /// RTOS infrastructure
    RTOS_infrastructure iRTOS;
};


#endif //RP2040_FREERTOS_IRQ_DISPLAY_H
