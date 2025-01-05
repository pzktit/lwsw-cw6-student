#include "I2CBus.hpp"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>

// Class definition
I2CBus::I2CBus(const std::string &deviceFile, uint8_t address_) : address_(address_) {
    fd_ = open(deviceFile.c_str(), O_RDWR);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open I2C device file: " + std::string(strerror(errno)));
    }
}

I2CBus::~I2CBus() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

void I2CBus::write8(uint8_t reg, uint8_t value) {
    std::array<uint8_t, 1> data = {value};
    writeBlock(reg, data);
}

uint8_t I2CBus::read8(uint8_t reg) {
    std::array<uint8_t, 1> valueBuffer;
    readBlock(reg, valueBuffer);
    return valueBuffer[0];
}

void I2CBus::write16(uint8_t reg, uint16_t value) {
    std::array<uint8_t, 2> data = {static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF)};
    writeBlock(reg, data);
}

uint16_t I2CBus::read16(uint8_t reg) {
    std::array<uint8_t, 2> valueBuffer;
    readBlock(reg, valueBuffer);
    return (static_cast<uint16_t>(valueBuffer[0]) << 8) | valueBuffer[1];
}
