#ifndef FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
#define FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H

#include "FreeRTOS.h"
#include "queue.h"

// RTOS task notifications require breathing room
// OLED gets crazy if events are sent too frequently -- i.e. with Rotor
static const uint64_t TASK_NOTIFICATION_RATE_LIMIT_US{35000};

const char INIT_CHAR{'.'};
const int16_t CO2_INCREMENT{1};

enum program_state {
    STATUS,
    NETWORK
};

enum network_phase {
    NEW_IP,
    NEW_SSID,
    NEW_PW
};

enum display_notifications {
    bSTATE          = 0x0001,
    bCO2_TARGET     = 0x0002,
    bCO2_MEASURE    = 0x0004,
    bPRESSURE       = 0x0008,
    bFAN            = 0x0010,
    bHUMIDITY       = 0x0020,
    bTEMPERATURE    = 0x0040,
    bNETWORK_PHASE  = 0x0080,
    bCHANGE_CHAR    = 0x0100,
    bINSERT_CHAR    = 0x0200,
    bBACKSPACE      = 0x0400,
    bCONNCETING     = 0x0800
};

struct RTOS_infrastructure {
    //QueueHandle_t qState{nullptr};
    QueueHandle_t qNetworkPhase{nullptr};
    QueueHandle_t qCO2TargetPending{nullptr};
    QueueHandle_t qCO2TargetCurr{nullptr};
    QueueHandle_t qCharPending{nullptr};

    TaskHandle_t tDisplay{nullptr};
};

#endif //FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H