#ifndef FREERTOS_GREENHOUSE_EEPROM_H
#define FREERTOS_GREENHOUSE_EEPROM_H

#include <memory>
#include "i2c/PicoI2C.h"

//static uint16_t nextFreeAddress{0};

class EEPROM{
public:
    EEPROM(std::shared_ptr<PicoI2C> i2c_sp, uint8_t device_address = AT24C256_DEV_ADDRESS);
    static const uint16_t ENTRY_SIZE{64};
    static const uint16_t AT24C256_TOP_ADDRESS{32768-1};
    static const uint16_t FIRST_ADDR{0};
    static const uint8_t MAX_ENTRIES{32};

    enum eeprom_addr{
        LOG_INDEX_ADDR = FIRST_ADDR,
//        CO2_TARGET_ADDR = AT24C256_TOP_ADDRESS-64*2,
//        IP_ADDR = AT24C256_TOP_ADDRESS-64*3,
//        USERNAME_ADDR = AT24C256_TOP_ADDRESS-64*4,
//        PW_ADDR = AT24C256_TOP_ADDRESS-64*5,
        CO2_TARGET_ADDR = FIRST_ADDR + ENTRY_SIZE,
        IP_ADDR = FIRST_ADDR + ENTRY_SIZE*2,
        USERNAME_ADDR = FIRST_ADDR + ENTRY_SIZE*3,
        PW_ADDR = FIRST_ADDR + ENTRY_SIZE*4,
        LOG_FIRST_ADDR = FIRST_ADDR + ENTRY_SIZE*5
    };

    void put(uint16_t address, uint16_t number);
    void put(uint16_t address, const std::string& str);
    uint16_t get(uint16_t address);
    std::string get_str(uint16_t address);

    void put_log_entry(const char *str);
    void get_log_entry();
    void erase_logs();
    void get_log_index_value(const uint16_t idx);
private:
    static const uint8_t AT24C256_DEV_ADDRESS{0x50};
    static const uint8_t BITS_PER_BYTE{8};
    static const uint8_t REG_ADDR_LEN{2};
    static const uint8_t WRITE_CYCLE_TIME_PER_BYTE{5};
    static const uint STRLEN_EEPROM{62};
    static const uint CRC_CHAR{2};

    void read_from_eeprom(uint16_t address, uint8_t *data, uint length);
    void write_to_eeprom(uint16_t address, const uint8_t *data, uint length);
    uint16_t crc16(const uint8_t *data_p, size_t length);
    std::shared_ptr<PicoI2C> mI2C;
    uint8_t mDevAddr;
    uint8_t mBuffer[STRLEN_EEPROM + REG_ADDR_LEN];
    uint16_t index;

};

//namespace EEPROM {
//
//    class SingleStorage {
//    public:
//        SingleStorage(std::shared_ptr<PicoI2C> i2c_sp, uint8_t data_size, uint8_t device_address = AT24C256_DEV_ADDRESS);
//        int get();
//        void put(int data);
//
//    private:
//        void apply_reg_addr();
//
//        static const uint8_t AT24C256_DEV_ADDRESS{0x50};
//        static const uint16_t AT24C256_TOP_ADDRESS{0xFFFF};
//        static const uint8_t BITS_PER_BYTE{8};
//        static const uint8_t REG_ADDR_LEN{2};
//
//        std::shared_ptr<PicoI2C> mI2C;
//        uint8_t mDevAddr;
//        uint8_t mBytes;
//        uint16_t mRegAddr;
//        uint8_t mBuffer[64 + REG_ADDR_LEN];
//    };
//
//}


#endif //FREERTOS_GREENHOUSE_EEPROM_H
