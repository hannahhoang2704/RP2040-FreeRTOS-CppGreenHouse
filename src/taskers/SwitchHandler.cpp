#include "SwitchHandler.h"
#include "Logger.h"
#include "Display.h"

QueueHandle_t SwitchHandler::mIRQ_eventQueue = xQueueCreate(10, sizeof(button_irq_event_data));;
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

SwitchHandler::SwitchHandler(RTOS_infrastructure RTOSi) :
        sw2(SW_2, irq_handler),
        sw1(SW_1, irq_handler),
        sw0(SW_0, irq_handler),
        swRotPress(SW_ROT, irq_handler),
        swRotor(ROT_A, ROT_B, irq_handler),
        iRTOS(RTOSi) {
    if (xTaskCreate(task_switch_handler,
                    "SW_HANDLER",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created SW_HANDLER task\n");
    } else {
        Logger::log("Failed to create SW_HANDLER task\n");
    }
}

void SwitchHandler::task_switch_handler(void *params) {
    auto object_ptr = static_cast<SwitchHandler *>(params);
    object_ptr->switch_handler();
}

void SwitchHandler::switch_handler() {
    Logger::log("Initiated\n");
    Logger::log("Waiting for storage to upload CO2TargetCurrent...\n");
    if (xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2TargetCurrent, pdMS_TO_TICKS(1000)) == pdFALSE) {
        Logger::log("WARNING: Storage didn't upload CO2TargetCurrent. Defaulting to %hu ppm\n", mCO2TargetCurrent);
    } else {
        mCO2TargetPending = mCO2TargetCurrent;
    }

    set_sw_irq(true);
    Logger::log("Switch IRQs enabled\n");

    while (true) {
        while (xQueueReceive(mIRQ_eventQueue,
                             static_cast<void *>(&mEventData),
                             mLostEvents > 0 ? 0 : portMAX_DELAY) == pdTRUE) {
            mDisplayNote = 0;
            if (mEventData.gpio == swRotor.mPinA || mEventData.gpio == swRotor.mPinB) {
                rot_event();
            } else {
                button_event();
            }
            Display::notify(eSetBits, mDisplayNote);
        }
        if (mLostEvents) {
            Logger::log("ERROR: %u sw events lost\n", mLostEvents);
            mLostEvents = 0;
        }
    }
}

void SwitchHandler::rot_event() {
    if (mEventData.timeStamp - mPrevBackspace > BUTTON_DEBOUNCE) {
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
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    mDisplayNote |= bCO2_TARGET;
                } else if (mEvent == ROT_COUNTER_CLOCKWISE && mCO2TargetPending > CO2_MIN) {
                    mCO2TargetPending -= CO2_INCREMENT;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    mDisplayNote |= bCO2_TARGET;
                }
            } else {
                if ((mEvent == ROT_CLOCKWISE && inc_pending_char()) ||
                    (mEvent == ROT_COUNTER_CLOCKWISE && dec_pending_char())) {
                    xQueueOverwrite(iRTOS.qCharPending, &mCharPending);
                    mDisplayNote |= bCHANGE_CHAR;
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
                next_phase();
                break;
            case SW_ROT:
                backspace();
                break;
            default:
                Logger::log("ERROR: who dis gpio %u\n", mEventData.gpio);

        }
    }
}

/// change system state -- status or relog
// changing system state resets pending string input
void SwitchHandler::state_toggle() {
    mState = mState == STATUS ? NETWORK : STATUS;
    if (mState == STATUS) {
        mCharPending = INIT_CHAR;
        for (std::string &str: mNetworkStrings) str.clear();
        mNetworkPhase = NEW_IP;
        xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
        Logger::log("[NETWORK => STATUS]\n");
    } else {
        Logger::log("[STATUS => NETWORK]\n");
    }
    mDisplayNote |= bSTATE;
}

/// confirm rotated adjustment
// status = CO2 target
// relog = character to string
void SwitchHandler::insert() {
    if (mState == STATUS) {
        if (mCO2TargetCurrent != mCO2TargetPending) {
            mCO2TargetCurrent = mCO2TargetPending;
            xQueueOverwrite(iRTOS.qCO2TargetCurrent, &mCO2TargetCurrent);
            xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
            mDisplayNote |= bCO2_TARGET;
            Logger::log("[STATUS] CO2 set: %hu\n", mCO2TargetCurrent);
        }
    } else {
        mNetworkStrings[mNetworkPhase] += mCharPending;
        Logger::log("[NETWORK] IP[%s] UN[%s] PW[%s]\n",
                    mNetworkStrings[NEW_IP].c_str(),
                    mNetworkStrings[NEW_SSID].c_str(),
                    mNetworkStrings[NEW_PW].c_str());
        mCharPending = INIT_CHAR;
        mDisplayNote |= bINSERT_CHAR;
    }
}

/// confirm a series of confirmed rotated adjustments
// applies only (?) in relog state when confirming string, moving then on to the next phase
void SwitchHandler::next_phase() {
    if (mState == NETWORK) {
        switch (mNetworkPhase) {
            case NEW_IP:
                Logger::log("[NETWORK] IP[%s] UN[%s] PW[%s]\n",
                            mNetworkStrings[NEW_IP].c_str(),
                            mNetworkStrings[NEW_SSID].c_str(),
                            mNetworkStrings[NEW_PW].c_str());
                vTaskDelay(1);
                mNetworkPhase = NEW_SSID;
                xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                break;
            case NEW_SSID:
                Logger::log("[NETWORK] IP[%s] UN[%s] PW[%s]\n",
                            mNetworkStrings[NEW_IP].c_str(),
                            mNetworkStrings[NEW_SSID].c_str(),
                            mNetworkStrings[NEW_PW].c_str());
                mNetworkPhase = NEW_PW;
                xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                break;
            case NEW_PW:
                // TODO: order reconnection

                for (std::string &str: mNetworkStrings) str.clear();

                mState = STATUS;
                mDisplayNote |= bSTATE;

                mNetworkPhase = NEW_IP;
                xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                break;
        }
        mDisplayNote |= bNETWORK_PHASE;
    }
}

/// backspace
// status = resets pending user input (CO2 target) to current CO2 target
// relog = remove last character from string corresponding to the phase
//  - If pending string is empty, move back to previous phase. If current phase is the first phase, ignore.
void SwitchHandler::backspace() {
    if (mState == STATUS) {
        mCO2TargetPending = mCO2TargetCurrent;
        xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
        mDisplayNote |= bCO2_TARGET;
        Logger::log("[STATUS] CO2 reset: %hu\n", mCO2TargetCurrent);
    } else {
        if (mNetworkStrings[mNetworkPhase].empty()) {
            if (mNetworkPhase != NEW_IP) {
                mNetworkPhase = mNetworkPhase == NEW_SSID ? NEW_IP : NEW_SSID;
                xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                mDisplayNote |= bNETWORK_PHASE;
            }
        } else {
            mNetworkStrings[mNetworkPhase].pop_back();
            Logger::log("[NETWORK] IP[%s] UN[%s] PW[%s]\n",
                        mNetworkStrings[NEW_IP].c_str(),
                        mNetworkStrings[NEW_SSID].c_str(),
                        mNetworkStrings[NEW_PW].c_str());
            vTaskDelay(1);
            mCharPending = INIT_CHAR;
            mDisplayNote |= bBACKSPACE;
        }
    }
    mPrevBackspace = time_us_64();
}

/* Character Mapping:
 * '0' (zero) is the lowest reachable character, ignoring rotations below it.
 * '~' (tilde) is the highest reachable character, ignoring rotations above it.
 * Incrementing and decrementing alphabet characters runs through upper- and lowercase characters in pairs,
 * like so: AaBbCcDdEe... and so forth.
 * General order: ['0'..'9'], '.', [Aa..Zz], [rest of the fucking owl]
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
