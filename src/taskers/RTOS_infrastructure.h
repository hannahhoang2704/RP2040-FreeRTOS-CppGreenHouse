#ifndef FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
#define FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H

#include "FreeRTOS.h"
#include "queue.h"

// RTOS task notifications require breathing room
// OLED gets crazy if events are sent too frequently -- i.e. with Rotor
static const uint64_t TASK_NOTIFICATION_RATE_LIMIT_US{35000};

const char INIT_CHAR{'.'};

enum program_state {
    STATUS,
    NETWORK
};

enum network_phase {
    NEW_IP,
    NEW_SSID,
    NEW_PW
};

enum character_action {
    bNONE,
    bCHAR_INSERT,
    bCHAR_BACKSPACE
};

enum connection_state {
    bCONNECTING,
    bCONNECTED,
    bNOT_CONNECTED
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
    QueueHandle_t qCharAction{nullptr};
    QueueHandle_t qConnectionState{nullptr};

    SemaphoreHandle_t sUpdateGreenhouse{nullptr};
    SemaphoreHandle_t sUpdateDisplay{nullptr};
};

#endif //FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
