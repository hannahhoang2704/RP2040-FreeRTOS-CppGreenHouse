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
    //vTaskDelay(pdMS_TO_TICKS(5000));
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
            mDisplayNote = 0;
            if (mEventData.gpio == mRotor.mPinA || mEventData.gpio == mRotor.mPinB) {
                rot_event();
            } else {
                button_event();
            }
            Display::notify(eSetBits, mDisplayNote);
        }
        if (mLostEvents) {
            Logger::log("ERROR: SwitchHandler: %u sw events lost\n", mLostEvents);
            mLostEvents = 0;
        }
    }
}

void SwitchHandler::rot_event() {
    if (mEventData.timeStamp - mPrevBackspace > BUTTON_DEBOUNCE) {
        // deduce rotation direction
        mEvent = UNKNOWN;
        if (mEventData.eventMask == GPIO_IRQ_EDGE_FALL) {
            mEvent = mEventData.gpio == mRotor.mPinA ? ROT_CLOCKWISE : ROT_COUNTER_CLOCKWISE;
        } else if (mEventData.eventMask == GPIO_IRQ_EDGE_RISE) {
            mEvent = mEventData.gpio == mRotor.mPinA ? ROT_COUNTER_CLOCKWISE : ROT_CLOCKWISE;
        }
        // execute input according to state
        if (mEvent == mPrevRotation) {
            if (mState == STATUS) {
                if (mEvent == ROT_CLOCKWISE && mCO2TargetPending < CO2_MAX) {
                    mCO2TargetPending += CO2_INCREMENT;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    mDisplayNote |= bCO2_TARGET;

                    Logger::log("SWH: Pending CO2 adjustment: +%hd => %hd\n", CO2_INCREMENT, mCO2TargetPending);
                } else if (mEvent == ROT_COUNTER_CLOCKWISE && mCO2TargetPending > CO2_MIN) {
                    mCO2TargetPending -= CO2_INCREMENT;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    mDisplayNote |= bCO2_TARGET;
                    Logger::log("SWH: Pending CO2 adjustment: -%hd => %hd\n", CO2_INCREMENT, mCO2TargetPending);
                }
            } else {
                if ((mEvent == ROT_CLOCKWISE && inc_pending_char()) ||
                    (mEvent == ROT_COUNTER_CLOCKWISE && dec_pending_char())) {
                    xQueueOverwrite(iRTOS.qCharPending, &mCharPending);
                    mDisplayNote |= bCHANGE_CHAR;
                    Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
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
                /// change system state -- status or relog
                // changing system state resets any pending user input
                mState = mState == STATUS ? NETWORK : STATUS;
                if (mState == STATUS) {
                    mCharPending = INIT_CHAR;
                    for (std::string &str: mNetworkStrings) str.clear();
                    mNetworkPhase = NEW_IP;
                    xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                    Logger::log("SWH: State: Status\n");
                    vTaskDelay(1);
                    Logger::log("SWH: CO2 set: %hu\n", mCO2TargetCurr);
                } else {
                    mCO2TargetPending = mCO2TargetCurr;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mNetworkPhase);
                    Logger::log("SWH: State: Relog\n");
                    vTaskDelay(1);
                    Logger::log("SWH: Writing: IP[" + mNetworkStrings[NEW_IP] +
                                "] UN[" + mNetworkStrings[NEW_SSID] +
                                "] PW[" + mNetworkStrings[NEW_PW] + "]\n");
                    vTaskDelay(1);
                    Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
                }
                mDisplayNote |= bSTATE;
                break;
            case SW_1:
                /// confirm rotated adjustment
                // status = CO2 target
                // relog = character to string
                if (mState == STATUS) {
                    if (mCO2TargetCurr != mCO2TargetPending) {
                        mCO2TargetCurr = mCO2TargetPending;
                        xQueueOverwrite(iRTOS.qCO2TargetCurr, &mCO2TargetCurr);
                        xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                        mDisplayNote |= bCO2_TARGET;
                        Logger::log("SWH: CO2 set: %hu\n", mCO2TargetCurr);
                    }
                } else {
                    mNetworkStrings[mNetworkPhase] += mCharPending;
                    Logger::log("SWH: Writing: IP[" + mNetworkStrings[NEW_IP] +
                                "] UN[" + mNetworkStrings[NEW_SSID] +
                                "] PW[" + mNetworkStrings[NEW_PW] + "]\n");
                    vTaskDelay(1);
                    mCharPending = INIT_CHAR;
                    Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
                    xTaskNotify(iRTOS.tDisplay, bINSERT_CHAR, eSetBits);
                    mDisplayNote |= bINSERT_CHAR;
                }
                break;
            case SW_0:
                /// confirm a series of confirmed rotated adjustments
                // applies only (?) in relog state when confirming string, moving then on to the next phase
                if (mState == NETWORK) {
                    switch (mNetworkPhase) {
                        case NEW_IP:
                            Logger::log("SWH: Writing: IP[" + mNetworkStrings[NEW_IP] +
                                        "] UN[" + mNetworkStrings[NEW_SSID] +
                                        "] PW[" + mNetworkStrings[NEW_PW] + "]\n");
                            vTaskDelay(1);
                            mNetworkPhase = NEW_SSID;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                            Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
                            break;
                        case NEW_SSID:
                            Logger::log("SWH: Writing: IP[" + mNetworkStrings[NEW_IP] +
                                        "] UN[" + mNetworkStrings[NEW_SSID] +
                                        "] PW[" + mNetworkStrings[NEW_PW] + "]\n");
                            vTaskDelay(1);
                            mNetworkPhase = NEW_PW;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                            Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
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
                break;
            case SW_ROT:
                /// backspace
                // status = resets pending user input (CO2 target) to current CO2 target
                // relog = remove last character from string corresponding to the phase
                //  - If pending string is empty, move back to previous phase. If current phase is the first phase, ignore.
                if (mState == STATUS) {
                    mCO2TargetPending = mCO2TargetCurr;
                    xQueueOverwrite(iRTOS.qCO2TargetPending, &mCO2TargetPending);
                    mDisplayNote |= bCO2_TARGET;
                    Logger::log("SWH: CO2 reset: %hu\n", mCO2TargetCurr);
                } else {
                    if (mNetworkStrings[mNetworkPhase].empty()) {
                        if (mNetworkPhase != NEW_IP) {
                            mNetworkPhase = mNetworkPhase == NEW_SSID ? NEW_IP : NEW_SSID;
                            xQueueOverwrite(iRTOS.qNetworkPhase, &mNetworkPhase);
                            mDisplayNote |= bNETWORK_PHASE;
                            Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
                        }
                    } else {
                        mNetworkStrings[mNetworkPhase].pop_back();
                        Logger::log("SWH: Writing: IP[" + mNetworkStrings[NEW_IP] +
                                    "] UN[" + mNetworkStrings[NEW_SSID] +
                                    "] PW[" + mNetworkStrings[NEW_PW] + "]\n");
                        vTaskDelay(1);
                        mCharPending = INIT_CHAR;
                        mDisplayNote |= bBACKSPACE;

                        Logger::log("SWH: Writing:" + mNetworkStrings[mNetworkPhase] + "<" + mCharPending + "\n");
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
