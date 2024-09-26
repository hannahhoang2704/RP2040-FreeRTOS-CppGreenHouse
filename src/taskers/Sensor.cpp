#include "Sensor.h"

#include <utility>
#include <iostream>

using namespace std;

Sensor::Sensor(shared_ptr<PicoOsUart> uart_sp, shared_ptr<ModbusClient> modbus_client, uint led_pin) :
        mUART(std::move(uart_sp)),
        mRH_RH(std::move(modbus_client), 241, 256),
        mT_RH(std::move(modbus_client), 241, 257),
        mLED(led_pin) {
    if (xTaskCreate(task_sense,
                    "SENSOR",
                    512,
                    (void *) this,
                    tskIDLE_PRIORITY + 3,
                    &mTaskHandle) == pdPASS) {
        cout << "Created SENSOR task" << endl;
    } else {
        cout << "Failed to create SENSOR task" << endl;
    }
}

void Sensor::sense() {
    cout << "Initiated SENSOR task" << endl;
    while (true) {
        mLED.toggle();
        printf("RH=%5.1f%%\n", mRH_RH.read() / 10.0);
        vTaskDelay(5);
        printf("T =%5.1f%%\n", mT_RH.read() / 10.0);
        vTaskDelay(3000);
    }
}
