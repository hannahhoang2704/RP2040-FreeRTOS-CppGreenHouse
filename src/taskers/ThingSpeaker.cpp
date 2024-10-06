#include <lwip/altcp_tls.h>
#include "ThingSpeaker.h"
#include "Logger.h"

using namespace std;

uint64_t ThingSpeaker::mPrevNotification{0};
TaskHandle_t ThingSpeaker::mTaskHandle{nullptr};
//const char *ThingSpeaker::REQUEST_FMT;

//void ThingSpeaker::notify(eNotifyAction eAction, uint32_t note) {
//    if (time_us_64() - mPrevNotification > 35000) {
//        mPrevNotification = time_us_64();
//        xTaskNotify(mTaskHandle, note, eAction);
//    }
//}

ThingSpeaker::ThingSpeaker(RTOS_infrastructure iRTOS, const char *wifi_ssid, const char *wifi_pw,
                           const char *thingspeak_api) :
        RTOS_infra(iRTOS),
        mInitSSID(wifi_ssid),
        mInitPW(wifi_pw),
        thing_speak_api(thingspeak_api),
        mIPStack() {
    if (xTaskCreate(task_speak,
                    "THINGSPEAK",
                    1024,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created THING_SPEAKER task\n");
    } else {
        Logger::log("Failed to create THING_SPEAKER task\n");
    }
    mSendDataTimer = xTimerCreate("SEND_DATA_TO_THINGSPEAK",
                                  pdMS_TO_TICKS(SEND_DATA_TIMER_FREQ),
                                  pdTRUE,
                                  (void *) iRTOS.xThingSpeakEvent,
                                  ThingSpeaker::send_data_callback);
    mReceiveDataTimer = xTimerCreate("RECEIVE_DATA_TO_THINGSPEAK",
                                     pdMS_TO_TICKS(RECEIVE_DATA_TIMER_FREQ),
                                     pdTRUE,
                                     (void *) iRTOS.xThingSpeakEvent,
                                     ThingSpeaker::receive_data_callback);
    if (mSendDataTimer != nullptr) {
        Logger::log("Created SEND_DATA_TO_THINGSPEAK timer\n");
    } else {
        Logger::log("Failed to create SEND_DATA_TO_THINGSPEAK task\n");
    }
    if (mReceiveDataTimer != nullptr) {
        Logger::log("Created RECEIVE_DATA_TO_THINGSPEAK timer\n");
    } else {
        Logger::log("Failed to create RECEIVE_DATA_TO_THINGSPEAK task\n");
    }
}

void ThingSpeaker::task_speak(void *params) {
    auto object_ptr = static_cast<ThingSpeaker *>(params);
    object_ptr->speak();
}

void ThingSpeaker::speak() {
    Logger::log("Initiated\n");
//    xTimerStop(mSendDataTimer, portMAX_DELAY); //this should be when reconnect the wifi network
//    xTimerStop(mReceiveDataTimer, portMAX_DELAY);
    //wifi connection
    if (cyw43_arch_init()) {
        Logger::log("Fail to initialize\n");
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(mInitSSID, mInitPW, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        Logger::log("failed to connect\n");
    }

    //start timer
    xTimerStart(mSendDataTimer, portMAX_DELAY);
    //xTimerStart(mReceiveDataTimer, portMAX_DELAY);printf("connecting to server IP %s port %d\n", ipaddr_ntoa(ipaddr), port);
    set_iRTOS(RTOS_infra);
    if (!init_tls(mTLSClient, TLS_CLIENT_HTTP_REQUEST, TLS_CLIENT_TIMEOUT_SECS, TLS_CLIENT_SERVER)) {
        Logger::log("Failed to initialize TLS Client\n");
    }

    while (true) {
        /*
        bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, TLS_CLIENT_TIMEOUT_SECS);
        if (pass) {
            Logger::log("Test passed\n");
        } else {
            Logger::log("Test failed\n");
        }
        */
        mEvents = xEventGroupGetBits(RTOS_infra.xThingSpeakEvent);
        xEventGroupClearBits(RTOS_infra.xThingSpeakEvent, bSEND | bRECEIVE | bRECONNECT);
        if (mEvents & bSEND) {

            if (xQueuePeek(RTOS_infra.qCO2TargetCurrent, &mCO2Target, 0) == pdTRUE) {
                Logger::log("CO2 Target: %hd\n", mCO2Target);
            }
            if (xQueuePeek(RTOS_infra.qCO2Measurement, &mCO2Measurement, 0) == pdTRUE) {
                Logger::log("CO2 measurement: %f\n", mCO2Measurement);
            }
            if (xQueuePeek(RTOS_infra.qPressure, &mPressure, 0) == pdTRUE) {
                Logger::log("Pressure: %f\n", mPressure);
            }
            if (xQueuePeek(RTOS_infra.qFan, &mFan, 0) == pdTRUE) {
                Logger::log("Fan: %hd\n", mFan);
            }
            if (xQueuePeek(RTOS_infra.qHumidity, &mHumidity, 0) == pdTRUE) {
                Logger::log("Humid: %f\n", mHumidity);
            }
            if (xQueuePeek(RTOS_infra.qTemperature, &mTemperature, 0) == pdTRUE) {
                Logger::log("Temp: %f\n", mTemperature);
            }
            char request[2048];
            snprintf(request, 2048, REQUEST_FMT, thing_speak_api,
                     mCO2Target,
                     mCO2Measurement,
                     mPressure,
                     mFan,
                     mHumidity,
                     mTemperature);
            mTLSClient->http_request = request;
            Logger::log("Sending: %s\n", request);
            vTaskDelay(1000);
            mErr = send(mTLSClient, TLS_CLIENT_SERVER);
            if (mErr != ERR_OK) {
                Logger::log("error writing data err=%d\n", mErr);
            }
        }
        if (mEvents & bRECEIVE) {

        }
        if (mEvents & bRECONNECT) {

        }
    }

//
//    mIPStack.connect_wifi(mInitSSID.c_str(), mInitPW.c_str());
//    xTimerStart(mSendDataTimer, portMAX_DELAY);
//    while (true) {
//        mNotification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//        Logger::log("Note: %u\n", mNotification);
//        execute_notifications();
//        ++mCO2Target;
//        mCO2Measurement += 3.7;
//        mPressure += 1.2;
//        mFan += 5;
//        mFan %= 1000;
//        mHumidity += 4.1;
//        mTemperature += 0.3;
//    }
}

void ThingSpeaker::send_data_callback(TimerHandle_t xTimer) {
    auto xEventGroup = static_cast<EventGroupHandle_t>(pvTimerGetTimerID(xTimer));
    xEventGroupSetBits(xEventGroup, bSEND);
}

void ThingSpeaker::receive_data_callback(TimerHandle_t xTimer) {
    auto xEventGroup = static_cast<EventGroupHandle_t>(pvTimerGetTimerID(xTimer));
    xEventGroupSetBits(xEventGroup, bRECEIVE);
}

//void ThingSpeaker::send_data_callback(TimerHandle_t xTimer) {
//    auto iRTOS = static_cast<RTOS_infrastructure *>(pvTimerGetTimerID(xTimer));
//    int16_t qCO2TargetCurrent;
//    float mCO2Measurement{0};
//    float mPressure{0};
//    int16_t mFan{0};
//    float mHumidity{0};
//    float mTemperature{0};
//
//    if (xQueuePeek(iRTOS->qCO2TargetCurrent, &qCO2TargetCurrent, 0) == pdTRUE) {
//        Logger::log("CO2 Target: %hd\n", qCO2TargetCurrent);
//    }
//    if (xQueuePeek(iRTOS->qCO2Measurement, &mCO2Measurement, 0) == pdTRUE) {
//        Logger::log("CO2 measurement: %f\n", mCO2Measurement);
//    }
//    if (xQueuePeek(iRTOS->qPressure, &mPressure, 0) == pdTRUE) {
//        Logger::log("Pressure: %f\n", mPressure);
//    }
//    if (xQueuePeek(iRTOS->qFan, &mFan, 0) == pdTRUE) {
//        Logger::log("Fan: %hd\n", mFan);
//    }
//    if (xQueuePeek(iRTOS->qHumidity, &mHumidity, 0) == pdTRUE) {
//        Logger::log("Humid: %f\n", mHumidity);
//    }
//    if (xQueuePeek(iRTOS->qTemperature, &mTemperature, 0) == pdTRUE) {
//        Logger::log("Temp: %f\n", mTemperature);
//    }
//    char request[2048];
//    const char *REQUEST_FMT = "GET /update?api_key=%s"
//                              "&field1=%hd"
//                              "&field2=%.1f"
//                              "&field3=%.1f"
//                              "&field4=%hd"
//                              "&field5=%.1f"
//                              "&field6=%.1f"
//                              " HTTP/1.1\r\n"
//                              "Host: api.thingspeak.com\r\n"
//                              "\r\n";
//    snprintf(request, 2048, REQUEST_FMT, THING_SPEAK_API,
//             qCO2TargetCurrent,
//             mCO2Measurement,
//             mPressure,
//             mFan,
//             mHumidity,
//             mTemperature);
//    Logger::log("Sending: %s\n", request);

//
//}

//void ThingSpeaker::execute_notifications() {
//    if (mNotification & bRECONNECT) {
//        char newAPI[32];
//        char newSSID[32];
//        char newPW[32];
//        mAPI = newAPI;
//        // get new network credentials from queues
//        if (0) {
//            Logger::log("Reconnecting to WiFi... SSID[%s] PW[%s]\n", newSSID, newPW);
//            mIPStack.disconnect();
//            mIPStack = IPStack(newSSID, newPW);
//            connect_http();
//        }
//    }
//    if (mNotification & bSPEAK) {
//        // peek latest date from queues
//        send_data();
//    }
//}
//
//void ThingSpeaker::connect_http() {
//    mIPStack.disconnect();
//    if ((mConnectionError = mIPStack.connect(HTTP_SERVER, PORT)) == ERR_OK) {
//        Logger::log("Connected to %s:%d!\n", HTTP_SERVER, PORT);
//    } else {
//        Logger::log("ERROR: Failed to connect to %s:%d. Error nr: [%d]\n",
//                    HTTP_SERVER, PORT, mConnectionError);
//    }
//
//}
//
//void ThingSpeaker::send_data() {
//    connect_http();
//    char request[2048];
//    snprintf(request, 2048, REQUEST_FMT, THING_SPEAK_API, mAPI.c_str(),
//             mCO2Target,
//             mCO2Measurement,
//             mPressure,
//             mFan / 10, mFan % 10,
//             mHumidity,
//             mTemperature);
//    Logger::log("Sending: %s\n", request);
//    mIPStack.write((unsigned char *) (request), strlen(request), 1000);
//    int rv = mIPStack.read(rBuffer, 2048, 1000);
//    rBuffer[rv] = 0;
//    Logger::log("Response: %s", rBuffer);
//}
