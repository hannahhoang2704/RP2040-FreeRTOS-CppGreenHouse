#include "Greenhouse.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>

using namespace std;

Greenhouse::Greenhouse(const shared_ptr<ModbusClient> &modbus_client, shared_ptr<PicoI2C>pressureSensorI2C) :
        mTaskName("Greenhouse"),
        mCO2(modbus_client),
        mHumidity(modbus_client),
        mTemperature(modbus_client),
        mPressureSensor(pressureSensorI2C),
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

void Greenhouse::task_automate_greenhouse(void *params) {
    auto object_ptr = static_cast<Greenhouse *>(params);
    object_ptr->automate_greenhouse();
}

void Greenhouse::automate_greenhouse() {
    Logger::log("Initiated GREENHOUSE task\n");
    mMIO12_V.write(100);
    mMIO12_V.write(100);
    stringstream ss;
    while (true) {
        ss << setw(5) << setprecision(1) << fixed << mCO2.update();
        Logger::log("Greenhouse: GMP252:    CO2: " + ss.str() + " ppm\n");
        ss.str("");
        vTaskDelay(1);
        ss << setw(5) << setprecision(1) << fixed << mTemperature.update_GMP252();
        Logger::log("Greenhouse: GMP252:   Temp: " + ss.str() + " C\n");
        ss.str("");
        vTaskDelay(1);
        ss << setw(5) << setprecision(1) << fixed << mHumidity.update();
        Logger::log("Greenhouse:  HMP60: RelHum: " + ss.str() + " %%\n");
        ss.str("");
        vTaskDelay(1);
        ss << setw(5) << setprecision(1) << fixed << mTemperature.update_HMP60();
        Logger::log("Greenhouse:  HMP60:   Temp: " + ss.str() + " C\n");
        ss.str("");
        vTaskDelay(1);
        mPressureSensor.read_pressure();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
