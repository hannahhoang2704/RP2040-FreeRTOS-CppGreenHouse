#include <iostream>
#include <memory>
#include "FreeRTOS.h"
#include "uart/PicoOsUart.h"
#include "Sensor.h"
#include "ConsoleInput.h"

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

#define MODBUS_PROTOTYPE 1 // as opposed to blinker example

#define BAUD_RATE 9600
#define STOP_BITS 2 // for real system (pico simualtor also requires 2 stop bits)

#if MODBUS_PROTOTYPE
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#else
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#endif

using namespace std;

int main() {
    stdio_init_all();
    cout << "Boot!" << endl;

    auto uart = make_shared<PicoOsUart>(
            UART_NR,
            UART_TX_PIN,
            UART_RX_PIN,
            BAUD_RATE,
            STOP_BITS);
#if MODBUS_PROTOTYPE
    auto rtu_client = make_shared<ModbusClient>(uart);

    new Sensor(uart, rtu_client, LED2);
    // fan
#else
    new ConsoleInput(uart, LED1);
#endif

    std::cout << "Initializing scheduler..." << std::endl;
    vTaskStartScheduler();

    while (true) {};
}
