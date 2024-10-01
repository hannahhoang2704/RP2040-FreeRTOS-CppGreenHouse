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
    reprint_CO2_measurement();
    reprint_pressure();
    reprint_fan();
    reprint_hum();
    reprint_temp();
    mSSD1306.text("Connected", 0, CONNECTION_Y);
    mSSD1306.show();

    while (true) {
        mNotification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        mSSD1306.fill(0);
        update();
        mSSD1306.show();
    }
}

void Display::update() {
    if (mNotification & bSTATE) {
        mState = mState == STATUS ? NETWORK : STATUS;
        if (mState == STATUS) {
            mCO2TargetPending = mCO2TargetCurr;
        } else {
            for (std::string &str: mRelogStrings) str.clear();
            mRelogPhase = NEW_IP;
        }
    }

    if (mState == STATUS) {
        print_status_base();
        reprint_CO2_target();
        reprint_CO2_measurement();
        reprint_pressure();
        reprint_fan();
        reprint_hum();
        reprint_temp();
    } else {
        print_network_base();
        reprint_network_pending_char();
        reprint_network_input();
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
    //mSSD1306.rect(STATUS_VALUE_X, STATUS_CO2T_Y - 1, OLED_WIDTH - STATUS_VALUE_X, CHAR_HEIGHT + 1, 0, true);
    xQueuePeek(iRTOS.qCO2TargetPending, &mCO2TargetPending, 0);
    xQueuePeek(iRTOS.qCO2TargetCurr, &mCO2TargetCurr, 0);
    bool pending = mCO2TargetPending != mCO2TargetCurr;
    ssValue.str("");
    ssValue << setw(STATUS_VALUE_W - 2) << mCO2TargetPending;
    ssValue << "   ppm";

    if (pending) mSSD1306.rect(STATUS_VALUE_X - CHAR_WIDTH, STATUS_CO2T_Y - 1,
                               OLED_WIDTH - STATUS_VALUE_X + CHAR_WIDTH, 9, pending, true);
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_CO2T_Y, !pending);
}

void Display::reprint_CO2_measurement() {
    //xQueuePeek(iRTOS.qCO2Meausure, mCO2Measurement, 0);
    ssValue.str("");
    ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mCO2Measurement;
    ssValue << " ppm";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_CO2M_Y, 1);
}

void Display::reprint_pressure() {
    //xQueuePeek(iRTOS.qPressure, mCO2Measurement, 0);
    ssValue.str("");
    ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mPressure;
    ssValue << " ppm";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_PRES_Y, 1);
}

void Display::reprint_fan() {
    //xQueuePeek(iRTOS.qFan, mCO2Measurement, 0);

    ssValue.str("");
    ssValue << setw(STATUS_VALUE_W - 2) << mFan / 10 << "." << mFan % 10;
    ssValue << " %";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_FAN_Y, 1);
}

void Display::reprint_hum() {
    //xQueuePeek(iRTOS.qHumidity, mCO2Measurement, 0);
    ssValue.str("");
    ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mRelHum;
    ssValue << " %";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_HUM_Y, 1);
}

void Display::reprint_temp() {
    //xQueuePeek(iRTOS.qTemperature, mCO2Measurement, 0);
    ssValue.str("");
    ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mTemp;
    ssValue << " C";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_TEMP_Y, 1);
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
