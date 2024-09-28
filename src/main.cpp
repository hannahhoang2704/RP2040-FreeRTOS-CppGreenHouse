#include <memory>
#include <pico/stdio.h>
#include "uart/PicoOsUart.h"
#include "Greenhouse.h"
#include "Display.h"
#include "SwitchHandler.h"
#include "Logger.h"

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

#define MODBUS_STOP_BITS 2 // for real system (pico simualtor also requires 2 stop bits)
#define MODBUS_BAUD_RATE 9600
#define MODBUS_UART_NR 1
#define MODBUS_UART_TX_PIN 4
#define MODBUS_UART_RX_PIN 5

#define CLI_STOP_BITS 1
#define CLI_BAUD_RATE 115200
#define CLI_UART_NR 0
#define CLI_UART_TX_PIN 0
#define CLI_UART_RX_PIN 1

#define OLED_SDP_I2C_BUS 1
#define OLED_SDP_I2C_BAUD 400000

using namespace std;

int main() {
    Logger::log("Boot!\n");

    /// interfaces
    auto CLI_UART = make_shared<PicoOsUart>(
            CLI_UART_NR,
            CLI_UART_TX_PIN,
            CLI_UART_RX_PIN,
            CLI_BAUD_RATE,
            CLI_STOP_BITS);
    auto modbusUART = make_shared<PicoOsUart>(
            MODBUS_UART_NR,
            MODBUS_UART_TX_PIN,
            MODBUS_UART_RX_PIN,
            MODBUS_BAUD_RATE,
            MODBUS_STOP_BITS);
    auto OLED_SDP600_I2C = make_shared<PicoI2C>(
            OLED_SDP_I2C_BUS,
            OLED_SDP_I2C_BAUD);
    auto rtu_client = make_shared<ModbusClient>(modbusUART);

    /// taskers
    new Greenhouse(CLI_UART, rtu_client, LED2);
    new Display(OLED_SDP600_I2C);
    new Logger(CLI_UART);
    new SwitchHandler();

    Logger::log("Initializing scheduler...\n");
    vTaskStartScheduler();
}
