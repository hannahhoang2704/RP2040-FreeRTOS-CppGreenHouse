#ifndef FREERTOS_GREENHOUSE_THINGSPEAKER_H
#define FREERTOS_GREENHOUSE_THINGSPEAKER_H


#include <string>
#include "FreeRTOS.h"
#include "task.h"
#include "IPStack.h"
#include "timers.h"
#include "tls_common.c"
#include <mbedtls/debug.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "RTOS_infrastructure.h"
#define TLS_CLIENT_SERVER        "api.thingspeak.com"
#define TLS_CLIENT_HTTP_REQUEST  "POST talkbacks/53302/commands/execute.json?api_key=LGSSKNARG3LPLX6R\r\n" \
                                 "Host: " TLS_CLIENT_SERVER "\r\n" \
                                 "Connection: close\r\n" \
                                 "\r\n"
#define TLS_CLIENT_TIMEOUT_SECS  15

extern "C" {
#include "tls_common.h"
}


class ThingSpeaker {
public:
    ThingSpeaker(RTOS_infrastructure iRTOS, const char *wifi_ssid = WIFI_SSID, const char *wifi_pw = WIFI_PASSWORD, const char *thingspeak_api = THING_SPEAK_API);

    static void notify(eNotifyAction eAction, uint32_t note);

private:
    void speak();
    static void task_speak(void *params);
    static void send_data_callback(TimerHandle_t xTimer);
    static void receive_data_callback(TimerHandle_t xTimer);
    void execute_notifications();
    void connect_http();
    void send_data();

    static TaskHandle_t mTaskHandle;
    TimerHandle_t mSendDataTimer;
    TimerHandle_t mReceiveDataTimer;
    RTOS_infrastructure RTOS_infra;
    static uint64_t mPrevNotification;

    const char *REQUEST_FMT = "GET /update?api_key=%s"
                              "&field1=%hd"
                              "&field2=%.1f"
                              "&field3=%.1f"
                              "&field4=%hd"
                              "&field5=%.1f"
                              "&field6=%.1f"
                              " HTTP/1.1\r\n"
                              "Host: api.thingspeak.com\r\n"
                              "\r\n";
    const char *HTTP_SERVER{"3.224.58.169"};
    const int PORT{80};
    const int CONNECTION_TIMEOUT_MS{1000};
    unsigned char rBuffer[2048];
    int mConnectionError{0};
    std::string mAPI;
    const char *mInitSSID;
    const char *mInitPW;
    const char *thing_speak_api;
    IPStack mIPStack;

    static const uint32_t SEND_DATA_TIMER_FREQ{15000};
    static const uint32_t RECEIVE_DATA_TIMER_FREQ{5000};

    uint32_t mNotification{0};

    int16_t mCO2Target{0};
    float mCO2Measurement{0};
    float mPressure{0};
    int16_t mFan{0};
    float mHumidity{0};
    float mTemperature{0};

    TLS_CLIENT_T_ *mTLSClient;
};


#endif //FREERTOS_GREENHOUSE_THINGSPEAKER_H
