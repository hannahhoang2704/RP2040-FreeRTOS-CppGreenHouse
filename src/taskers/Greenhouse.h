#ifndef RP2040_FREERTOS_IRQ_GREENHOUSE_H
#define RP2040_FREERTOS_IRQ_GREENHOUSE_H


enum greenhouse_notifications {
    bPASSIVE = 0x01
};

#include <memory>
#include <utility>
#include <iostream>

#include "uart/PicoOsUart.h"
#include "LED.h"
#include "modbus/ModbusRegister.h"
#include "Sensor.h"
#include "Actuator.h"
#include "RTOS_infrastructure.h"
#include "timers.h"

class Greenhouse {
public:
    Greenhouse(const std::shared_ptr<ModbusClient> &modbus_client, const std::shared_ptr<PicoI2C> &pressure_sensor_I2C, RTOS_infrastructure RTOSi);

private:
    void automate_greenhouse();
    static void task_automate_greenhouse(void *params);
    static void passive_update(TimerHandle_t xTimer);

    void update_sensors();
    void actuate();
    void emergency();
    void pursue_CO2_target();

    TaskHandle_t mTaskHandle;
    TimerHandle_t mTimerHandle;
    static uint64_t mPrevNotification;

    const uint DEFAULT_TIMER_FREQ_MS{1000};
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
    const float UPDATE_THRESHOLD{1};

    const float CO2_FATAL{2000};
    const float CO2_DELTA_MARGIN{100};
    float mCO2Delta{0};
    bool mCO2OnTarget{true};


    RTOS_infrastructure iRTOS;
};

#endif //RP2040_FREERTOS_IRQ_GREENHOUSE_H
