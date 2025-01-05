#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <stdexcept>

class I2CBus {
public:
    // Constructor
    I2CBus(const std::string &deviceFile, uint8_t deviceAddress);

    // Destructor
    ~I2CBus();

    // Write 8-bit value to a register
    void write8(uint8_t reg, uint8_t value);

    // Read 8-bit value from a register
    uint8_t read8(uint8_t reg);

    // Write 16-bit value to a register
    void write16(uint8_t reg, uint16_t value);

    // Read 16-bit value from a register
    uint16_t read16(uint8_t reg);

    // Write a block of data to a register
    template <size_t N>
    void writeBlock(uint8_t reg, const std::array<uint8_t, N> &data);

    // Read a block of data from a register
    template <size_t N>
    void readBlock(uint8_t reg, std::array<uint8_t, N> &data);

private:
    int fd_;
    uint8_t address_;
};

template <size_t N>
void I2CBus::writeBlock(uint8_t reg, const std::array<uint8_t, N> &data) {
    struct i2c_msg messages[1];
    std::array<uint8_t, N + 1> buffer;
    buffer[0] = reg;
    std::copy(data.begin(), data.end(), buffer.begin() + 1);

    messages[0].addr = address_;
    messages[0].flags = 0;
    messages[0].len = buffer.size();
    messages[0].buf = buffer.data();

    struct i2c_rdwr_ioctl_data ioctlData;
    ioctlData.msgs = messages;
    ioctlData.nmsgs = 1;

    if (ioctl(fd_, I2C_RDWR, &ioctlData) < 0) {
        throw std::runtime_error("Failed to write block of data: " + std::string(strerror(errno)));
    }
}

template <size_t N>
void I2CBus::readBlock(uint8_t reg, std::array<uint8_t, N> &data) {
    struct i2c_msg messages[2];
    uint8_t regBuffer = reg;

    messages[0].addr = address_;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &regBuffer;

    messages[1].addr = address_;
    messages[1].flags = I2C_M_RD;
    messages[1].len = data.size();
    messages[1].buf = data.data();

    struct i2c_rdwr_ioctl_data ioctlData;
    ioctlData.msgs = messages;
    ioctlData.nmsgs = 2;

    if (ioctl(fd_, I2C_RDWR, &ioctlData) < 0) {
        throw std::runtime_error("Failed to read block of data: " + std::string(strerror(errno)));
    }
}

