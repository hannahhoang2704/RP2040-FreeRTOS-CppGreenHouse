#include <lwip/altcp_tls.h>
#include "ThingSpeaker.h"
#include "Logger.h"
#include "Storage.h"

using namespace std;

ThingSpeaker::ThingSpeaker(const RTOS_infrastructure iRtos, const char *wifi_ssid, const char *wifi_pw,
                           const char *thingspeak_api) :
//ThingSpeaker::ThingSpeaker( const char *wifi_ssid, const char *wifi_pw, const char *thingspeak_api) :
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
                                  (void *)& iRtos,
                                  ThingSpeaker::send_data_callback);
    mReceiveDataTimer = xTimerCreate("RECEIVE_DATA_TO_THINGSPEAK",
                                     pdMS_TO_TICKS(RECEIVE_DATA_TIMER_FREQ),
                                     pdTRUE,
                                     (void *) &iRtos,
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

void ThingSpeaker::speak() {
    Logger::log("Initiated\n");
    Logger::log("Connecting to WiFi... SSID[%s] PW[%s], API %s\n", mInitSSID, mInitPW, thing_speak_api);
    //wifi connection
    connect_network();
    Logger::log("Connected to WiFi\n");
//    xSemaphoreGive(RTOS_infra.sWifiConnected);
//    xTaskCreate(task_storage, "Storage", 256, (void *) this,
//                tskIDLE_PRIORITY + 1, &mTaskHandle);
//    if(mTaskHandle != nullptr){
//        Logger::log("Created STORAGE task.\n");
//    } else{
//        Logger::log("Failed to create STORAGE task.\n");
//    }
    //start timer
    set_iRTOS(RTOS_infra);
    xTimerStart(mSendDataTimer, portMAX_DELAY);
    xTimerStart(mReceiveDataTimer, portMAX_DELAY);

    while (true) {

        vTaskDelay(100);
    }
}

void ThingSpeaker::receive_data_callback(TimerHandle_t xTimer) {
//    auto iRTOS = static_cast<RTOS_infrastructure *>(pvTimerGetTimerID(xTimer));;
    Logger::log("Receiving data from ThingSpeak\n");
    bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, 5);
    if (pass) {
        Logger::log("Test passed\n");
    } else {
        Logger::log("Test failed\n");
    }
}

void ThingSpeaker::send_data_callback(TimerHandle_t xTimer) {
    Logger::log("Sending data to ThingSpeak\n");
    auto iRTOS = static_cast<RTOS_infrastructure *>(pvTimerGetTimerID(xTimer));
    int16_t qCO2TargetCurrent=2000;
    float mCO2Measurement=120.4;
    float mPressure=12.5;
    int16_t mFan=10;
    float mHumidity=20.2;
    float mTemperature=24.4;
    Logger::log("Sending data to ThingSpeak\n");
    if (xQueuePeek(iRTOS->qCO2TargetCurrent, &qCO2TargetCurrent, 0) == pdTRUE) {
        Logger::log("CO2 Target: %hd\n", qCO2TargetCurrent);
    }
    if (xQueuePeek(iRTOS->qCO2Measurement, &mCO2Measurement, 0) == pdTRUE) {
        Logger::log("CO2 measurement: %f\n", mCO2Measurement);
    }
    if (xQueuePeek(iRTOS->qPressure, &mPressure, 0) == pdTRUE) {
        Logger::log("Pressure: %f\n", mPressure);
    }
    if (xQueuePeek(iRTOS->qFan, &mFan, 0) == pdTRUE) {
        Logger::log("Fan: %hd\n", mFan);
    }
    if (xQueuePeek(iRTOS->qHumidity, &mHumidity, 0) == pdTRUE) {
        Logger::log("Humid: %f\n", mHumidity);
    }
    if (xQueuePeek(iRTOS->qTemperature, &mTemperature, 0) == pdTRUE) {
        Logger::log("Temp: %f\n", mTemperature);
    }
    char request[2048];
    const char *REQUEST_FMT = "GET /update?api_key=%s&field1=%hd&field2=%.1f&field3=%.1f&field4=%hd&field5=%.1f&field6=%.1f HTTP/1.1\r\n" \
                                "Host: api.thingspeak.com\r\n" \
                                "Connection: close\r\n" \
                                "\r\n";
    snprintf(request, 2048, REQUEST_FMT, THING_SPEAK_API,
             qCO2TargetCurrent,
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
}

