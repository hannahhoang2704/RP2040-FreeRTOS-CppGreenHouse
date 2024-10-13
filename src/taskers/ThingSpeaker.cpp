#include <lwip/altcp_tls.h>
#include "ThingSpeaker.h"
#include "Logger.h"
#include "Storage.h"

using namespace std;

ThingSpeaker::ThingSpeaker(const RTOS_infrastructure iRtos, const char *wifi_ssid, const char *wifi_pw,
                           const char *thingspeak_api) :
        RTOS_infra(iRtos),
        mInitSSID(wifi_ssid),
        mInitPW(wifi_pw),
        thing_speak_api(thingspeak_api){
    if (xTaskCreate(task_speak,
                    "THINGSPEAK",
                    8192,
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
                                  (void *)RTOS_infra.xThingSpeakEvent,
                                  ThingSpeaker::send_data_callback);
    mReceiveDataTimer = xTimerCreate("RECEIVE_DATA_TO_THINGSPEAK",
                                     pdMS_TO_TICKS(RECEIVE_DATA_TIMER_FREQ),
                                     pdTRUE,
                                     (void *)RTOS_infra.xThingSpeakEvent,
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

void ThingSpeaker::connect_network() {
    if (cyw43_arch_init()) {
        Logger::log("Fail to initialize\n");
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(mInitSSID, mInitPW, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        Logger::log("failed to connect\n");
    }
    Logger::log("Connected to WiFi\n");
}

void ThingSpeaker::get_data_to_send() {
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
}

void ThingSpeaker::speak() {
    Logger::log("Initiated\n");
    Logger::log("Connecting to WiFi... SSID[%s] PW[%s], API %s\n", mInitSSID, mInitPW, thing_speak_api);
    //wifi connection
    connect_network();
    Logger::log("Connected to WiFi\n");
    //start timer
    set_iRTOS(RTOS_infra);
    xTimerStart(mSendDataTimer, portMAX_DELAY);
    xTimerStart(mReceiveDataTimer, portMAX_DELAY);
    mEvents = xEventGroupGetBits(RTOS_infra.xThingSpeakEvent);
    while (true) {
        mEvents = xEventGroupWaitBits(RTOS_infra.xThingSpeakEvent, bSEND | bRECEIVE | bRECONNECT, pdTRUE, pdFALSE, portMAX_DELAY);
        if (mEvents & bSEND) {
            Logger::log("Sending data to ThingSpeak\n");
            get_data_to_send();
            char request[2048];
            snprintf(request, 2048, REQUEST_FMT, thing_speak_api,
             mCO2Target,
             mCO2Measurement,
             mPressure,
             mFan,
             mHumidity,
             mTemperature);
            Logger::log("Sending: %s\n", request);
            bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, request, TLS_CLIENT_TIMEOUT_SECS);
            if (pass) {
                Logger::log("Test passed\n");
            } else {
                Logger::log("Test failed\n");
            }
            xEventGroupClearBits(RTOS_infra.xThingSpeakEvent, bSEND);
        }else if(mEvents & bRECEIVE){
            Logger::log("Receiving data from ThingSpeak\n");
            bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, 5);
            if (pass) {
                Logger::log("Test passed\n");
            } else {
                Logger::log("Test failed\n");
            }
            xEventGroupClearBits(RTOS_infra.xThingSpeakEvent, bRECEIVE);
        }else if(mEvents & bRECONNECT){
            Logger::log("Reconnecting to WiFi\n");
            cyw43_arch_deinit();
            xTimerStop(mSendDataTimer, portMAX_DELAY);
            xTimerStop(mReceiveDataTimer, portMAX_DELAY);
            memcpy((void *) mInitSSID, RTOS_infra.qNetworkStrings[NEW_SSID], sizeof(mInitSSID));
            memcpy((void *) mInitPW, RTOS_infra.qNetworkStrings[NEW_PW], sizeof(mInitPW));
            memcpy((void *) thing_speak_api, RTOS_infra.qNetworkStrings[NEW_API], sizeof(thing_speak_api));
            connect_network();
            xTimerStart(mSendDataTimer, portMAX_DELAY);
            xTimerStart(mReceiveDataTimer, portMAX_DELAY);
            xEventGroupClearBits(RTOS_infra.xThingSpeakEvent, bRECONNECT);
        }
        vTaskDelay(50);
    }
}

void ThingSpeaker::receive_data_callback(TimerHandle_t xTimer) {
    auto mThingSpeakEvent = static_cast<EventGroupHandle_t>(pvTimerGetTimerID(xTimer));
    xEventGroupSetBits(mThingSpeakEvent, bRECEIVE);
}

void ThingSpeaker::send_data_callback(TimerHandle_t xTimer) {
    Logger::log("Sending data to ThingSpeak\n");
    auto mThingSpeakEvent = static_cast<EventGroupHandle_t>(pvTimerGetTimerID(xTimer)); // Corrected casting
    if (mThingSpeakEvent != nullptr) {
        xEventGroupSetBits(mThingSpeakEvent, bSEND);
    } else {
        Logger::log("Failed to retrieve event group handle\n");
    }
}

