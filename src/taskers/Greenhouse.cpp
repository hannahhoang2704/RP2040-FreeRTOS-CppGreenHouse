#include "Greenhouse.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>

using namespace std;

Greenhouse::Greenhouse(const shared_ptr<ModbusClient> &modbus_client, const shared_ptr<PicoI2C>&pressure_sensor_I2C) :
        mCO2(modbus_client),
        mHumidity(modbus_client),
        mTemperature(modbus_client),
        mPressure(pressure_sensor_I2C),
        mFan(modbus_client),
        mCO2_Emitter() {
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

void Greenhouse::task_automate_greenhouse(void *params) {
    auto object_ptr = static_cast<Greenhouse *>(params);
    object_ptr->automate_greenhouse();
}

void Greenhouse::automate_greenhouse() {
    Logger::log("Initiated GREENHOUSE task\n");
    stringstream ss;
    while (true) {
        Logger::log("Pressure Sensor is %d\n", 40);
        ss << setw(5) << setprecision(1) << fixed << mCO2.update();
        Logger::log("GMP252:    CO2: " + ss.str() + " ppm\n");
        ss.str("");
//        Logger::log("CO2 is %.1f ppm\n", mCO2.update());
        Logger::log("Temp from GMP252 is %.1f C and humidity is %.1f %%\n", mTemperature.update_GMP252(), mHumidity.update());
        Logger::log("Temp from HMP60 is %.1f C\n", mTemperature.update_HMP60());


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}