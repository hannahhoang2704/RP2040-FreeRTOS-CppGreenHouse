#include <iomanip>
#include "Logger.h"
#include "Display.h"

using namespace std;

Display::Display(const shared_ptr<PicoI2C> &i2c_sp,
                 const RTOS_infrastructure *RTOSi) :
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

void Display::display() {
    Logger::log("Initiated\n");
    mSSD1306.init();
    print_init();
    mSSD1306.show();
    while (true) {
        xSemaphoreTake(iRTOS->sUpdateDisplay, portMAX_DELAY);
        mSSD1306.fill(0);
        update();
        mSSD1306.show();
        vTaskDelay(10); // some flickering can be witness without limits.
    }
}

void Display::print_init() {
    mSSD1306.text("Initializing...", 0, LINE_0_Y);
}

void Display::update() {
    if (xQueuePeek(iRTOS->qState, &mState, 0) == pdFALSE) {
        Logger::log("WARNING: qState empty\n");
    }
    if (mState == STATUS) {
        print_status_base();
        print_CO2_target();
        print_CO2_measurement();
        print_pressure();
        print_fan();
        print_hum();
        print_temp();
    } else {
        print_network_base();
        print_network_input();
        print_network_pending_char();
    }
    //print_connection();
}

/// STATUS Screen

void Display::print_status_base() {
    mSSD1306.text("CO2T:", 0, LINE_0_Y);
    mSSD1306.text("CO2M:", 0, LINE_1_Y);
    mSSD1306.text("Pres:", 0, LINE_2_Y);
    mSSD1306.text(" Fan:", 0, LINE_3_Y);
    mSSD1306.text(" Hum:", 0, LINE_4_Y);
    mSSD1306.text("Temp:", 0, LINE_5_Y);
}

void Display::print_CO2_target() {
    ssValue.str("");
    bool pendingQempty = xQueuePeek(iRTOS->qCO2TargetPending, &mCO2TargetPending, 0) == pdFALSE;
    bool currentQempty = xQueuePeek(iRTOS->qCO2TargetCurrent, &mCO2TargetCurrent, 0) == pdFALSE;
    if (pendingQempty) {
        Logger::log("WARNING: qCO2TargetPending empty\n");
        if (currentQempty) {
            Logger::log("WARNING: qCO2TargetCurr empty\n");
        }
    }
    bool pending = mCO2TargetPending != mCO2TargetCurrent;
    if (currentQempty) {
        if (pendingQempty || !pending) {
            ssValue << setw(STATUS_VALUE_W) << "   N/A ppm";
        } else {
            ssValue << setw(STATUS_VALUE_W - 2) << mCO2TargetPending << "   ppm";
        }
    } else {
        if (pending) {
            ssValue << setw(STATUS_VALUE_W - 2) << mCO2TargetPending << "   ppm";
        } else {
            ssValue << setw(STATUS_VALUE_W - 2) << mCO2TargetCurrent << "   ppm";
        }
    }
    if (!pendingQempty) {
        if (pending)
            mSSD1306.rect(STATUS_VALUE_X - CHAR_WIDTH, LINE_0_Y - 1,
                          OLED_WIDTH - STATUS_VALUE_X + CHAR_WIDTH, 9, pending, true);
    }
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, LINE_0_Y, pendingQempty || !pending);
}

void Display::print_CO2_measurement() {
    ssValue.str("");
    if (xQueuePeek(iRTOS->qCO2Measurement, &mCO2Measurement, 0) == pdFALSE) {
        Logger::log("WARNING: qCO2Measurement empty\n");
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mCO2Measurement;
    }
    ssValue << " ppm";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, LINE_1_Y, 1);
}

void Display::print_pressure() {
    ssValue.str("");
    if (xQueuePeek(iRTOS->qPressure, &mPressure, 0) == pdFALSE) {
        Logger::log("WARNING: qPressure empty\n");
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W - 2) << mPressure;
    }
    ssValue << "   Pa";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, LINE_2_Y, 1);
}

void Display::print_fan() {
    ssValue.str("");
    if (xQueuePeek(iRTOS->qFan, &mFan, 0) == pdFALSE) {
        Logger::log("WARNING: qFan empty\n");
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W - 2) << mFan / 10 << "." << mFan % 10;
    }
    ssValue << " %";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, LINE_3_Y, 1);
}

void Display::print_hum() {
    ssValue.str("");
    if (xQueuePeek(iRTOS->qHumidity, &mHumidity, 0) == pdFALSE) {
        Logger::log("WARNING: qHumidity empty\n");
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mHumidity;
    }
    ssValue << " %";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, LINE_4_Y, 1);
}

void Display::print_temp() {
    ssValue.str("");
    if (xQueuePeek(iRTOS->qTemperature, &mTemperature, 0) == pdFALSE) {
        Logger::log("WARNING: qTemperature empty\n");
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mTemperature;
    }
    ssValue << " C";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, LINE_5_Y, 1);
}

/// NETWORK Screen

void Display::print_network_base() {
    uint8_t prevPhase = mNetworkPhase;
    if (xQueuePeek(iRTOS->qNetworkPhase, &mNetworkPhase, 0) == pdFALSE) {
        Logger::log("ERROR: qNetworkPhase empty\n");
    } else {
        if (prevPhase == NEW_PW && mNetworkPhase == NEW_API) {
            for (std::string &str: mNetworkStrings) str.clear();
            print_status_base();
            print_CO2_target();
            print_CO2_measurement();
            print_pressure();
            print_fan();
            print_hum();
            print_temp();
            return;
        }
    }
    switch (mNetworkPhase) {
        case NEW_PW:
            mSSD1306.text("WiFi PW:", 0, LINE_4_Y);
        case NEW_SSID:
            mSSD1306.text("WiFi SSID:", 0, LINE_2_Y);
        case NEW_API:
            mSSD1306.text("ThingSpeak API:", 0, LINE_0_Y);
    }
}

void Display::print_network_input() {
    for (uint8_t str_i = 0; str_i < 3; ++str_i) {
        char buf[MAX_CREDENTIAL_STRING_LEN + 1];
        if (xQueuePeek(iRTOS->qNetworkStrings[str_i], buf, 0) == pdFALSE) {
            Logger::log("WARNING: qNetworkStrings[%s] empty\n",
                        (str_i == NEW_API ? "NEW_API" : str_i == NEW_SSID ? "NEW_SSID" : "NEW_PW"));
        }
        mNetworkStrings[str_i] = buf;
    }

    bool tooLong;
    std::string cut_str;

    switch (mNetworkPhase) {
        case NEW_PW:
            tooLong = mNetworkStrings[NEW_PW].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mNetworkStrings[NEW_PW];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mNetworkStrings[NEW_PW], 0, LINE_5_Y);
        case NEW_SSID:
            tooLong = mNetworkStrings[NEW_SSID].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mNetworkStrings[NEW_SSID];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mNetworkStrings[NEW_SSID], 0, LINE_3_Y);
        case NEW_API:
            tooLong = mNetworkStrings[NEW_API].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mNetworkStrings[NEW_API];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mNetworkStrings[NEW_API], 0, LINE_1_Y);
    }
}

void Display::print_network_pending_char() {
    if (xQueuePeek(iRTOS->qCharPending, &mCharPending, 0) == pdFALSE) {
        Logger::log("WARNING: qCharPending empty\n");
    }

    size_t str_len = mNetworkStrings[mNetworkPhase].length();
    bool tooLong = str_len > MAX_OLED_STR_WIDTH - 1;
    uint8_t line;

    switch (mNetworkPhase) {
        case NEW_API:
            line = LINE_1_Y;
            break;
        case NEW_SSID:
            line = LINE_3_Y;
            break;
        case NEW_PW:
            line = LINE_5_Y;
            break;
    }
    mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                  line - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
    mSSD1306.text(&mCharPending,
                  tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                  line * 1, 0);
}

void Display::print_connection() {
    if (xQueuePeek(iRTOS->qConnectionState, &mConnectionState, 0) == pdFALSE) {
        Logger::log("qConnectionState empty\n");
    }
    ssValue.str("");
    ssValue << setw(MAX_OLED_STR_WIDTH);
    switch (mConnectionState) {
        case bCONNECTING:
            ssValue << setw(MAX_OLED_STR_WIDTH) << "Connecting...";
            break;
        case bCONNECTED:
            ssValue << setw(MAX_OLED_STR_WIDTH) << "Connected";
            break;
        case bNOT_CONNECTED:
            ssValue << setw(MAX_OLED_STR_WIDTH) << "Not connected";
            break;
    }
    mSSD1306.text(ssValue.str(), 0, LINE_6_Y);
}
