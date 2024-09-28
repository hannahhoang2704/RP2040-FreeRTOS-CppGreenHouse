#ifndef FREERTOS_GREENHOUSE_EEPROM_H
#define FREERTOS_GREENHOUSE_EEPROM_H

#include <memory>
#include "i2c/PicoI2C.h"

#define AT24C256_I2C_DEV_ADDRESS 0x50
#define AT24C256_TOP_ADDRESS 0xFFFF
#define BITS_PER_BYTE 8
#define REG_ADDR_LEN 2

namespace EEPROM {

    class SingleStorage {
    public:
        SingleStorage(std::shared_ptr<PicoI2C> i2c_sp, uint8_t bytes, uint8_t deviceAddress = AT24C256_I2C_DEV_ADDRESS);
        int get();
        void put(int data);
    private:
        void apply_reg_addr();

        std::shared_ptr<PicoI2C> mI2C;
        uint8_t mDevAddr;
        uint8_t mBytes;
        uint16_t mRegAddr;
        uint8_t mBuffer[64 + REG_ADDR_LEN];
    };

}


#endif //FREERTOS_GREENHOUSE_EEPROM_H
