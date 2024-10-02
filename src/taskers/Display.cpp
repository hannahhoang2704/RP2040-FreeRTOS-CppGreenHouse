#include <iomanip>
#include "Logger.h"
#include "Display.h"

using namespace std;

TaskHandle_t Display::mTaskHandle{nullptr};
uint64_t Display::mPrevNotification{0};

void Display::notify(eNotifyAction eAction, uint32_t note) {
    if (time_us_64() - mPrevNotification > TASK_NOTIFICATION_RATE_LIMIT_US) {
        mPrevNotification = time_us_64();
        xTaskNotify(mTaskHandle, note, eAction);
    }
}

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

void Display::display() {
    Logger::log("Initiated\n");
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
            mCO2TargetPending = mCO2TargetCurrent;
        } else {
            for (std::string &str: mNetworkStrings) str.clear();
            mNetworkPhase = NEW_IP;
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
        reprint_network_pending_char();
        print_network_base();
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
    ssValue.str("");
    bool pendingQempty = xQueuePeek(iRTOS.qCO2TargetPending, &mCO2TargetPending, 0) == pdFALSE;
    bool currentQempty = xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2TargetCurrent, 0) == pdFALSE;
    if (mNotification & bCO2_TARGET) {
        if (pendingQempty) {
            Logger::log("WARNING: qCO2TargetPending empty\n");
        }
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
        if (pendingQempty) {
            ssValue << setw(STATUS_VALUE_W - 2) << mCO2TargetCurrent << "   ppm";
        } else {
            ssValue << setw(STATUS_VALUE_W - 2) << mCO2TargetPending << "   ppm";
        }
    }
    if (!pendingQempty) {
        if (pending)
            mSSD1306.rect(STATUS_VALUE_X - CHAR_WIDTH, STATUS_CO2T_Y - 1,
                          OLED_WIDTH - STATUS_VALUE_X + CHAR_WIDTH, 9, pending, true);
    }
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_CO2T_Y, pendingQempty || !pending);
}

void Display::reprint_CO2_measurement() {
    ssValue.str("");
    if (xQueuePeek(iRTOS.qCO2Measurement, &mCO2Measurement, 0) == pdFALSE) {
        if (mNotification & bCO2_MEASURE) {
            Logger::log("WARNING: qCO2Measurement empty\n");
        }
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mCO2Measurement;
    }
    ssValue << " ppm";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_CO2M_Y, 1);
}

void Display::reprint_pressure() {
    ssValue.str("");
    if (xQueuePeek(iRTOS.qPressure, &mPressure, 0) == pdFALSE) {
        if (mNotification & bPRESSURE) {
            Logger::log("WARNING: qPressure empty\n");
        }
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mPressure;
    }
    ssValue << " ppm";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_PRES_Y, 1);
}

void Display::reprint_fan() {
    ssValue.str("");
    if (xQueuePeek(iRTOS.qFan, &mFan, 0) == pdFALSE) {
        if (mNotification & bFAN) {
            Logger::log("WARNING: qFan empty\n");
        }
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W - 2) << mFan / 10 << "." << mFan % 10;
    }
    ssValue << " %";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_FAN_Y, 1);
}

void Display::reprint_hum() {
    ssValue.str("");
    if (xQueuePeek(iRTOS.qHumidity, &mHumidity, 0) == pdFALSE) {
        if (mNotification & bHUMIDITY) {
            Logger::log("WARNING: qHumidity empty\n");
        }
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mHumidity;
    }
    ssValue << " %";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_HUM_Y, 1);
}

void Display::reprint_temp() {
    ssValue.str("");
    if (xQueuePeek(iRTOS.qTemperature, &mTemperature, 0) == pdFALSE) {
        if (mNotification & bTEMPERATURE) {
            Logger::log("WARNING: qTemperature empty\n");
        }
        ssValue << setw(STATUS_VALUE_W) << "N/A";
    } else {
        ssValue << setw(STATUS_VALUE_W) << setprecision(1) << fixed << mTemperature;
    }
    ssValue << " C";
    mSSD1306.text(ssValue.str(), STATUS_VALUE_X, STATUS_TEMP_Y, 1);
}

/// NETWORK Screen

void Display::print_network_base() {
    if (mNotification & bNETWORK_PHASE) {
        network_phase prevPhase = mNetworkPhase;
        if (mNotification & bNETWORK_PHASE && xQueuePeek(iRTOS.qNetworkPhase, &mNetworkPhase, 0) == pdFALSE) {
            Logger::log("ERROR: qNetworkPhase empty\n");
        } else {
            if (prevPhase == NEW_PW && mNetworkPhase == NEW_IP) {
                for (std::string &str: mNetworkStrings) str.clear();
                print_status_base();
                reprint_CO2_target();
                reprint_CO2_measurement();
                reprint_pressure();
                reprint_fan();
                reprint_hum();
                reprint_temp();
                return;
            }
        }
    }
    switch (mNetworkPhase) {
        case NEW_PW:
            mSSD1306.text("WiFi PW:", 0, NETWORK_DESC_PW_Y);
        case NEW_SSID:
            mSSD1306.text("WiFi SSID:", 0, NETWORK_DESC_SSID_Y);
        case NEW_IP:
            mSSD1306.text("ThingSpeak IP:", 0, NETWORK_DESC_IP_Y);
    }
}

void Display::reprint_network_pending_char() {
    if (mNotification & (bCHANGE_CHAR | bINSERT_CHAR) && xQueuePeek(iRTOS.qCharPending, &mCharPending, 0) == pdFALSE) {
        Logger::log("ERROR: qCharPending empty\n");
    } else {
        if (mNotification & bINSERT_CHAR) {
            mNetworkStrings[mNetworkPhase] += mCharPending;
            mCharPending = INIT_CHAR;
        } else if (mNotification & bBACKSPACE) {
            if (mNetworkStrings[mNetworkPhase].empty()) {
                switch (mNetworkPhase) {
                    case NEW_IP:
                        break;
                    case NEW_SSID:
                        mNetworkPhase = NEW_IP;
                        break;
                    case NEW_PW:
                        mNetworkPhase = NEW_PW;
                        break;
                }
            } else {
                mNetworkStrings[mNetworkPhase].pop_back();
            }
            mCharPending = INIT_CHAR;
        }
    }

    size_t str_len = mNetworkStrings[mNetworkPhase].length();
    bool tooLong = str_len > MAX_OLED_STR_WIDTH - 1;

    switch (mNetworkPhase) {
        case NEW_IP:
            mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_IP_Y - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
            mSSD1306.text(&mCharPending,
                          tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_IP_Y, 0);
            break;
        case NEW_SSID:
            mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_SSID_Y - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
            mSSD1306.text(&mCharPending,
                          tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_SSID_Y, 0);
            break;
        case NEW_PW:
            mSSD1306.rect(tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_PW_Y - 1, CHAR_WIDTH, CHAR_HEIGHT + 2, 1, true);
            mSSD1306.text(&mCharPending,
                          tooLong ? 7 * (MAX_OLED_STR_WIDTH + 1) + 1 : str_len * CHAR_WIDTH,
                          NETWORK_INPUT_PW_Y, 0);
            break;
    }
}

void Display::reprint_network_input() {

    bool tooLong;
    std::string cut_str;

    switch (mNetworkPhase) {
        case NEW_PW:
            tooLong = mNetworkStrings[NEW_PW].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mNetworkStrings[mNetworkPhase];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mNetworkStrings[NEW_PW], 0, NETWORK_INPUT_PW_Y);
        case NEW_SSID:
            tooLong = mNetworkStrings[NEW_SSID].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mNetworkStrings[mNetworkPhase];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mNetworkStrings[NEW_SSID], 0, NETWORK_INPUT_SSID_Y);
        case NEW_IP:
            tooLong = mNetworkStrings[NEW_IP].length() >= MAX_OLED_STR_WIDTH;
            if (tooLong) {
                cut_str = mNetworkStrings[mNetworkPhase];
                cut_str.erase(0, cut_str.length() - (MAX_OLED_STR_WIDTH - 1));
            }
            mSSD1306.text(tooLong ? cut_str : mNetworkStrings[NEW_IP], 0, NETWORK_INPUT_IP_Y);
    }
}
