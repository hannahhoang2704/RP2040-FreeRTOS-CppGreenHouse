#ifndef FREERTOS_GREENHOUSE_THINGSPEAKER_H
#define FREERTOS_GREENHOUSE_THINGSPEAKER_H


#include <string>
#include "FreeRTOS.h"
#include "task.h"
#include "IPStack.h"
#include "timers.h"
#include <mbedtls/debug.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "RTOS_infrastructure.h"


extern "C" {
#include "tls_common.h"
#include "lwip/altcp.h"
}


class ThingSpeaker {
public:
    ThingSpeaker(const RTOS_infrastructure iRtos, char *wifi_ssid = WIFI_SSID, char *wifi_pw = WIFI_PASSWORD, char *thingspeak_api = THING_SPEAK_API);

private:
    void speak();
    static void task_speak(void *params);
    static void send_data_callback(TimerHandle_t xTimer);
    static void receive_data_callback(TimerHandle_t xTimer);

    TaskHandle_t mTaskHandle;
    TimerHandle_t mSendDataTimer;
    TimerHandle_t mReceiveDataTimer;
    RTOS_infrastructure RTOS_infra;

    const char *HTTP_THINGSPEAK_REQUEST = "GET /update?api_key=%s"
                                          "&field1=%hd"
                                          "&field2=%.1f"
                                          "&field3=%.1f"
                                          "&field4=%hd"
                                          "&field5=%.1f"
                                          "&field6=%.1f"
                                          " HTTP/1.1\r\n"
                                          "Host: api.thingspeak.com\r\n"
                                          "\r\n";
    const char *HTTP_TALKBACK_REQUEST = "GET /talkbacks/53302/commands/execute.json?api_key=LGSSKNARG3LPLX6R HTTP/1.1\r\n" \
                                        "Host: api.thingspeak.com\r\n" \
                                        "Connection: close\r\n" \
                                        "\r\n";
    const char *TLS_CLIENT_SERVER = "api.thingspeak.com";
    const int TLS_CLIENT_TIMEOUT_SECS = 15;
    bool connect_network();
    void get_data_to_send();
    char mInitSSID[MAX_STRING_LEN];
    char mInitPW[MAX_STRING_LEN];
    char thing_speak_api[MAX_STRING_LEN];

    static const uint32_t SEND_DATA_TIMER_FREQ{15000};
    static const uint32_t RECEIVE_DATA_TIMER_FREQ{5000};
    bool wifi_connected;
    EventBits_t mEvents;

    int16_t mCO2Target{0};
    float mCO2Measurement{0};
    float mPressure{0};
    int16_t mFan{0};
    float mHumidity{0};
    float mTemperature{0};
};


#endif //FREERTOS_GREENHOUSE_THINGSPEAKER_H
