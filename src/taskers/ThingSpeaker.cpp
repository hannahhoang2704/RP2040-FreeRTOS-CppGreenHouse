#include "ThingSpeaker.h"
#include "Logger.h"

using namespace std;

uint64_t ThingSpeaker::mPrevNotification{0};
TaskHandle_t ThingSpeaker::mTaskHandle{nullptr};

void ThingSpeaker::notify(eNotifyAction eAction, uint32_t note) {
    if (time_us_64() - mPrevNotification > 35000) {
        mPrevNotification = time_us_64();
        xTaskNotify(mTaskHandle, note, eAction);
    }
}

ThingSpeaker::ThingSpeaker(const char *wifi_ssid, const char *wifi_pw) :
        mInitSSID(wifi_ssid),
        mInitPW(wifi_pw),
        mIPStack()
        {
    if (xTaskCreate(task_speak,
                    "THING_SPEAKER",
                    1024,
                    (void *) this,
                    tskIDLE_PRIORITY + 2,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created THING_SPEAKER task\n");
    } else {
        Logger::log("Failed to create THING_SPEAKER task\n");
    }
    mTimerHandle = xTimerCreate("TIME_TO_SPEAK",
                                pdMS_TO_TICKS(TIMER_FREQ),
                                pdTRUE,
                                (void *) nullptr,
                                ThingSpeaker::time_to_speak_the_thing);
    if (mTimerHandle != nullptr) {
        Logger::log("Created TIME_TO_SPEAK timer\n");
    } else {
        Logger::log("Failed to create TIME_TO_SPEAK task\n");
    }
}

void ThingSpeaker::task_speak(void *params) {
    auto object_ptr = static_cast<ThingSpeaker *>(params);
    object_ptr->speak();
}

void ThingSpeaker::speak() {
    Logger::log("Initiated\n");
    mIPStack.connect_wifi(mInitSSID.c_str(), mInitPW.c_str());
    xTimerStart(mTimerHandle, portMAX_DELAY);
    while (true) {
        mNotification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Logger::log("Note: %u\n", mNotification);
        execute_notifications();
        ++mCO2Target;
        mCO2Measurement += 3.7;
        mPressure += 1.2;
        mFan += 5;
        mFan %= 1000;
        mHumidity += 4.1;
        mTemperature += 0.3;
    }
}

void ThingSpeaker::time_to_speak_the_thing(TimerHandle_t xTimer) {
    ThingSpeaker::notify(eSetBits, bSPEAK);
}

void ThingSpeaker::execute_notifications() {
    if (mNotification & bRECONNECT) {
        char newAPI[32];
        char newSSID[32];
        char newPW[32];
        mAPI = newAPI;
        // get new network credentials from queues
        if (0) {
            Logger::log("Reconnecting to WiFi... SSID[%s] PW[%s]\n", newSSID, newPW);
            mIPStack.disconnect();
            mIPStack = IPStack(newSSID, newPW);
            connect_http();
        }
    }
    if (mNotification & bSPEAK) {
        // peek latest date from queues
        send_data();
    }
}

void ThingSpeaker::connect_http() {
    mIPStack.disconnect();
    if ((mConnectionError = mIPStack.connect(HTTP_SERVER, PORT)) == ERR_OK) {
        Logger::log("Connected to %s:%d!\n", HTTP_SERVER, PORT);
    } else {
        Logger::log("ERROR: Failed to connect to %s:%d. Error nr: [%d]\n",
                    HTTP_SERVER, PORT, mConnectionError);
    }

}

void ThingSpeaker::send_data() {
    connect_http();
    char request[2048];
    snprintf(request, 2048, REQUEST_FMT, THING_SPEAK_API, mAPI.c_str(),
             mCO2Target,
             mCO2Measurement,
             mPressure,
             mFan / 10, mFan % 10,
             mHumidity,
             mTemperature);
    Logger::log("Sending: %s\n", request);
    mIPStack.write((unsigned char *) (request), strlen(request), 1000);
    int rv = mIPStack.read(rBuffer, 2048, 1000);
    rBuffer[rv] = 0;
    Logger::log("Response: %s", rBuffer);
}
