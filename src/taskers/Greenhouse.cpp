#include "Greenhouse.h"
#include "Logger.h"

using namespace std;

Greenhouse::Greenhouse(shared_ptr<PicoOsUart> uart_sp, const shared_ptr<ModbusClient>& modbus_client, uint led_pin) :
        mCLI_UART(std::move(uart_sp)),
        mGMP252(modbus_client),
        mHMP60(modbus_client),
        // missing pressure sensor
        mMIO12_V(modbus_client, 1, 0),
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
    mCLI_UART->send("Initiated SENSOR task\n");
    mMIO12_V.write(0);
    mMIO12_V.write(0);
    vTaskDelay(pdMS_TO_TICKS(5));
    Logger::log("Initiated GREENHOUSE task\n");
    mFan.write(100);
    char msg[64];
    while (true) {
        mLED->toggle();
        mCLI_UART->send("\n");

        snprintf(msg, 64, " TC = %5.1f C\n", mGMP252.update_temperature());
        mCLI_UART->send(msg);

        snprintf(msg, 64, "CO2 = %5.1f ppm CO2\n", mGMP252.update_co2());
        mCLI_UART->send(msg);

        snprintf(msg, 64, " TR = %5.1f C\n", mHMP60.update_temperature());
        mCLI_UART->send(msg);

        snprintf(msg, 64, " RH = %5.1f %%\n", mHMP60.update_relHum());
        mCLI_UART->send(msg);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}