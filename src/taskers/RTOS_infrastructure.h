#ifndef FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
#define FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H

#include "FreeRTOS.h"
#include "queue.h"

const int16_t CO2_MAX{1500};
const int16_t CO2_MIN{0};
const char INIT_CHAR{'.'};
const uint MAX_CREDENTIAL_STRING_LEN{61};

enum program_state {
    STATUS,
    NETWORK
};

enum network_phase {
    NEW_API,
    NEW_SSID,
    NEW_PW
};

enum connection_state {
    bCONNECTING,
    bCONNECTED,
    bNOT_CONNECTED
};

enum storage_data {
    CO2_target,
    API_str,
    SSID_str,
    PW_str
};

struct RTOS_infrastructure {
    QueueHandle_t qState{nullptr};
    QueueHandle_t qNetworkPhase{nullptr};
    QueueHandle_t qCO2TargetPending{nullptr};
    QueueHandle_t qCO2TargetCurrent{nullptr};
    QueueHandle_t qCO2Measurement{nullptr};
    QueueHandle_t qPressure{nullptr};
    QueueHandle_t qFan{nullptr};
    QueueHandle_t qHumidity{nullptr};
    QueueHandle_t qTemperature{nullptr};
    QueueHandle_t qCharPending{nullptr};
    QueueHandle_t qConnectionState{nullptr};
    QueueHandle_t qNetworkStrings[3] {[NEW_API] = nullptr,
                                      [NEW_SSID] = nullptr,
                                      [NEW_PW] = nullptr};
    QueueHandle_t qStorageQueue{nullptr};

    SemaphoreHandle_t sUpdateGreenhouse{nullptr};
    SemaphoreHandle_t sUpdateDisplay{nullptr};
};

#endif //FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
