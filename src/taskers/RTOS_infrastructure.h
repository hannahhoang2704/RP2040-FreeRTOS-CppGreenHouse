#ifndef FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
#define FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H

#include "FreeRTOS.h"

enum program_state {
    STATUS,
    RELOG,
    CONNECTING
};

enum network_phase {
    IP,
    USERNAME,
    PASSWORD
};

struct RTOS_infrastructure {
    QueueHandle_t qProgramState;
    QueueHandle_t qNetworkPhase;
    QueueHandle_t qCO2TargetPending;
    QueueHandle_t qCO2TargetCurr;
    QueueHandle_t qCharPending;
    QueueHandle_t qNetworkStrings[3];

    SemaphoreHandle_t sUpdateDisplay;
};

#endif //FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
