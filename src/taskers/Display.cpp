#include <iomanip>
#include "Logger.h"
#include "Display.h"

using namespace std;

Display::Display(const shared_ptr<PicoI2C> &i2c_sp,
                 RTOS_infrastructure RTOSi) :
        mSSD1306(i2c_sp),
        iRTOS(RTOSi) {
    if (xTaskCreate(task_display,
                    "DISPLAY",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created DISPLAY task.\n");
    } else {
        Logger::log("Failed to create DISPLAY task.\n");
    }
}

void Display::task_display(void *params) {
    auto object_ptr = static_cast<Display *>(params);
    object_ptr->display();
}

TaskHandle_t Display::get_handle() const {
    return mTaskHandle;
}

void Display::display() {
    Logger::log("Initiated DISPLAY task.\n");
    mSSD1306.init();
    print_status_base();
    reprint_CO2_target();
    mSSD1306.text("Connected", 0, CONNECTION_Y);
    mSSD1306.show();

    while (true) {
        mNotification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        mSSD1306.fill(0);

        vTaskDelay(1);
        Logger::log("Note: " + to_string(mNotification) + "\n");
        vTaskDelay(1);
        update();
        mSSD1306.show();
    }
}

void Display::update() {
    if (mNotification & bSTATE) {
        xQueuePeek(iRTOS.qState, &mState, portMAX_DELAY);
        print_new_state();
        if (mState == STATUS) {
            for (std::string &str: mRelogStrings) str.clear();
        }
    }
    if (mState == STATUS) {
        print_status_base();
        if (mNotification & bCO2_TARGET) {
            xQueuePeek(iRTOS.qCO2TargetCurr, &mCO2TargetCurr, 0);
            xQueuePeek(iRTOS.qCO2TargetPending, &mCO2TargetPending, 0);
            reprint_CO2_target();
        }
        if (mNotification & bCO2_MEASURE) {

        }
        if (mNotification & bPRESSURE);
        if (mNotification & bFAN);
        if (mNotification & bHUMIDITY);
        if (mNotification & bTEMPERATURE);

    } else if (mState == NETWORK) {
        print_network_base();
        if (mNotification & bNETWORK_PHASE) {
            xQueuePeek(iRTOS.qNetworkPhase, &mRelogPhase, portMAX_DELAY);
            print_network_base();
        }
        if (mNotification & bCHAR) {
            xQueuePeek(iRTOS.qCharPending, &mCharPending, portMAX_DELAY);
        }
        if (mNotification & bINSERT_CHAR) {
            xQueuePeek(iRTOS.qCharPending, &mCharPending, 0);
            mRelogStrings[mRelogPhase] += mCharPending;
            mCharPending = INIT_CHAR;
        }
        if (mNotification & bBACKSPACE) {
            mRelogStrings[mRelogPhase].pop_back();
            mCharPending = INIT_CHAR;
        }
        if (mNotification & (bCHAR | bBACKSPACE | bNETWORK_PHASE | bINSERT_CHAR)) {
            reprint_network_pending_char();
            reprint_network_input();
        }
    }
    if (mNotification & bCONNCETING) {
        mSSD1306.text("Connecting...", 0, CONNECTION_Y);
    }
}

void Display::print_new_state() {
    switch (mState) {
        case STATUS:
            print_status_base();
            reprint_CO2_target();
            reprint_CO2_measurement();
            reprint_pressure();
            reprint_fan();
            reprint_hum();
            reprint_temp();
            break;
        case NETWORK:
            print_network_base();
            reprint_network_pending_char();
            break;
    }
}

/// STATUS Screen

void Display::print_status_base() {
    mSSD1306.text("CO2T:", 0, STATUS_CO2T_Y);
    mSSD1306.text("CO2M:", 0, STATUS_CO2M_Y);
    mSSD1306.text("Pres:", 0, STATUS_PRES_Y);
    mSSD1306.text(" Fan:", 0, STATUS_FAN_Y);
    mSSD1306.text(" Hum:", 0, STATUS_HUM_Y);
    mSSD1306.text("Temp:", 0, STATUS_TEMP_Y);
}

void Display::reprint_CO2_target() {
    //mSSD1306.rect(STATUS_CO2T_X, STATUS_CO2T_Y - 1, OLED_WIDTH - STATUS_CO2T_X, 9, 0, true);
    bool pending = mCO2TargetPending != mCO2TargetCurr;
    mCO2T_ss.str("");
    mCO2T_ss << setw(5) << mCO2TargetPending;
    mCO2T_ss << " ppm";

    if (pending) mSSD1306.rect(STATUS_VALUE_X, STATUS_CO2T_Y - 1, OLED_WIDTH - STATUS_VALUE_X, 9, pending, true);
    mSSD1306.text(mCO2T_ss.str(), STATUS_VALUE_X, STATUS_CO2T_Y, !pending);
}

void Display::reprint_CO2_measurement() {
    mCO2T_ss.str("");
    mCO2T_ss << setw(5) << mCO2TargetPending;
    mCO2T_ss << " ppm";
    mSSD1306.text(mCO2T_ss.str(), STATUS_VALUE_X, STATUS_CO2M_Y, 1);
}

void Display::reprint_pressure() {

}

void Display::reprint_fan() {

}

void Display::reprint_hum() {

}

void Display::reprint_temp() {

}

/// NETWORK Screen

void Display::print_network_base() {
    switch (mRelogPhase) {
        case NEW_PW:
            mSSD1306.text("WiFi PW:", 0, NETWORK_DESC_PW_Y);
        case NEW_SSID:
            mSSD1306.text("WiFi SSID:", 0, NETWORK_DESC_SSID_Y);
        case NEW_IP:
            mSSD1306.text("ThingSpeak IP:", 0, NETWORK_DESC_IP_Y);
    }
}

void Display::reprint_network_input() {

    bool tooLong;
    std::string cut_str;

    switch (mRelogPhase) {
        case NEW_PW:
            tooLong = mRelogStrings[NEW_PW].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mRelogStrings[mRelogPhase];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mRelogStrings[NEW_PW], 0, NETWORK_INPUT_PW_Y);
        case NEW_SSID:
            tooLong = mRelogStrings[NEW_SSID].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mRelogStrings[mRelogPhase];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mRelogStrings[NEW_SSID], 0, NETWORK_INPUT_SSID_Y);
        case NEW_IP:
            tooLong = mRelogStrings[NEW_IP].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mRelogStrings[mRelogPhase];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mRelogStrings[NEW_IP], 0, NETWORK_INPUT_IP_Y);
    }
}

void Display::reprint_network_pending_char() {
    size_t str_len = mRelogStrings[mRelogPhase].length();
    bool tooLong = str_len > MAX_OLED_STR_WIDTH - 1;

    switch (mRelogPhase) {
        case NEW_IP:
            mSSD1306.rect(0, NETWORK_INPUT_IP_Y - 1, OLED_WIDTH, CHAR_HEIGHT + 2, 0, true);
            mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_IP_Y - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
            mSSD1306.text(&mCharPending,
                          tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_IP_Y, 0);
            break;
        case NEW_SSID:
            mSSD1306.rect(0, NETWORK_INPUT_SSID_Y - 1, OLED_WIDTH, CHAR_HEIGHT + 2, 0, true);
            mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_SSID_Y - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
            mSSD1306.text(&mCharPending,
                          tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_SSID_Y, 0);
            break;
        case NEW_PW:
            mSSD1306.rect(0, NETWORK_INPUT_PW_Y - 1, OLED_WIDTH, CHAR_HEIGHT + 2, 0, true);
            mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_PW_Y - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
            mSSD1306.text(&mCharPending,
                          tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_PW_Y, 0);
            break;
    }
}
