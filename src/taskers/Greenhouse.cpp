#include "Greenhouse.h"
#include "Logger.h"
#include "Display.h"
#include <sstream>
#include <iomanip>

using namespace std;

void null_timer(TimerHandle_t xTimer) {};

Greenhouse::Greenhouse(const shared_ptr<ModbusClient> &modbus_client, const shared_ptr<PicoI2C> &pressure_sensor_I2C,
                       const RTOS_infrastructure *RTOSi) :
        sCO2(modbus_client),
        sHumidity(modbus_client),
        sTemperature(modbus_client),
        sPressure(pressure_sensor_I2C),
        aFan(modbus_client),
        aCO2_Emitter(),
        iRTOS(RTOSi) {
    xTaskCreate(task_automate_greenhouse,
                    "GREENHOUSE",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 3,
                    &mTaskHandle);
    mUpdateTimerHandle = xTimerCreate("GREENHOUSE_UPDATE",
                                      pdMS_TO_TICKS(mTimerFreq),
                                      pdTRUE,
                                      iRTOS->sUpdateGreenhouse,
                                      Greenhouse::passive_update);
    mCO2WaitTimerHandle = xTimerCreate("CO2_WAIT",
                                       pdMS_TO_TICKS(CO2_DIFFUSION_MS),
                                       pdFALSE,
                                       nullptr,
                                       null_timer);
}

void Greenhouse::task_automate_greenhouse(void *params) {
    auto object_ptr = static_cast<Greenhouse *>(params);
    object_ptr->automate_greenhouse();
}

void Greenhouse::passive_update(TimerHandle_t xTimer) {
    xSemaphoreGive(pvTimerGetTimerID(xTimer));
}

void Greenhouse::automate_greenhouse() {
    Logger::log("Initiated\n");
    xTimerStart(mUpdateTimerHandle, portMAX_DELAY);
    while (true) {
        update_sensors();
        actuate();
        xSemaphoreTake(iRTOS->sUpdateGreenhouse, portMAX_DELAY);
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
    xQueueOverwrite(iRTOS->qCO2Measurement, &mCO2Measurement);
    xQueueOverwrite(iRTOS->qPressure, &mPressure);
    xQueueOverwrite(iRTOS->qHumidity, &mHumidity);
    xQueueOverwrite(iRTOS->qTemperature, &mTemperature);
    if (abs(prevCO2 - mCO2Measurement) > UPDATE_THRESHOLD ||
        static_cast<float>(abs(prevPressure - mPressure)) > UPDATE_THRESHOLD ||
        abs(prevHumidity - mHumidity) > UPDATE_THRESHOLD ||
        abs(prevTemperature - mTemperature) > UPDATE_THRESHOLD) {
        xSemaphoreGive(iRTOS->sUpdateDisplay);
    }
}

void Greenhouse::actuate() {
    if (mCO2Measurement > CO2_FATAL) {
        emergency();
    } else {
        if (xQueuePeek(iRTOS->qCO2TargetCurrent, &mCO2Target, 0) == pdFALSE) {
            Logger::log("WARNING: qCO2TargetCurrent empty; actuators off\n");
            mFan = aFan.OFF;
            aFan.set_power(mFan);
            aCO2_Emitter.put_state(false);
            xQueueOverwrite(iRTOS->qFan, &mFan);
            xSemaphoreGive(iRTOS->sUpdateDisplay);
        } else {
            pursue_CO2_target();
        }
    }
}

void Greenhouse::emergency() {
    Logger::log("EMERGENCY: CO2 Measurement: %5.1f ppm\n");
    if (mFan != aFan.MAX_POWER) {
        mFan = aFan.MAX_POWER;
        xQueueOverwrite(iRTOS->qFan, &mFan);
        while (!aFan.running() && aFan.read_power() != aFan.MAX_POWER) {
            aFan.set_power(mFan);
        }
        xSemaphoreGive(iRTOS->sUpdateDisplay);
    }
}

void Greenhouse::pursue_CO2_target() {
    if (!xTimerIsTimerActive(mCO2WaitTimerHandle)) {
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
                xQueueOverwrite(iRTOS->qFan, &mFan);
                xSemaphoreGive(iRTOS->sUpdateDisplay);
                mCO2PrevDelta = mCO2Delta;
                Logger::log("CO2 target margin reached: T:%hd M:%.1f\n", mCO2Target, mCO2Measurement);
            } else if (mFan != aFan.MAX_POWER / 4) {
                mFan = aFan.MAX_POWER / 4;
                aFan.set_power(mFan);
                xQueueOverwrite(iRTOS->qFan, &mFan);
                xSemaphoreGive(iRTOS->sUpdateDisplay);
                Logger::log("Fan set to: %hd\n", mFan);
            }
        } else if (mCO2Delta < 0) {
            if (mFan) {
                mFan = aFan.OFF;
                aFan.set_power(mFan);
                Logger::log("Fan off\n");
            }
            uint emissionPeriod_ms = static_cast<uint>(abs(mCO2Delta * 1000) / CO2_EXPECTED_EMISSION_RATE_CO2pS);
            if (emissionPeriod_ms > MAX_EMISSION_PERIOD_MS) emissionPeriod_ms = MAX_EMISSION_PERIOD_MS;
            if (emissionPeriod_ms < MIN_EMISSION_PERIOD_MS) emissionPeriod_ms = MIN_EMISSION_PERIOD_MS;
            aCO2_Emitter.put_state(true);
            Logger::log("CO2 emission open for %u ms\n", emissionPeriod_ms);
            vTaskDelay(pdMS_TO_TICKS(emissionPeriod_ms));
            aCO2_Emitter.put_state(false);
            Logger::log("CO2 emission closed\n");
            xTimerStart(mCO2WaitTimerHandle, portMAX_DELAY);
        } else {
            if (mFan) {
                mFan = aFan.OFF;
                aFan.set_power(mFan);
                xQueueOverwrite(iRTOS->qFan, &mFan);
                xSemaphoreGive(iRTOS->sUpdateDisplay);
                Logger::log("Shutting down fan\n");
            }
            if (aCO2_Emitter.get_state()) {
                aCO2_Emitter.put_state(false);
                Logger::log("Shutting down CO2 emitter\n");
            }
        }
    }
}
