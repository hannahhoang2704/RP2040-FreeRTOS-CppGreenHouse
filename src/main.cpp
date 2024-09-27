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

using namespace std;

int main() {
    Logger::log("Boot!\n");

    /// interfaces
    auto CLI_UART = make_shared<PicoOsUart>(0,0,1,115200,1);
    auto modbusUART = make_shared<PicoOsUart>(1,4,5,9600,2);
    auto OLED_SDP600_I2C = make_shared<PicoI2C>(1,400000);
    auto rtu_client = make_shared<ModbusClient>(modbusUART);

    /// taskers
    new Greenhouse(rtu_client);
    new Display(OLED_SDP600_I2C);
    new Logger(CLI_UART);
    new SwitchHandler();

    Logger::log("Initializing scheduler...\n");
    vTaskStartScheduler();
}
