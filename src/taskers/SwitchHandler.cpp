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

SwitchHandler::SwitchHandler(RTOS_infrastructure RTOSi) :
        mToggleState(SW_2, irq_handler),
        mInsert(SW_1, irq_handler),
        mNext(SW_0, irq_handler),
        mBackspace(SW_ROT, irq_handler),
        mRotor(ROT_A, ROT_B, irq_handler),
        iRTOS(RTOSi) {
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
    mCO2TargetCurr = 0;
    mCO2TargetPending = mCO2TargetCurr;

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
        if (mEvent == mPrevRotation && mEventData.timeStamp - mPreRotEvent > mRotDebounce_us) {
            if (mState == STATUS) {
                if (mEvent == ROT_CLOCKWISE && mCO2TargetPending < CO2_MAX) {
                    mCO2TargetPending += CO2_INCREMENT;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    xTaskNotify(iRTOS.tDisplay, bCO2_TARGET, eSetBits);
                    Logger::log("SWH: Pending CO2 adjustment: +%hd => %hd\n", CO2_INCREMENT, mCO2TargetPending);
                } else if (mEvent == ROT_COUNTER_CLOCKWISE && mCO2TargetPending > 0) {
                    mCO2TargetPending -= CO2_INCREMENT;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    xTaskNotify(iRTOS.tDisplay, bCO2_TARGET, eSetBits);
                    Logger::log("SWH: Pending CO2 adjustment: -%hd => %hd\n", CO2_INCREMENT, mCO2TargetPending);
                }
            } else {
                if ((mEvent == ROT_CLOCKWISE && inc_pending_char()) || (mEvent == ROT_COUNTER_CLOCKWISE && dec_pending_char())) {
                    xQueueOverwrite(iRTOS.qCharPending, &mCharPending);
                    xTaskNotify(iRTOS.tDisplay, bCHAR, eSetBits);
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
                }
            }
            mPreRotEvent = mEventData.timeStamp;
        }
        mPrevRotation = mEvent;
    }
}

void SwitchHandler::button_event() {
    // reset rotator
    mPrevRotation = UNKNOWN;
    // debounce button
    if (mEventData.timeStamp - mPrevEventTime[mEventData.gpio] > mPressDebounce_us) {
        mPrevEventTime[mEventData.gpio] = mEventData.timeStamp;
        switch (mEventData.gpio) {
            case SW_2:
                /// change system state -- status or relog
                // changing system state resets any pending user input
                mState = mState == STATUS ? NETWORK : STATUS;
                if (mState == STATUS) {
                    mCharPending = INIT_CHAR;
                    for (std::string &str: mRelogStrings) str.clear();
                    mRelogPhase = NEW_IP;
                    xQueueOverwrite(iRTOS.qNetworkPhase, &mRelogPhase);
                    Logger::log("SWH: State: Status\n");
                    vTaskDelay(1);
                    Logger::log("SWH: CO2 set: %hu\n", mCO2TargetCurr);
                } else {
                    mCO2TargetPending = mCO2TargetCurr;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mRelogPhase);
                    Logger::log("SWH: State: Relog\n");
                    vTaskDelay(1);
                    Logger::log("SWH: Writing: IP[" + mRelogStrings[NEW_IP] +
                                "] UN[" + mRelogStrings[NEW_SSID] +
                                "] PW[" + mRelogStrings[NEW_PW] + "]\n");
                    vTaskDelay(1);
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
                }
                xQueueOverwrite(iRTOS.qState, &mState);
                xTaskNotify(iRTOS.tDisplay, bSTATE, eSetBits);
                break;
            case SW_1:
                /// confirm rotated adjustment
                // status = CO2 target
                // relog = character to string
                if (mState == STATUS) {
                    if (mCO2TargetCurr != mCO2TargetPending) {
                        mCO2TargetCurr = mCO2TargetPending;
                        xQueueOverwrite(iRTOS.qCO2TargetCurr, &mCO2TargetCurr);
                        xTaskNotify(iRTOS.tDisplay, bCO2_TARGET, eSetBits);
                        Logger::log("SWH: CO2 set: %hu\n", mCO2TargetCurr);
                    }
                } else {
                    mRelogStrings[mRelogPhase] += mCharPending;
                    Logger::log("SWH: Writing: IP[" + mRelogStrings[NEW_IP] +
                                "] UN[" + mRelogStrings[NEW_SSID] +
                                "] PW[" + mRelogStrings[NEW_PW] + "]\n");
                    vTaskDelay(1);
                    mCharPending = INIT_CHAR;
                    Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
                    xTaskNotify(iRTOS.tDisplay, bINSERT_CHAR, eSetBits);
                }
                break;
            case SW_0:
                /// confirm a series of confirmed rotated adjustments
                // applies only (?) in relog state when confirming string, moving then on to the next phase
                if (mState == NETWORK) {
                    switch (mRelogPhase) {
                        case NEW_IP:
                            Logger::log("SWH: Writing: IP[" + mRelogStrings[NEW_IP] +
                                        "] UN[" + mRelogStrings[NEW_SSID] +
                                        "] PW[" + mRelogStrings[NEW_PW] + "]\n");
                            vTaskDelay(1);
                            mRelogPhase = NEW_SSID;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mRelogPhase);
                            Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
                            break;
                        case NEW_SSID:
                            Logger::log("SWH: Writing: IP[" + mRelogStrings[NEW_IP] +
                                        "] UN[" + mRelogStrings[NEW_SSID] +
                                        "] PW[" + mRelogStrings[NEW_PW] + "]\n");
                            vTaskDelay(1);
                            mRelogPhase = NEW_PW;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mRelogPhase);
                            Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
                            break;
                        case NEW_PW:
                            // TODO: order reconnection

                            for (std::string &str: mRelogStrings) str.clear();

                            mState = STATUS;
                            xQueueOverwrite(iRTOS.qState, &mState);
                            xTaskNotify(iRTOS.tDisplay, bSTATE, eSetBits);

                            mRelogPhase = NEW_IP;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mRelogPhase);
                            break;
                    }
                    xTaskNotify(iRTOS.tDisplay, bNETWORK_PHASE, eSetBits);
                }
                break;
            case SW_ROT:
                /// backspace
                // status = resets pending user input (CO2 target) to current CO2 target
                // relog = remove last character from string corresponding to the phase
                //  - If pending string is empty, move back to previous phase. If current phase is the first phase, ignore.
                if (mState == STATUS) {
                    mCO2TargetPending = mCO2TargetCurr;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    xTaskNotify(iRTOS.tDisplay, bCO2_TARGET, eSetBits);
                    Logger::log("SWH: CO2 reset: %hu\n", mCO2TargetCurr);
                } else {
                    if (mRelogStrings[mRelogPhase].empty()) {
                        if (mRelogPhase != NEW_IP) {
                            mRelogPhase = mRelogPhase == NEW_SSID ? NEW_IP : NEW_SSID;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mRelogPhase);
                            xTaskNotify(iRTOS.tDisplay, bNETWORK_PHASE, eSetBits);
                            Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
                        }
                    } else {
                        mRelogStrings[mRelogPhase].pop_back();
                        Logger::log("SWH: Writing: IP[" + mRelogStrings[NEW_IP] +
                                    "] UN[" + mRelogStrings[NEW_SSID] +
                                    "] PW[" + mRelogStrings[NEW_PW] + "]\n");
                        vTaskDelay(1);
                        mCharPending = INIT_CHAR;
                        xTaskNotify(iRTOS.tDisplay, bBACKSPACE, eSetBits);

                        Logger::log("SWH: Writing:" + mRelogStrings[mRelogPhase] + "<" + mCharPending + "\n");
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
    mToggleState.set_irq(state);
    mInsert.set_irq(state);
    mNext.set_irq(state);
    mBackspace.set_irq(state);
    mRotor.set_irq(state);
}
