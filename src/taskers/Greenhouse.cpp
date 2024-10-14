#include "Greenhouse.h"
#include "Logger.h"
#include "Display.h"
#include <sstream>
#include <iomanip>

using namespace std;

void null_timer(TimerHandle_t xTimer) {};

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
    mUpdateTimerHandle = xTimerCreate("GREENHOUSE_UPDATE",
                                      pdMS_TO_TICKS(mTimerFreq),
                                      pdTRUE,
                                      iRTOS.sUpdateGreenhouse,
                                      Greenhouse::passive_update);
    if (mUpdateTimerHandle != nullptr) {
        Logger::log("Created GREENHOUSE_UPDATE timer\n");
    } else {
        Logger::log("Failed to create GREENHOUSE_UPDATE timer\n");
    }
    mCO2WaitTimerHandle = xTimerCreate("CO2_WAIT",
                                       pdMS_TO_TICKS(CO2_DIFFUSION_MS),
                                       pdFALSE,
                                       nullptr,
                                       null_timer);
    if (mUpdateTimerHandle != nullptr) {
        Logger::log("Created GREENHOUSE_UPDATE timer\n");
    } else {
        Logger::log("Failed to create GREENHOUSE_UPDATE timer\n");
    }
    if (mCO2WaitTimerHandle != nullptr) {
        Logger::log("Created CO2_WAIT timer\n");
    } else {
        Logger::log("Failed to create CO2_WAIT timer\n");
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
    xTimerStart(mUpdateTimerHandle, portMAX_DELAY);
    while (true) {
        update_sensors();
        actuate();
        xSemaphoreTake(iRTOS.sUpdateGreenhouse, portMAX_DELAY);
    }
}

void Greenhouse::update_sensors() {
    float prevCO2 = mCO2Measurement;
    int prevPressure = mPressure;
    float prevHumidity = mHumidity;
    float prevTemperature = mTemperature;
    mCO2Measurement = sCO2.update();
    mPressure = sPressure.update_SDP610();
    mHumidity = sHumidity.update();
    mTemperature = sTemperature.update_all();
    xQueueOverwrite(iRTOS.qCO2Measurement, &mCO2Measurement);
    xQueueOverwrite(iRTOS.qPressure, &mPressure);
    xQueueOverwrite(iRTOS.qHumidity, &mHumidity);
    xQueueOverwrite(iRTOS.qTemperature, &mTemperature);
    if (abs(prevCO2 - mCO2Measurement) > UPDATE_THRESHOLD ||
        static_cast<float>(abs(prevPressure - mPressure)) > UPDATE_THRESHOLD ||
        abs(prevHumidity - mHumidity) > UPDATE_THRESHOLD ||
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
        mFan = aFan.OFF;
        aFan.set_power(mFan);
        aCO2_Emitter.put_state(false);
        xQueueOverwrite(iRTOS.qFan, &mFan);
        xSemaphoreGive(iRTOS.sUpdateDisplay);
    } else {
        pursue_CO2_target();
    }
}

void Greenhouse::emergency() {
    Logger::log("EMERGENCY: CO2 Measurement: %5.1f ppm\n");
    mFan = aFan.MAX_POWER;
    xQueueOverwrite(iRTOS.qFan, &mFan);
    while (!aFan.running() && aFan.read_power() != aFan.MAX_POWER) {
        aFan.set_power(mFan);
    }
    while (mCO2Measurement > CO2_FATAL) {
        mCO2Measurement = sCO2.update();
        mPressure = sPressure.update_SDP610();
        xQueueOverwrite(iRTOS.qCO2Measurement, &mCO2Measurement);
        xQueueOverwrite(iRTOS.qPressure, &mPressure);
        xSemaphoreGive(iRTOS.sUpdateDisplay);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    Logger::log("EMERGENCY OVER: CO2 Measurement: %5.1f ppm\n");
    aFan.set_power(aFan.OFF);
    xQueueOverwrite(iRTOS.qFan, &mFan);
}

void Greenhouse::pursue_CO2_target() {
    mCO2Delta = mCO2Measurement - static_cast<float>(mCO2Target);
    if (mCO2Delta > CO2_FAN_MARGIN) {
        if (aCO2_Emitter.get_state()) {
            aCO2_Emitter.put_state(false);
            Logger::log("CO2 emitter off\n");
        }
        mCO2ProjectedChange = mCO2Delta - mCO2PrevDelta;
        if (mCO2Delta + mCO2ProjectedChange < CO2_FAN_MARGIN) {
            mFan = aFan.OFF;
            aFan.set_power(mFan);
            xQueueOverwrite(iRTOS.qFan, &mFan);
            xSemaphoreGive(iRTOS.sUpdateDisplay);
            mCO2PrevDelta = mCO2Delta;
            Logger::log("CO2 target margin reached: T:%hd M:%.1f\n", mCO2Target, mCO2Measurement);
        } else if (mFan == aFan.OFF) {
            mFan = aFan.MAX_POWER / 4;
            aFan.set_power(mFan);
            xQueueOverwrite(iRTOS.qFan, &mFan);
            xSemaphoreGive(iRTOS.sUpdateDisplay);
            Logger::log("Fan set to: %hd\n", mFan);
        }
    } else if (mCO2Delta < -CO2_EMISSION_MARGIN) {
        if (mFan) {
            mFan = aFan.OFF;
            aFan.set_power(mFan);
            Logger::log("Fan off\n");
        }

        if (!xTimerIsTimerActive(mCO2WaitTimerHandle)) {
            aCO2_Emitter.put_state(true);
            Logger::log("CO2 emission open\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            aCO2_Emitter.put_state(false);
            Logger::log("CO2 emission closed\n");
            xTimerChangePeriod(mCO2WaitTimerHandle, pdMS_TO_TICKS(CO2_DIFFUSION_MS), portMAX_DELAY);
            xTimerStart(mCO2WaitTimerHandle, portMAX_DELAY);
        }
    } else {
        if (mFan) {
            mFan = aFan.OFF;
            aFan.set_power(mFan);
            xQueueOverwrite(iRTOS.qFan, &mFan);
            xSemaphoreGive(iRTOS.sUpdateDisplay);
            Logger::log("Shutting down fan\n");
        }
        if (aCO2_Emitter.get_state()) {
            aCO2_Emitter.put_state(false);
            Logger::log("Shutting down CO2 emitter\n");
        }
    }
}
