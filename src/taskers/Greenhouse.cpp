#include "Greenhouse.h"
#include "Logger.h"

using namespace std;

Greenhouse::Greenhouse(shared_ptr<PicoOsUart> uart_sp, shared_ptr<ModbusClient> modbus_client, uint led_pin) :
        mCLI_UART(std::move(uart_sp)),
        mRH_RH(std::move(modbus_client), 241, 256),
        mT_RH(std::move(modbus_client), 241, 257),
        // missing CO2 sensor
        // missing pressure sensor
        mFan(std::move(modbus_client), 1, 0),
        mLED(make_shared<LED>(led_pin)) {

    if (xTaskCreate(task_automate_greenhouse,
                    "GREENHOUSE",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 3,
                    &mTaskHandle) == pdPASS) {
        Logger::log("Created GREENHOUSE task.\n");
    } else {
        Logger::log("Failed to create GREENHOUSE task.\n");
    }
}

void Greenhouse::automate_greenhouse() {
    Logger::log("Initiated GREENHOUSE task\n");
    mFan.write(100);
    char msg[64];
    while (true) {
        mLED->toggle();
        snprintf(msg, 64, "RH=%5.1f%%\n", mRH_RH.read() / 10.0);
        mCLI_UART->send(msg);
        vTaskDelay(5);
        snprintf(msg, 64, "T =%5.1f%%\n", mT_RH.read() / 10.0);
        mCLI_UART->send(msg);
        vTaskDelay(3000);
    }
}
