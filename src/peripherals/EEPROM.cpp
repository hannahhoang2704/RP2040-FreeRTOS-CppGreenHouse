#include "EEPROM.h"

#include <utility>

static uint16_t nextFreeAddress{0};

EEPROM::SingleStorage::SingleStorage(std::shared_ptr<PicoI2C> i2c_sp, uint8_t bytes, uint8_t deviceAddress) :
        mI2C(std::move(i2c_sp)),
        mBytes(bytes),
        mDevAddr(deviceAddress),
        mRegAddr(nextFreeAddress)
{
    nextFreeAddress += 2 << (mBytes * BITS_PER_BYTE);
    if (nextFreeAddress > AT24C256_TOP_ADDRESS) {
        // prompt warning: EEPROM space exceeded
        // and maybe do something smart...
    }
}

int EEPROM::SingleStorage::get() {
    apply_reg_addr();
    mI2C->read(mDevAddr, mBuffer, mBytes + REG_ADDR_LEN);
    int retVal = 0;
    for (int byte = 0; byte < mBytes; ++byte) {
        retVal += static_cast<int>(mBuffer[byte + REG_ADDR_LEN]) << (byte * BITS_PER_BYTE);
    }
    return retVal;
}

void EEPROM::SingleStorage::put(int data) {
    apply_reg_addr();
    for (int byte = 0; byte < mBytes; ++byte) {
        mBuffer[byte] = static_cast<uint8_t>(2);
    }
    mI2C->write(mDevAddr, mBuffer, mBytes);
}

void EEPROM::SingleStorage::apply_reg_addr() {
    mBuffer[0] = static_cast<uint8_t>((mRegAddr & 0xFF00) >> BITS_PER_BYTE);
    mBuffer[1] = static_cast<uint8_t>(mRegAddr & 0x00FF);
}