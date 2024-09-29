#include "SwitchHandler.h"
#include "Logger.h"

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

SwitchHandler::SwitchHandler() :
        mToggleState(SW_2, irq_handler),
        mInsert(SW_1, irq_handler),
        mNext(SW_0, irq_handler),
        mBackspace(SW_ROT, irq_handler),
        mRotor(ROT_A, ROT_B, irq_handler) {
    if (xTaskCreate(task_state_handler,
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

void SwitchHandler::task_state_handler(void *params) {
    auto object_ptr = static_cast<SwitchHandler *>(params);
    object_ptr->state_handler();
}

void SwitchHandler::state_handler() {
    Logger::log("SWH: Initiated\n");
    // TODO: order connector task to (try and) establish connection to ThingSpeak
    Logger::log("SWH: Waiting for ThingSpeak connection...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
    mState = STATUS;

    // TODO: request CO2 target from EEPROM
    mCurrCO2Target = 0;
    mPendingCO2Target = mCurrCO2Target;

    // TODO: order Display to update OLED

    set_sw_irq(true);
    Logger::log("SWH: IRQ enabled\n");

    while (true) {
        while (xQueueReceive(mIRQ_eventQueue,
                             static_cast<void *>(&mEventData),
                             portMAX_DELAY) == pdTRUE) {
            if (mEventData.gpio == mRotor.mPinA || mEventData.gpio == mRotor.mPinB) {
                rot_event();
            } else {
                button_event();
            }
        }
        if (mLostEvents) {
            Logger::log("ERROR: SwitchHandler: %u sw events lost\n", mLostEvents);
            mLostEvents = 0;
        }
    }
}

void SwitchHandler::rot_event() {
    if (mEventData.timeStamp - mPrevBackspace > mPressDebounce_us) {
        // deduce rotation direction
        mEvent = UNKNOWN;
        if (mEventData.eventMask == GPIO_IRQ_EDGE_FALL) {
            mEvent = mEventData.gpio == mRotor.mPinA ? ROT_CLOCKWISE : ROT_COUNTER_CLOCKWISE;
        } else if (mEventData.eventMask == GPIO_IRQ_EDGE_RISE) {
            mEvent = mEventData.gpio == mRotor.mPinA ? ROT_COUNTER_CLOCKWISE : ROT_CLOCKWISE;
        }
        // execute input according to state
        if (mState == STATUS) {
            if (mEvent == ROT_CLOCKWISE) {
                mPendingCO2Target += CO2_INCREMENT;
                Logger::log("SWH: Pending CO2 adjustment: +%hd => %hd\n", CO2_INCREMENT, mPendingCO2Target);
            } else if (mEvent == ROT_COUNTER_CLOCKWISE) {
                mPendingCO2Target -= CO2_INCREMENT;
                Logger::log("SWH: Pending CO2 adjustment: -%hd => %hd\n", CO2_INCREMENT, mPendingCO2Target);
            }
        } else {
            if (mEvent == ROT_CLOCKWISE) {
                if (inc_pending_char())
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
            } else if (mEvent == ROT_COUNTER_CLOCKWISE) {
                if (dec_pending_char())
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
            }
        }
    }
}

void SwitchHandler::button_event() {
    // debounce button
    if (mEventData.timeStamp - mPrevEventTime[mEventData.gpio] > mPressDebounce_us) {
        mPrevEventTime[mEventData.gpio] = mEventData.timeStamp;
        switch (mEventData.gpio) {
            case SW_2:
                /// change system state -- status or relog
                // changing system state resets any pending user input
                mState = mState == STATUS ? RELOG : STATUS;
                if (mState == STATUS) {
                    mPendingChar = INIT_CHAR;
                    for (std::string &str: mRelogStrings) str.clear();
                    mRelogPhase = IP;
                    Logger::log("SWH: State: Status\n");
                    vTaskDelay(1);
                    Logger::log("SWH: CO2 set: %hu\n", mCurrCO2Target);
                } else {
                    mPendingCO2Target = mCurrCO2Target;
                    Logger::log("SWH: State: Relog\n");
                    vTaskDelay(1);
                    Logger::log("SWH: Writing: IP[" + mRelogStrings[IP] +
                                "] UN[" + mRelogStrings[USERNAME] +
                                "] PW[" + mRelogStrings[PASSWORD] + "]\n");
                    vTaskDelay(1);
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
                }
                break;
            case SW_1:
                /// confirm rotated adjustment
                // status = CO2 target
                // relog = character to string
                if (mState == STATUS) {
                    if (mCurrCO2Target != mPendingCO2Target) {
                        mCurrCO2Target = mPendingCO2Target;
                        Logger::log("SWH: CO2 set: %hu\n", mCurrCO2Target);
                    }
                } else {
                    mRelogStrings[mRelogPhase] += mPendingChar;
                    Logger::log("SWH: Writing: IP[" + mRelogStrings[IP] +
                                "] UN[" + mRelogStrings[USERNAME] +
                                "] PW[" + mRelogStrings[PASSWORD] + "]\n");
                    vTaskDelay(1);
                    mPendingChar = INIT_CHAR;
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
                }
                break;
            case SW_0:
                /// confirm a series of confirmed rotated adjustments
                // applies only (?) in relog state when confirming string, moving then on to the next phase
                if (mState == RELOG) {
                    switch (mRelogPhase) {
                        case IP:
                            Logger::log("SWH: Writing: IP[" + mRelogStrings[IP] +
                                        "] UN[" + mRelogStrings[USERNAME] +
                                        "] PW[" + mRelogStrings[PASSWORD] + "]\n");
                            vTaskDelay(1);
                            mRelogPhase = USERNAME;
                            Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
                            break;
                        case USERNAME:
                            Logger::log("SWH: Writing: IP[" + mRelogStrings[IP] +
                                        "] UN[" + mRelogStrings[USERNAME] +
                                        "] PW[" + mRelogStrings[PASSWORD] + "]\n");
                            vTaskDelay(1);
                            mRelogPhase = PASSWORD;
                            Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
                            break;
                        case PASSWORD:
                            // TODO: initiate reconnection sequence

                            set_sw_irq(false);
                            mState = CONNECTING;

                            // TODO: keep Display up to date with reconnection phases

                            Logger::log("SWH: Reconnecting...: IP[" + mRelogStrings[IP] +
                                        "] UN[" + mRelogStrings[USERNAME] +
                                        "] PW[" + mRelogStrings[PASSWORD] + "]\n");
                            vTaskDelay(pdMS_TO_TICKS(5000));
                            mRelogPhase = IP;
                            mState = STATUS;
                            set_sw_irq(true);
                            Logger::log("SWH: Reconnection attempt finished\n");

                            for (std::string &str: mRelogStrings) str.clear();
                            break;
                    }
                }
                break;
            case SW_ROT:
                /// backspace
                // status = resets pending user input (CO2 target) to current CO2 target
                // relog = remove last character from string corresponding to the phase
                //  - If pending string is empty, move back to previous phase. If current phase is the first phase, ignore.
                if (mState == STATUS) {
                    mPendingCO2Target = mCurrCO2Target;
                    Logger::log("SWH: CO2 reset: %hu\n", mCurrCO2Target);
                } else {
                    if (mRelogStrings[mRelogPhase].empty()) {
                        if (mRelogPhase != IP) {
                            mRelogPhase = mRelogPhase == USERNAME ? IP : USERNAME;
                            Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
                        }
                    } else {
                        mRelogStrings[mRelogPhase].pop_back();
                        Logger::log("SWH: Writing: IP[" + mRelogStrings[IP] +
                                    "] UN[" + mRelogStrings[USERNAME] +
                                    "] PW[" + mRelogStrings[PASSWORD] + "]\n");
                        vTaskDelay(1);
                        mPendingChar = INIT_CHAR;
                        Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mPendingChar + "\n");
                    }
                }
                mPrevBackspace = time_us_64();
                break;
            default:
                Logger::log("ERROR: SwitchHandler: who dis gpio %u\n", mEventData.gpio);
        }
        // TODO: order Display to update OLED
    }
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
    if (isupper(mPendingChar)) {
        mPendingChar = tolower(mPendingChar);
    } else if (islower(mPendingChar) && mPendingChar != 'z') {
        mPendingChar = toupper(mPendingChar) + 1;
    } else
        switch (mPendingChar) {
            case '9':
                mPendingChar = '.';
                break;
            case '.':
                mPendingChar = 'A';
                break;
            case 'z':
                mPendingChar = '!';
                break;
            case '-':
                mPendingChar = '/';
                break;
            case '/':
                mPendingChar = ':';
                break;
            case '@':
                mPendingChar = '[';
                break;
            case '`':
                mPendingChar = '{';
                break;
            case '~':
                return false;
            default:
                ++mPendingChar;
                break;
        }
    return true;
}

/// decrement pending character with a modified lexicography
bool SwitchHandler::dec_pending_char() {
    if (islower(mPendingChar)) {
        mPendingChar = toupper(mPendingChar);
    } else if (isupper(mPendingChar) && mPendingChar != 'A') {
        mPendingChar = tolower(mPendingChar) - 1;
    } else
        switch (mPendingChar) {
            case '0':
                return false;
            case '.':
                mPendingChar = '9';
                break;
            case 'A':
                mPendingChar = '.';
                break;
            case '!':
                mPendingChar = 'z';
                break;
            case '/':
                mPendingChar = '-';
                break;
            case ':':
                mPendingChar = '/';
                break;
            case '[':
                mPendingChar = '@';
                break;
            case '{':
                mPendingChar = '`';
                break;
            default:
                --mPendingChar;
                break;
        }
    return true;
}

void SwitchHandler::set_sw_irq(bool state) const {
    mToggleState.set_irq(state);
    mInsert.set_irq(state);
    mNext.set_irq(state);
    mBackspace.set_irq(state);
    mRotor.set_irq(state);
}
