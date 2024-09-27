#include "Greenhouse.h"
#include "Logger.h"

using namespace std;

Greenhouse::Greenhouse(const shared_ptr<ModbusClient> &modbus_client) :
        mTaskName("Greenhouse"),
        mCO2(modbus_client),
        mHumidity(modbus_client),
        mTemperature(modbus_client),
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
        mCO2.update();
        Logger::log("Greenhouse: GMP252: CO2:    %5u ppm\n", mCO2.update().u32);
        Logger::log("Greenhouse: GMP252: Temp:   %5.1f C\n", mTemperature.update_GMP252());
        Logger::log("Greenhouse: HMP60:  RelHum: %5.1f %%\n", mHumidity.update());
        Logger::log("Greenhouse: HMP60:  Temp:   %5.1f C\n", mTemperature.update_HMP60());

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}