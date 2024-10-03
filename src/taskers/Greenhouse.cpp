#include "Greenhouse.h"
#include "Logger.h"
#include "Display.h"
#include <sstream>
#include <iomanip>

using namespace std;

uint64_t Greenhouse::mPrevNotification{0};
TaskHandle_t Greenhouse::mTaskHandle{nullptr};

void Greenhouse::notify(eNotifyAction eAction, uint32_t note) {
    if (time_us_64() - mPrevNotification > TASK_NOTIFICATION_RATE_LIMIT_US) {
        mPrevNotification = time_us_64();
        xTaskNotify(mTaskHandle, note, eAction);
    }
}

Greenhouse::Greenhouse(const shared_ptr<ModbusClient> &modbus_client, const shared_ptr<PicoI2C> &pressure_sensor_I2C,
                       RTOS_infrastructure RTOSi) :
        sCO2(modbus_client),
        sHumidity(modbus_client),
        sTemperature(modbus_client),
        sPressure(pressure_sensor_I2C),
        aFan(modbus_client),
        aCO2_Emitter(),
        iRTOS(RTOSi) {
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
    mTimerHandle = xTimerCreate("GREENHOUSE_UPKEEP",
                                pdMS_TO_TICKS(mTimerFreq),
                                pdTRUE,
                                iRTOS.sUpdateGreenhouse,
                                Greenhouse::upkeep);
    if (mTimerHandle != nullptr) {
        Logger::log("Created GREENHOUSE_UPKEEP timer\n");
    } else {
        Logger::log("Failed to create GREENHOUSE_UPKEEP timer\n");
    }
}

void Greenhouse::task_automate_greenhouse(void *params) {
    auto object_ptr = static_cast<Greenhouse *>(params);
    object_ptr->automate_greenhouse();
}

void Greenhouse::upkeep(TimerHandle_t xTimer) {
    xSemaphoreGive(pvTimerGetTimerID(xTimer));
}

void Greenhouse::automate_greenhouse() {
    Logger::log("Initiated GREENHOUSE task\n");
    stringstream ss;
    if (xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2Target, pdMS_TO_TICKS(1000)) == pdFALSE) {
        Logger::log("WARNING: qCO2TargetCurrent empty\n");
    }
    xTimerStart(mTimerHandle, portMAX_DELAY);
    while (true) {
        update_sensors();
        actuate();
        xSemaphoreTake(iRTOS.sUpdateGreenhouse, portMAX_DELAY);
    }
}

void Greenhouse::update_sensors() {
    float prevCO2         = mCO2Measurement;
    int prevPressure      = mPressure;
    float prevHumidity    = mHumidity;
    float prevTemperature = mTemperature;
    mCO2Measurement = sCO2.update();
    mPressure       = sPressure.update_SDP610();
    mHumidity       = sHumidity.update();
    mTemperature    = sTemperature.update_all();
    xQueueOverwrite(iRTOS.qCO2Measurement, &mCO2Measurement);
    xQueueOverwrite(iRTOS.qPressure,       &mPressure);
    xQueueOverwrite(iRTOS.qHumidity,       &mHumidity);
    xQueueOverwrite(iRTOS.qTemperature,    &mTemperature);
    if (abs(prevCO2 - mCO2Measurement)      > UPDATE_THRESHOLD ||
        static_cast<float>(abs(prevPressure - mPressure)) > UPDATE_THRESHOLD ||
        abs(prevHumidity - mHumidity)       > UPDATE_THRESHOLD ||
        abs(prevTemperature - mTemperature) > UPDATE_THRESHOLD) {
        xSemaphoreGive(iRTOS.sUpdateDisplay);
    }
}

void Greenhouse::actuate() {
    if (mCO2Measurement > CO2_FATAL) {
        Logger::log("EMERGENCY: CO2 Measurement: %5.1f ppm\n");
        while (!aFan.running() && aFan.read_power() != aFan.MAX_POWER) {
            aFan.set_power(aFan.MAX_POWER);
        }
        xQueueOverwrite(iRTOS.qFan, &mFan);
        while (mCO2Measurement > CO2_FATAL) {
            mCO2Measurement = sCO2.update();
            xQueueOverwrite(iRTOS.qCO2Measurement, &mCO2Measurement);
        }
        Logger::log("EMERGENCY OVER: CO2 Measurement: %5.1f ppm\n");
        aFan.set_power(aFan.OFF);
    }
    if (xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2Target, 0) == pdFALSE) {
        Logger::log("WARNING: qCO2TargetCurrent empty; actuators off\n");
    } else {
        mCO2Delta = mCO2Measurement - static_cast<float>(mCO2Target);
        if (mCO2Delta > CO2_DELTA_MARGIN) {
            mCO2OnTarget = false;
            aFan.set_power(200);
        } else if (mCO2Delta < -CO2_DELTA_MARGIN) {
            mCO2OnTarget = false;
            aCO2_Emitter.put_state(true);
        } else if (!mCO2OnTarget) {
            mCO2OnTarget = true;
            aFan.set_power(aFan.OFF);
            aCO2_Emitter.put_state(false);
        }
    }
}
