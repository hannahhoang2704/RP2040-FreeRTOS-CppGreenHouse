#include <memory>
#include <pico/stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "uart/PicoOsUart.h"
#include "Greenhouse.h"
#include "Display.h"
#include "SwitchHandler.h"
#include "Logger.h"
#include "RTOS_infrastructure.h"
#include "EEPROM.h"
#include "Storage.h"
#include "ThingSpeaker.h"
extern "C" {
#include "tls_common.h"
#include "lwip/altcp.h"
}

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

const struct {
    uint ctrl_nr = 0; // controller number
    uint tx_pin = 0;
    uint rx_pin = 1;
    uint baud = 115200;
    uint8_t stop_bits = 1;
} UART0_s;

const struct {
    uint ctrl_nr = 1; // controller number
    uint tx_pin = 4;
    uint rx_pin = 5;
    uint baud = 9600;
    uint8_t stop_bits = 2;
} UART1_s;

const struct {
    uint ctrl_nr = 1;
    uint sda_pin = 14;
    uint scl_pin = 15;
    uint baud = 400000;
} I2C1_s;

const struct {
    uint ctrl_nr = 0;
    uint sda_pin = 16;
    uint scl_pin = 17;
    uint baud = 100000;
} I2C0_s;

using namespace std;

void send_data_callback(TimerHandle_t xTimer) {
    auto iRTOS = (RTOS_infrastructure *) pvTimerGetTimerID(xTimer);
    Logger::log("send data to thingspeak\n");
}

void receive_data_callback(TimerHandle_t xTimer) {
    auto iRTOS = (RTOS_infrastructure *) pvTimerGetTimerID(xTimer);
    Logger::log("receive data from talkback\n");
}

void task_speak(void * params) {
    auto iRTOS = (RTOS_infrastructure *) params;
    char PW[MAX_STRING_LEN];
    char SSID[MAX_STRING_LEN];

    if(xQueuePeek(iRTOS->qNetworkStrings[NEW_SSID], SSID, 0) == pdTRUE){
        Logger::log("SSID from storage: %s\n", SSID);
    }else{
        strcpy(SSID, "Hannah");
    }
    if(xQueuePeek(iRTOS->qNetworkStrings[NEW_PW], PW, 0) == pdTRUE){
        Logger::log("PW: %s from storage\n", PW);
    }else{
        strcpy(PW, "abcdehannah");
    }

    if (cyw43_arch_init()) {
        Logger::log("Fail to initialize\n");
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PW, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        Logger::log("failed to connect\n");
    }
    Logger::log("Connected to WiFi\n");
    TimerHandle_t mSendDataTimer = xTimerCreate("SEND_DATA_TO_THINGSPEAK",
                                  pdMS_TO_TICKS(15000),
                                  pdTRUE,
                                  (void *)& iRTOS,
                                  send_data_callback);
    TimerHandle_t mReceiveDataTimer = xTimerCreate("RECEIVE_DATA_TO_THINGSPEAK",
                                     pdMS_TO_TICKS(5000),
                                     pdTRUE,
                                     (void *) &iRTOS,
                                     receive_data_callback);
}

int main() {
    Logger::log("Boot!\n");

    /// interfaces
    auto CLI_UART = make_shared<PicoOsUart>(UART0_s.ctrl_nr, UART0_s.tx_pin, UART0_s.rx_pin, UART0_s.baud,UART0_s.stop_bits);
    auto modbusUART = make_shared<PicoOsUart>(UART1_s.ctrl_nr, UART1_s.tx_pin, UART1_s.rx_pin, UART1_s.baud,UART1_s.stop_bits);
    auto OLED_SDP600_I2C = make_shared<PicoI2C>(I2C1_s.ctrl_nr, I2C1_s.baud);
    auto rtu_client = make_shared<ModbusClient>(modbusUART);
    auto EEPROM_I2C = make_shared<PicoI2C>(I2C0_s.ctrl_nr, I2C0_s.baud);

    /// RTOS infrastructure
    // for passing mutual RTOS infrastructure to requiring taskers
    const RTOS_infrastructure iRTOS{
            .qState             = xQueueCreate(1, sizeof(uint8_t)),
            .qNetworkPhase      = xQueueCreate(1, sizeof(uint8_t)),
            .qCO2TargetPending  = xQueueCreate(1, sizeof(int16_t)),
            .qCO2TargetCurrent  = xQueueCreate(1, sizeof(int16_t)),
            .qCO2Measurement    = xQueueCreate(1, sizeof(float)),
            .qPressure          = xQueueCreate(1, sizeof(int)),
            .qFan               = xQueueCreate(1, sizeof(int16_t)),
            .qHumidity          = xQueueCreate(1, sizeof(float)),
            .qTemperature       = xQueueCreate(1, sizeof(float)),
            .qCharPending       = xQueueCreate(1, sizeof(char)),
            .qConnectionState   = xQueueCreate(1, sizeof(uint8_t)),
            .qNetworkStrings = {
                    [NEW_API]   = xQueueCreate(1, sizeof(char[MAX_STRING_LEN])),
                    [NEW_SSID]  = xQueueCreate(1, sizeof(char[MAX_STRING_LEN])),
                    [NEW_PW]    = xQueueCreate(1, sizeof(char[MAX_STRING_LEN]))
            },
            .qStorageQueue = xQueueCreate(5, sizeof(storage_data)),

            .sUpdateGreenhouse = xSemaphoreCreateBinary(),
            .sUpdateDisplay = xSemaphoreCreateBinary(),
            .sWifiConnected = xSemaphoreCreateBinary(),
            .xThingSpeakEvent = xEventGroupCreate()
    };

    // register all the queues
    vQueueAddToRegistry(iRTOS.qState, "State");
    vQueueAddToRegistry(iRTOS.qNetworkPhase, "NetworkCredentials");
    vQueueAddToRegistry(iRTOS.qCO2TargetPending, "CO2TargetPending");
    vQueueAddToRegistry(iRTOS.qCO2TargetCurrent, "CO2TargetCurrent");
    vQueueAddToRegistry(iRTOS.qCO2Measurement, "qCO2Measurement");
    vQueueAddToRegistry(iRTOS.qPressure, "Pressure");
    vQueueAddToRegistry(iRTOS.qFan, "Fan");
    vQueueAddToRegistry(iRTOS.qHumidity, "Humidity");
    vQueueAddToRegistry(iRTOS.qTemperature, "Temperature");
    vQueueAddToRegistry(iRTOS.qCharPending, "CharacterPending");
    vQueueAddToRegistry(iRTOS.qConnectionState, "ConnectionState");
    vQueueAddToRegistry(iRTOS.qNetworkStrings[NEW_API], "NewAPI");
    vQueueAddToRegistry(iRTOS.qNetworkStrings[NEW_SSID], "NewSSID");
    vQueueAddToRegistry(iRTOS.qNetworkStrings[NEW_PW], "NewPW");
    vQueueAddToRegistry(iRTOS.qStorageQueue, "StorageQueue");

    char ssid[] = "Hannah";
    char pwd[] = "abcdehannah";
    char api[] = "9JS2SW0BYBVNSLTC";
    /// taskers
//    new ThingSpeaker(iRTOS, ssid, pwd, api);
    new Storage(EEPROM_I2C, iRTOS);
    new Logger(CLI_UART);
    new Display(OLED_SDP600_I2C, iRTOS);
    new Greenhouse(rtu_client, OLED_SDP600_I2C, iRTOS);
    new SwitchHandler(iRTOS);
    xTaskCreate(task_speak,"THINGSPEAK", 8192, (void *) &iRTOS, tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
}
