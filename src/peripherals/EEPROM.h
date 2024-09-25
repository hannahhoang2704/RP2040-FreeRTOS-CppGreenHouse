#ifndef FREERTOS_GREENHOUSE_EEPROM_H
#define FREERTOS_GREENHOUSE_EEPROM_H

#include <memory>
#include "i2c/PicoI2C.h"

namespace EEPROM {

    class SingleStorage {
    public:
        SingleStorage(std::shared_ptr<PicoI2C> i2c_sp, uint8_t data_size, uint8_t device_address = AT24C256_DEV_ADDRESS);
        int get();
        void put(int data);

    private:
        void apply_reg_addr();

        static const uint8_t AT24C256_DEV_ADDRESS{0x50};
        static const uint16_t AT24C256_TOP_ADDRESS{0xFFFF};
        static const uint8_t BITS_PER_BYTE{8};
        static const uint8_t REG_ADDR_LEN{2};

        std::shared_ptr<PicoI2C> mI2C;
        uint8_t mDevAddr;
        uint8_t mBytes;
        uint16_t mRegAddr;
        uint8_t mBuffer[64 + REG_ADDR_LEN];
    };

}


#endif //FREERTOS_GREENHOUSE_EEPROM_H
