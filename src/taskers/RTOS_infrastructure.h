#ifndef FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
#define FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
// RTOS task notifications require breathing room
// OLED gets crazy if events are sent too frequently -- i.e. with Rotor
const int16_t CO2_MAX = 1500;
const int16_t CO2_MIN = 0;
const char INIT_CHAR = '.';
const uint MAX_STRING_LEN = 64;

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

enum thing_speak_event{
    bSEND = 0x01,
    bRECEIVE = 0x02,
    bRECONNECT = 0x04
};

struct RTOS_infrastructure {
    QueueHandle_t qState;
    QueueHandle_t qNetworkPhase;
    QueueHandle_t qCO2TargetPending;
    QueueHandle_t qCO2TargetCurrent;
    QueueHandle_t qCO2Measurement;
    QueueHandle_t qPressure;
    QueueHandle_t qFan;
    QueueHandle_t qHumidity;
    QueueHandle_t qTemperature;
    QueueHandle_t qCharPending;
    QueueHandle_t qConnectionState;
    QueueHandle_t qNetworkStrings[3];
    QueueHandle_t qStorageQueue;

    SemaphoreHandle_t sUpdateGreenhouse;
    SemaphoreHandle_t sUpdateDisplay;
    SemaphoreHandle_t sWifiConnected;
    EventGroupHandle_t xThingSpeakEvent;
};

#endif //FREERTOS_GREENHOUSE_RTOS_INFRASTRUCTURE_H
