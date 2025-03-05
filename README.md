# Project description
The project's purpose is to develop a CO2 controller for a small greenhouse. The controller was a Raspberry Pi PicoW, integrated onto a custom development board designed by the school’s teacher, equipped with various ports for switches, an OLED display, an EEPROM memory chip and pertinent ports for communicating with the system’s fan and various sensors via a Modbus server, excluding a pressure sensor which communicated via a different port, and a custom integrated CO2 cannister for CO2 output.

The controller was required to keep the CO2 level at a user set level via CO2 emission and fan control, offer an interface for a user to set the CO2 target, display all sensor readings on the controller’s OLED display as well as provide this documentation and systems user manual.

# rp2040-freertos-CPP-template

This is a template project for developing FreeRTOS based applications on Raspberry Pi RP2040 based boards. 
This template uses the "official" RP2040 port from the Raspberry Pi Foundation.
A stripped down version of FreeRTOSKernel V10.6.2 is included in the project. 
All other ports except RP2040 port have been removed to reduce disk usage.

The drivers included in the project are interrupt driven and require FreeRTOS to work correctly.

