#include "Greenhouse.h"
#include "Logger.h"

using namespace std;

Greenhouse::Greenhouse(const shared_ptr<ModbusClient> &modbus_client, uint led_pin) :
        mTaskName("Greenhouse"),
        mGMP252(modbus_client),
        mHMP60(modbus_client),
        // missing pressure sensor
        mMIO12_V(modbus_client, 1, 0) {
    if (xTaskCreate(task_automate_greenhouse,
                    mTaskName.c_str(),
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
    mMIO12_V.write(50);
    mMIO12_V.write(50);
    while (true) {
        Logger::log("Greenhouse: GMP252: CO2: %5.1f ppm\n", mGMP252.update_co2());
        Logger::log("Greenhouse: GMP252: Temp: %5.1f C\n", mGMP252.update_temperature());
        Logger::log("Greenhouse: HMP60: RelHum: %4.1f %%\n", mHMP60.update_relHum());
        Logger::log("Greenhouse: HMP60: Temp: %5.1f C\n", mHMP60.update_temperature());

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}