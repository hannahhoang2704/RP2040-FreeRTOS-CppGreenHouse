#ifndef RP2040_FREERTOS_IRQ_GREENHOUSE_H
#define RP2040_FREERTOS_IRQ_GREENHOUSE_H


#include <memory>
#include <utility>

#include "uart/PicoOsUart.h"
#include "modbus/ModbusRegister.h"
#include "i2c/PicoI2C.h"
#include "Sensor.h"
#include "Actuator.h"
#include "RTOS_infrastructure.h"

class Greenhouse {
public:
    Greenhouse(const std::shared_ptr<ModbusClient> &modbus_client,
               const std::shared_ptr<PicoI2C> &pressure_sensor_I2C,
               const RTOS_infrastructure * RTOSi);

private:
    void automate_greenhouse();
    static void task_automate_greenhouse(void *params);
    static void passive_update(TimerHandle_t xTimer);

    void update_sensors();
    void actuate();
    void emergency();
    void pursue_CO2_target();

    TaskHandle_t mTaskHandle;
    TimerHandle_t mUpdateTimerHandle;
    TimerHandle_t mCO2WaitTimerHandle;

    const uint DEFAULT_TIMER_FREQ_MS{1000};
    const uint CO2_DIFFUSION_MS{15000};
    uint mTimerFreq{DEFAULT_TIMER_FREQ_MS};

    Sensor::CO2 sCO2;
    Sensor::PressureSensor sPressure;
    Sensor::Humidity sHumidity;
    Sensor::Temperature sTemperature;
    Actuator::Fan aFan;
    Actuator::CO2_Emitter aCO2_Emitter;

    int16_t mCO2Target{0};
    float mCO2Measurement{-1};
    int mPressure{-1};
    int16_t mFan{-1};
    float mHumidity{-1};
    float mTemperature{-273.16};
    const float UPDATE_THRESHOLD{1.0};

    const float CO2_FATAL{2000};
    const float CO2_EXPECTED_EMISSION_RATE_CO2pS{200};
    const float CO2_EXPECTED_FAN_EFFECT_RATE_CO2_ps{55};
    const float CO2_FAN_MARGIN{CO2_EXPECTED_EMISSION_RATE_CO2pS + CO2_EXPECTED_FAN_EFFECT_RATE_CO2_ps * 2};
    const uint MIN_EMISSION_PERIOD_MS{500}; // to avoid abusing the canisters valve mechanism with redundant emissions
    const uint MAX_EMISSION_PERIOD_MS{2000};
    float mCO2Delta{0};
    float mCO2PrevDelta{0};
    float mCO2ProjectedChange{0};

    const RTOS_infrastructure * iRTOS;
};

#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
