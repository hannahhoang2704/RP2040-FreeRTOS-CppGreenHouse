#include <memory>
#include <pico/stdio.h>
#include "FreeRTOS.h"
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

const struct {
    uint ctrl_nr = 0; // controller number
    uint tx_pin = 0;
    uint rx_pin = 1;
    uint baud = 115200;
    uint8_t stop_bits = 1;
} UART0_s;

const struct {
    uint ctrl_nr = 1; // controller number
    uint tx_pin = 4;
    uint rx_pin = 5;
    uint baud = 9600;
    uint8_t stop_bits = 2;
} UART1_s;

const struct {
    uint ctrl_nr = 1;
    uint sda_pin = 14;
    uint scl_pin = 15;
    uint baud = 400000;
} I2C1_s;

const struct {
    uint ctrl_nr = 0;
    uint sda_pin = 16;
    uint scl_pin = 17;
    uint baud = 12345; // ? EEPROM
} I2C0_s;

using namespace std;

int main() {
    Logger::log("Boot!\n");

    /// interfaces
    auto CLI_UART = make_shared<PicoOsUart>(UART0_s.ctrl_nr, UART0_s.tx_pin, UART0_s.rx_pin, UART0_s.baud,UART0_s.stop_bits);
    auto modbusUART = make_shared<PicoOsUart>(UART1_s.ctrl_nr,UART1_s.tx_pin, UART1_s.rx_pin,UART1_s.baud,UART1_s.stop_bits);
    auto OLED_SDP600_I2C = make_shared<PicoI2C>(I2C1_s.ctrl_nr, I2C1_s.baud);
    auto rtu_client = make_shared<ModbusClient>(modbusUART);

    /// taskers
    new Greenhouse(rtu_client, OLED_SDP600_I2C);
    new Display(OLED_SDP600_I2C);
    new Logger(CLI_UART);
    new SwitchHandler();

    Logger::log("Initializing scheduler...\n");
    vTaskStartScheduler();
}
