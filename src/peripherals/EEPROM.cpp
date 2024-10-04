#include "EEPROM.h"
#include "Logger.h"

#include <utility>

EEPROM::EEPROM(std::shared_ptr<PicoI2C> i2c_sp, uint8_t device_address):
mI2C(i2c_sp), mDevAddr(device_address) {}


void EEPROM::read_from_eeprom(uint16_t address, uint8_t *data, uint length){
    uint8_t buf[REG_ADDR_LEN];
    buf[0] = static_cast<uint8_t>((address & 0xFF00) >> BITS_PER_BYTE);
    buf[1] = static_cast<uint8_t>(address & 0x00FF);
    mI2C->transaction(mDevAddr, buf, REG_ADDR_LEN, data, length);
}

void EEPROM::write_to_eeprom(uint16_t address, const uint8_t *data, uint length){
    mBuffer[0] = static_cast<uint8_t>((address & 0xFF00) >> BITS_PER_BYTE);
    mBuffer[1] = static_cast<uint8_t>(address & 0x00FF);
    for (int i = 0; i < length; ++i) {
        mBuffer[i + REG_ADDR_LEN] = data[i];
    }
    mI2C->write(mDevAddr, mBuffer, length + REG_ADDR_LEN);
    vTaskDelay(WRITE_CYCLE_TIME_PER_BYTE * length);
}

void EEPROM::put(uint16_t address, uint16_t number){
    uint8_t data[2 + CRC_CHAR];
    data[0] = static_cast<uint8_t>((number >> BITS_PER_BYTE) & 0xFF);
    data[1] = static_cast<uint8_t>(number & 0xFF);
    uint16_t crc = crc16(data, 2);
    data[2] = static_cast<uint8_t>((crc >> BITS_PER_BYTE) & 0xFF);
    data[3] = static_cast<uint8_t>(crc & 0xFF);
    write_to_eeprom(address, data, 2 + CRC_CHAR);
}

bool EEPROM::get(uint16_t address, uint16_t &val){
    uint8_t data[2 + CRC_CHAR];
    read_from_eeprom(address, data, 2 + CRC_CHAR);
    uint16_t crc = crc16(data, 2);
    uint16_t read_crc = (static_cast<uint16_t>(data[2]) << 8) | data[3];
    if(crc!= read_crc){
        Logger::log("Fail to get number from EEPROM\n");
        return false;
    }
    val = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    return true;
}

void EEPROM::put(uint16_t address, const std::string& str) {
    const char* str_ = str.c_str();
    size_t string_length = strlen(str_) + 1;
    if (string_length > STRLEN_EEPROM) {
        string_length = STRLEN_EEPROM;
    }
    uint8_t data_str[string_length + CRC_CHAR];
    for (int a = 0; a < strlen(str_); ++a) {
        data_str[a] = (uint8_t) str[a];
    }
    data_str[strlen(str_)] = '\0';
    uint16_t crc = crc16(data_str, string_length);
    data_str[string_length] = static_cast<uint8_t>((crc >> BITS_PER_BYTE) & 0xFF);
    data_str[string_length + 1] = static_cast<uint8_t>(crc & 0xFF);
    write_to_eeprom(address, data_str, ENTRY_SIZE);
}

bool EEPROM::get_str(uint16_t address, std::string &out_str){
    uint8_t read_buff[ENTRY_SIZE];
    read_from_eeprom(address, (uint8_t *) &read_buff, ENTRY_SIZE);
    int term_zero_index = 0;
    while (read_buff[term_zero_index] != '\0') {
        term_zero_index++;
    }
    if (read_buff[0] != 0 && crc16(read_buff, (term_zero_index + 3)) == 0 && term_zero_index < (ENTRY_SIZE - 2)) {
        out_str = std::string(reinterpret_cast<char*>(read_buff), term_zero_index);
//        Logger::log("%s\n", out_str.c_str());
        return true;
    }else{
        out_str.clear();
        Logger::log("crc failed, return empty string\n");
        return false;
    }
}

uint16_t EEPROM::crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;
    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 5) ^ ((uint16_t) x));
    }
    return crc;
}

void EEPROM::put_log_entry(const char *str) {
    if (log_index >= MAX_ENTRIES) {
        Logger::log("Maximum log entries. Erasing the log to log the messages\n");
        erase_logs();
    }
    size_t string_length = strlen(str) + 1; //include NULL terminator
    if (string_length > STRLEN_EEPROM) {
        string_length = STRLEN_EEPROM;
    }
    uint8_t log_buf[string_length + CRC_CHAR];
    for (int a = 0; a < strlen(str); ++a) {
        log_buf[a] = (uint8_t) str[a];
    }
    log_buf[strlen(str)] = '\0';
    uint16_t crc = crc16(log_buf, string_length);
    log_buf[string_length] = (uint8_t)(crc >> BITS_PER_BYTE);
    log_buf[string_length + 1] = (uint8_t)crc;

    uint16_t write_address = (uint16_t) EEPROM::LOG_FIRST_ADDR + (log_index * (uint16_t) ENTRY_SIZE);
    if (write_address < (uint16_t)EEPROM::LOG_FIRST_ADDR + ((uint16_t)ENTRY_SIZE * (uint16_t) MAX_ENTRIES)) {
        write_to_eeprom(write_address, log_buf, ENTRY_SIZE);
        log_index += 1;
        put(LOG_INDEX_ADDR, log_index);
    }else{
        Logger::log("Address %u is exceeded the log entries. Unable to write to EEPROM\n", write_address);
    }
}

// read all the log entries that are valid from eeprom using crc to check
void EEPROM::get_log_entry() {
    Logger::log("Reading log entry\n");
    for (int idx = 0; idx < log_index; ++idx) {
        uint8_t read_buff[ENTRY_SIZE];
        uint16_t read_address = (uint16_t) EEPROM::LOG_FIRST_ADDR + (idx * (uint16_t) ENTRY_SIZE);
        read_from_eeprom(read_address, (uint8_t *) &read_buff, ENTRY_SIZE);
        int term_zero_index = 0;
        while (read_buff[term_zero_index] != '\0') {
            term_zero_index++;
        }
        if (read_buff[0] != 0 && crc16(read_buff, (term_zero_index + 3)) == 0 && term_zero_index < (ENTRY_SIZE - 2)) {
            std::string log_str(reinterpret_cast<char*>(read_buff), term_zero_index);
            Logger::log("%s\n", log_str.c_str());
        } else {
            Logger::log("Invalid or empty entry at index %d\n", log_index);
            break;
        }
    }
}

// erase all log entries in eeprom
void EEPROM::erase_logs() {
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        uint16_t write_address = EEPROM::LOG_FIRST_ADDR + (i * ENTRY_SIZE);
        uint8_t buf[] = {00};
        write_to_eeprom(write_address, buf, 1);
    }
    log_index = 0;
    put(EEPROM::LOG_INDEX_ADDR, log_index);
}

void EEPROM::set_log_index_value() {
    uint16_t index;
    if(get(EEPROM::LOG_INDEX_ADDR, index) && index <= MAX_ENTRIES && index >= 0){
        Logger::log("index get in eeprom is %u\n", index);
        log_index = index;
        Logger::log("log index get in eeprom is %u\n", log_index);
    }else{
        log_index = 0;
    }
}
