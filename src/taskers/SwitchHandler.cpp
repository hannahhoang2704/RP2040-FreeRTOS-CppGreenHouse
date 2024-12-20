#include "SwitchHandler.h"
#include "Logger.h"
#include "Storage.h"

QueueHandle_t SwitchHandler::mIRQ_eventQueue = nullptr;
uint32_t SwitchHandler::mLostEvents = 0;

void SwitchHandler::irq_handler(uint gpio, uint32_t event_mask) {
    BaseType_t xHigherPriorityWoken = pdFALSE;
    button_irq_event_data eventData{
            .gpio = gpio == ROT_A ? gpio_get(ROT_B) ? ROT_A : ROT_B : gpio,
            .eventMask = static_cast<gpio_irq_level>(event_mask),
            .timeStamp = time_us_64()
    };
    if (xQueueSendToBackFromISR(mIRQ_eventQueue,
                                static_cast<void *>(&eventData),
                                &xHigherPriorityWoken) == errQUEUE_FULL) {
        ++mLostEvents;
    }
    portYIELD_FROM_ISR(xHigherPriorityWoken);
}

SwitchHandler::SwitchHandler(const RTOS_infrastructure *RTOSi) :
        sw2(SW_2, irq_handler),
        sw1(SW_1, irq_handler),
        sw0(SW_0, irq_handler),
        swRotPress(SW_ROT, irq_handler),
        swRotor(ROT_A, ROT_B, irq_handler),
        iRTOS(RTOSi) {
    mIRQ_eventQueue = iRTOS->qSwitchIRQ;
    xTaskCreate(task_switch_handler,
                "SW_HANDLER",
                512,
                (void *) this,
                tskIDLE_PRIORITY + 2,
                &mTaskHandle);
}

void SwitchHandler::task_switch_handler(void *params) {
    auto object_ptr = static_cast<SwitchHandler *>(params);
    object_ptr->switch_handler();
}

void SwitchHandler::switch_handler() {
    Logger::log("Initiated\n");
    xQueueOverwrite(iRTOS->qState, &mState);
    if (xQueuePeek(iRTOS->qCO2TargetCurrent, &mCO2TargetCurrent, pdMS_TO_TICKS(1000)) == pdFALSE) {
        Logger::log("WARNING: qCO2TargetCurrent empty\n", mCO2TargetCurrent);
    } else {
        mCO2TargetPending = mCO2TargetCurrent;
    }
    set_sw_irq(true);
    Logger::log("Switch IRQs enabled\n");

    while (true) {
        while (xQueueReceive(mIRQ_eventQueue,
                             static_cast<void *>(&mEventData),
                             mLostEvents > 0 ? 0 : portMAX_DELAY) == pdTRUE) {
            if (mEventData.gpio == swRotor.mPinA || mEventData.gpio == swRotor.mPinB) {
                rot_event();
            } else {
                button_event();
            }
        }
        if (mLostEvents) {
            Logger::log("ERROR: %u sw events lost\n", mLostEvents);
            mLostEvents = 0;
        }
    }
}

void SwitchHandler::rot_event() {
    if (mEventData.timeStamp - mPrevSW_ROT > BUTTON_DEBOUNCE) {
        // deduce rotation direction
        mEvent = UNKNOWN;
        if (mEventData.eventMask == GPIO_IRQ_EDGE_FALL) {
            mEvent = mEventData.gpio == swRotor.mPinA ? ROT_CLOCKWISE : ROT_COUNTER_CLOCKWISE;
        } else if (mEventData.eventMask == GPIO_IRQ_EDGE_RISE) {
            mEvent = mEventData.gpio == swRotor.mPinA ? ROT_COUNTER_CLOCKWISE : ROT_CLOCKWISE;
        }
        // execute input according to state
        if (mEvent == mPrevRotation) {
            if (mState == STATUS) {
                if (mEvent == ROT_CLOCKWISE && mCO2TargetPending < CO2_MAX) {
                    mCO2TargetPending += CO2_INCREMENT;
                    xQueueOverwrite(iRTOS->qCO2TargetPending, &mCO2TargetPending);
                    if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                        Logger::log("Failed to give sUpdateDisplay\n");
                    }
                } else if (mEvent == ROT_COUNTER_CLOCKWISE && mCO2TargetPending > CO2_MIN) {
                    mCO2TargetPending -= CO2_INCREMENT;
                    xQueueOverwrite(iRTOS->qCO2TargetPending, &mCO2TargetPending);
                    if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                        Logger::log("Failed to give sUpdateDisplay\n");
                    }
                }
            } else {
                if ((mEvent == ROT_CLOCKWISE && inc_pending_char()) ||
                    (mEvent == ROT_COUNTER_CLOCKWISE && dec_pending_char())) {
                    xQueueOverwrite(iRTOS->qCharPending, &mCharPending);
                    if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                        Logger::log("Failed to give sUpdateDisplay\n");
                    }
                }
            }
        }
        mPrevRotation = mEvent;
    }
}

void SwitchHandler::button_event() {
    // reset rotator
    mPrevRotation = UNKNOWN;
    // debounce button
    if (mEventData.timeStamp - mPrevEventTimeMap[mEventData.gpio] > BUTTON_DEBOUNCE) {
        mPrevEventTimeMap[mEventData.gpio] = mEventData.timeStamp;
        switch (mEventData.gpio) {
            case SW_2:
                state_toggle();
                break;
            case SW_1:
                insert();
                break;
            case SW_0:
                if (mState == STATUS) {
                    omit_CO2_target();
                } else if (mState == NETWORK) {
                    next_phase();
                }
                break;
            case SW_ROT:
                backspace();
                mPrevSW_ROT = mEventData.timeStamp;
                break;
            default:
                Logger::log("ERROR: who dis gpio %u\n", mEventData.gpio);

        }
    }
}

/// toggle screen / system state between status and network
void SwitchHandler::state_toggle() {
    mState = mState == STATUS ? NETWORK : STATUS;
    if (mState == STATUS) {
        mCharPending = INIT_CHAR;
        mNetworkPhase = NEW_API;
        xQueueOverwrite(iRTOS->qNetworkPhase, &mNetworkPhase);
        Logger::log("[NETWORK => STATUS]\n");
    } else {
        /// TODO: with ThingSpeaker: reconsider NetworkStrings' handling during and after reconnection
        for (std::string &str: mNetworkStrings) str.clear();
        xQueueOverwrite(iRTOS->qNetworkStrings[NEW_API], mNetworkStrings[NEW_API].c_str());
        xQueueOverwrite(iRTOS->qNetworkStrings[NEW_SSID], mNetworkStrings[NEW_SSID].c_str());
        xQueueOverwrite(iRTOS->qNetworkStrings[NEW_PW], mNetworkStrings[NEW_PW].c_str());
        Logger::log("[STATUS => NETWORK]\n");
    }
    xQueueOverwrite(iRTOS->qState, &mState);
    if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
        Logger::log("Failed to give sUpdateDisplay\n");
    }
}

/// confirm pending rotation position
// status = CO2 target
// network = append character to string
void SwitchHandler::insert() {
    if (mState == STATUS) {
        if (mCO2TargetCurrent != mCO2TargetPending) {
            mCO2TargetCurrent = mCO2TargetPending;
            xQueueOverwrite(iRTOS->qCO2TargetCurrent, &mCO2TargetCurrent);
            xQueueOverwrite(iRTOS->qCO2TargetPending, &mCO2TargetPending);
            Storage::store(CO2_target);
            if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                Logger::log("Failed to give sUpdateDisplay\n");
            }
            if (xSemaphoreGive(iRTOS->sUpdateGreenhouse) == pdFALSE) {
                Logger::log("Failed to give sUpdateGreenhouse\n");
            }
            Logger::log("[STATUS] CO2 set: %hu\n", mCO2TargetCurrent);
        }
    } else {
        if (mNetworkStrings[mNetworkPhase].length() < MAX_CREDENTIAL_STRING_LEN) {
            mNetworkStrings[mNetworkPhase] += mCharPending;
            Logger::log("[NETWORK] API[%s] UN[%s] PW[%s]\n",
                        mNetworkStrings[NEW_API].c_str(),
                        mNetworkStrings[NEW_SSID].c_str(),
                        mNetworkStrings[NEW_PW].c_str());
            /// TODO: with ThingSpeaker: reconsider NetworkStrings' handling during and after reconnection
            xQueueOverwrite(iRTOS->qNetworkStrings[mNetworkPhase], mNetworkStrings[mNetworkPhase].c_str());
            mCharPending = INIT_CHAR;
            xQueueOverwrite(iRTOS->qCharPending, &mCharPending);
            if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                Logger::log("Failed to give sUpdateDisplay\n");
            }
        } else {
            Logger::log("String length capped to 61: [%s]\n", mNetworkStrings[mNetworkPhase].c_str());
        }
    }
}

// confirm credential strings, moving then on to the next credential string
// - if current phase is the last: prompt ThingSpeaker to send reconnect
void SwitchHandler::next_phase() {
    switch (mNetworkPhase) {
        case NEW_API:
            Logger::log("[NETWORK] API[%s] UN[%s] PW[%s]\n",
                        mNetworkStrings[NEW_API].c_str(),
                        mNetworkStrings[NEW_SSID].c_str(),
                        mNetworkStrings[NEW_PW].c_str());
            Storage::store(API_str);
            mNetworkPhase = NEW_SSID;
            xQueueOverwrite(iRTOS->qNetworkPhase, &mNetworkPhase);
            break;
        case NEW_SSID:
            Logger::log("[NETWORK] API[%s] UN[%s] PW[%s]\n",
                        mNetworkStrings[NEW_API].c_str(),
                        mNetworkStrings[NEW_SSID].c_str(),
                        mNetworkStrings[NEW_PW].c_str());
            Storage::store(SSID_str);
            mNetworkPhase = NEW_PW;
            xQueueOverwrite(iRTOS->qNetworkPhase, &mNetworkPhase);
            break;
        case NEW_PW:
            Logger::log("[NETWORK] Connect with: API[%s] UN[%s] PW[%s]\n",
                        mNetworkStrings[NEW_API].c_str(),
                        mNetworkStrings[NEW_SSID].c_str(),
                        mNetworkStrings[NEW_PW].c_str());

            Storage::store(PW_str);

            // TODO: order ThingSpeaker to reconnect
            // TODO: consider how are qNetworkStrings are handled during and after reconnection

            for (std::string &str: mNetworkStrings) str.clear();

            mState = STATUS;
            mNetworkPhase = NEW_API;
            xQueueOverwrite(iRTOS->qState, &mState);
            xQueueOverwrite(iRTOS->qNetworkPhase, &mNetworkPhase);
            break;
    }
    if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
        Logger::log("Failed to give sUpdateDisplay\n");
    }
}

// omit current CO2 target, halting CO2 target pursuit
void SwitchHandler::omit_CO2_target() {
    Logger::log("Omitting CO2Target\n");
    if (xQueueReceive(iRTOS->qCO2TargetCurrent, &mCO2TargetCurrent, 0) == pdFALSE) {
        Logger::log("qCO2TargetCurrent already empty");
    }
    if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
        Logger::log("Failed to give sUpdateDisplay\n");
    }
    if (xSemaphoreGive(iRTOS->sUpdateGreenhouse) == pdFALSE) {
        Logger::log("Failed to give sUpdateGreenhouse\n");
    }
}

/// backspace
// status = resets pending user input (CO2 target) to current CO2 target
// network = remove last character from string corresponding to the phase
//  - If pending string is empty, move back to previous phase. If current phase is the first phase, ignore.
void SwitchHandler::backspace() {
    if (mState == STATUS) {
        mCO2TargetPending = mCO2TargetCurrent;
        xQueueOverwrite(iRTOS->qCO2TargetPending, &mCO2TargetPending);
        if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
            Logger::log("Failed to give sUpdateDisplay\n");
        }
        Logger::log("[STATUS] CO2 reset: %hu\n", mCO2TargetCurrent);
    } else {
        if (mNetworkStrings[mNetworkPhase].empty()) {
            if (mNetworkPhase != NEW_API) {
                mNetworkPhase = mNetworkPhase == NEW_SSID ? NEW_API : NEW_SSID;
                xQueueOverwrite(iRTOS->qNetworkPhase, &mNetworkPhase);
                if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                    Logger::log("Failed to give sUpdateDisplay\n");
                }
            }
        } else {
            mNetworkStrings[mNetworkPhase].pop_back();
            Logger::log("[NETWORK] API[%s] UN[%s] PW[%s]\n",
                        mNetworkStrings[NEW_API].c_str(),
                        mNetworkStrings[NEW_SSID].c_str(),
                        mNetworkStrings[NEW_PW].c_str());

            xQueueOverwrite(iRTOS->qNetworkStrings[mNetworkPhase], mNetworkStrings[mNetworkPhase].c_str());
            mCharPending = INIT_CHAR;
            xQueueOverwrite(iRTOS->qCharPending, &mCharPending);
            if (xSemaphoreGive(iRTOS->sUpdateDisplay) == pdFALSE) {
                Logger::log("Failed to give sUpdateDisplay\n");
            }
        }
    }
}

/* Character Mapping:
 * '0' (zero) is the lowest reachable character, ignoring rotations below it.
 * '~' (tilde) is the highest reachable character, ignoring rotations above it.
 * Incrementing and decrementing alphabet characters runs through upper- and lowercase characters in pairs,
 * like so: AaBbCcDdEe... Zz.
 * General order: ['0' .. '9'], '.', [Aa .. Zz], [! .. ~]
 */

/// increment pending character with a modified lexicography
bool SwitchHandler::inc_pending_char() {
    if (isupper(mCharPending)) {
        mCharPending = tolower(mCharPending);
    } else if (islower(mCharPending) && mCharPending != 'z') {
        mCharPending = toupper(mCharPending) + 1;
    } else
        switch (mCharPending) {
            case '9':
                mCharPending = '.';
                break;
            case '.':
                mCharPending = 'A';
                break;
            case 'z':
                mCharPending = '!';
                break;
            case '-':
                mCharPending = '/';
                break;
            case '/':
                mCharPending = ':';
                break;
            case '@':
                mCharPending = '[';
                break;
            case '`':
                mCharPending = '{';
                break;
            case '~':
                return false;
            default:
                ++mCharPending;
                break;
        }
    return true;
}

/// decrement pending character with a modified lexicography
bool SwitchHandler::dec_pending_char() {
    if (islower(mCharPending)) {
        mCharPending = toupper(mCharPending);
    } else if (isupper(mCharPending) && mCharPending != 'A') {
        mCharPending = tolower(mCharPending) - 1;
    } else
        switch (mCharPending) {
            case '0':
                return false;
            case '.':
                mCharPending = '9';
                break;
            case 'A':
                mCharPending = '.';
                break;
            case '!':
                mCharPending = 'z';
                break;
            case '/':
                mCharPending = '-';
                break;
            case ':':
                mCharPending = '/';
                break;
            case '[':
                mCharPending = '@';
                break;
            case '{':
                mCharPending = '`';
                break;
            default:
                --mCharPending;
                break;
        }
    return true;
}

void SwitchHandler::set_sw_irq(bool state) const {
    sw2.set_irq(state);
    sw1.set_irq(state);
    sw0.set_irq(state);
    swRotPress.set_irq(state);
    swRotor.set_irq(state);
}
