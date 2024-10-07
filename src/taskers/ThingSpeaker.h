#ifndef FREERTOS_GREENHOUSE_THINGSPEAKER_H
#define FREERTOS_GREENHOUSE_THINGSPEAKER_H


#include <string>
#include "FreeRTOS.h"
#include "task.h"
#include "IPStack.h"
#include "timers.h"


enum thing_speaker_commands {
    bSPEAK = 0x01,
    bRECONNECT = 0x02
};

class ThingSpeaker {
public:
    ThingSpeaker(const char *wifi_ssid = WIFI_SSID, const char *wifi_pw = WIFI_PASSWORD, const char*api=THING_SPEAK_API);

    static void notify(eNotifyAction eAction, uint32_t note);

private:
    void speak();
    static void task_speak(void *params);
    static void time_to_speak_the_thing(TimerHandle_t xTimer);
    void execute_notifications();
    void connect_http();
    void send_data();

    static TaskHandle_t mTaskHandle;
    TimerHandle_t mTimerHandle;

    static uint64_t mPrevNotification;

    const char *REQUEST_FMT = "GET /update?api_key=%s"
                              "&field1=%hd"
                              "&field2=%.1f"
                              "&field3=%.1f"
                              "&field4=%hd.%hd"
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
    std::string mInitSSID;
    std::string mInitPW;
    IPStack mIPStack;

    static const uint32_t TIMER_FREQ{5000};

    uint32_t mNotification{0};

    int16_t mCO2Target{0};
    float mCO2Measurement{0};
    float mPressure{0};
    int16_t mFan{0};
    float mHumidity{0};
    float mTemperature{0};
};


#endif //FREERTOS_GREENHOUSE_THINGSPEAKER_H
