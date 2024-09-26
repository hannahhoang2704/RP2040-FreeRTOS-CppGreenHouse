#include "Greenhouse.h"
#include "Logger.h"

using namespace std;

Greenhouse::Greenhouse(shared_ptr<PicoOsUart> uart_sp, shared_ptr<ModbusClient> modbus_client, uint led_pin) :
        mCLI_UART(std::move(uart_sp)),
        mHMP60_rh(modbus_client, HMP60_ADDRESS, 256),
        mHMP60_t(modbus_client, HMP60_ADDRESS, 257),
        mGMP252_co2_low(modbus_client, GMP252_ADDRESS, 0x0000),
        mGMP252_co2_high(modbus_client, GMP252_ADDRESS, 0x0001),
        mGMP252_t_low(modbus_client, GMP252_ADDRESS, 0x0004),
        mGMP252_t_high(modbus_client, GMP252_ADDRESS, 0x0005),
        // missing pressure sensor
        mMIO12_V(modbus_client, 1, 0),
        mLED(make_shared<LED>(led_pin)) {

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

void Greenhouse::automate_greenhouse() {
    mCLI_UART->send("Initiated SENSOR task\n");
    mMIO12_V.write(100);
    vTaskDelay(pdMS_TO_TICKS(5));
    Logger::log("Initiated GREENHOUSE task\n");
    mFan.write(100);
    char msg[64];
    while (true) {
        mLED->toggle();
        mCLI_UART->send("\n");

        update_T();
        snprintf(msg, 64, " TC = %5.1f C\n", mTempGMP252.f);
        mCLI_UART->send(msg);
        snprintf(msg, 64, " TR = %5.1f C\n", mTempHMP60.f);
        mCLI_UART->send(msg);

        snprintf(msg, 64, "  T = %5.1f C\n", update_T());
        mCLI_UART->send(msg);

        snprintf(msg, 64, " RH = %5.1f %%\n", update_RH());
        mCLI_UART->send(msg);

        snprintf(msg, 64, "CO2 = %5.1f ppm CO2\n", update_CO2());
        mCLI_UART->send(msg);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

float Greenhouse::update_CO2() {
    mCO2.u32 = mGMP252_co2_low.read();
    mCO2.u32 += mGMP252_co2_high.read() << 16;
    return mCO2.f;
}

float Greenhouse::update_RH() {
    mRelHum.u16  = mHMP60_rh.read();
    mRelHum.f /= 10;
    return mRelHum.f;
}

float Greenhouse::update_T() {
    mTempGMP252.u32 = mGMP252_t_low.read();
    mTempGMP252.u32 += mGMP252_t_high.read() << 16;
    mTempHMP60.u16 = mHMP60_t.read();
    //mTempHMP60.f /= 10;
    return (mTempHMP60.f + mTempGMP252.f) / 2;
}
