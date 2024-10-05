#include "Greenhouse.h"
#include "Logger.h"
#include "Display.h"
#include <sstream>
#include <iomanip>

using namespace std;

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
    mTimerHandle = xTimerCreate("GREENHOUSE_UPDATE",
                                pdMS_TO_TICKS(mTimerFreq),
                                pdTRUE,
                                iRTOS.sUpdateGreenhouse,
                                Greenhouse::passive_update);
    if (mTimerHandle != nullptr) {
        Logger::log("Created GREENHOUSE_UPDATE timer\n");
    } else {
        Logger::log("Failed to create GREENHOUSE_UPDATE timer\n");
    }

}

void Greenhouse::task_automate_greenhouse(void *params) {
    auto object_ptr = static_cast<Greenhouse *>(params);
    object_ptr->automate_greenhouse();
}

void Greenhouse::passive_update(TimerHandle_t xTimer) {
    xSemaphoreGive(pvTimerGetTimerID(xTimer));
}

void Greenhouse::automate_greenhouse() {
    Logger::log("Initiated GREENHOUSE task\n");
    mFan = aFan.read_power();
    xQueueOverwrite(iRTOS.qFan, &mFan);
    if (xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2Target, pdMS_TO_TICKS(1000)) == pdFALSE) {
        Logger::log("WARNING: qCO2TargetCurrent empty; actuators off\n");
        mFan = aFan.OFF;
        aFan.set_power(mFan);
        xQueueOverwrite(iRTOS.qFan, &mFan);
        aCO2_Emitter.put_state(false);
    }
    xTimerStart(mTimerHandle, portMAX_DELAY);
    while (true) {
        update_sensors();
        actuate();
        xSemaphoreTake(iRTOS.sUpdateGreenhouse, portMAX_DELAY);
        //vTaskDelay(pdMS_TO_TICKS(10)); //I think we need vTaskDelay here to give time for other task even though greenhouse is still highest priority
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
        emergency();
    }
    if (xQueuePeek(iRTOS.qCO2TargetCurrent, &mCO2Target, 0) == pdFALSE) {
        Logger::log("WARNING: qCO2TargetCurrent empty; actuators off\n");
        aFan.set_power(aFan.OFF);
        xQueueOverwrite(iRTOS.qFan, &mFan);
        aCO2_Emitter.put_state(false);
    } else {
        pursue_CO2_target();
    }
}

void Greenhouse::emergency() {
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
    xQueueOverwrite(iRTOS.qFan, &mFan);
}

void Greenhouse::pursue_CO2_target() {
    mCO2Delta = mCO2Measurement - static_cast<float>(mCO2Target);
    if (abs(mCO2Delta) > CO2_DELTA_MARGIN) {
        if (!mPursuingCO2Target) {
            mPursuingCO2Target = true;
            xTimerChangePeriod(mTimerHandle, PURSUING_TIMER_FREQ_MS, portMAX_DELAY);
        }

    } else {
        mPursuingCO2Target = false;
    }

    if (mCO2Delta > CO2_DELTA_MARGIN) {
        Logger::log("Pursuing CO2Target: [%hd] mCO2Measurement [%.1f]\n", mCO2Target, mCO2Measurement);
        mPursuingCO2Target = false;
        mFan = 200;
        aFan.set_power(mFan);
        xQueueOverwrite(iRTOS.qFan, &mFan);
    } else if (mCO2Delta < -CO2_DELTA_MARGIN) {
        mPursuingCO2Target = false;
        aCO2_Emitter.put_state(true);
    } else if (!mPursuingCO2Target) {
        mPursuingCO2Target = true;
        aFan.set_power(aFan.OFF);
        aCO2_Emitter.put_state(false);
    }
}
