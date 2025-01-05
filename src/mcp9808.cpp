#include "mcp9808.hpp"

#include <stdexcept>
#include <fcntl.h> // for open()
#include <sys/ioctl.h> // for ioctl()
#include <unistd.h> // for close()
#include <cmath> // for rounding functions
#include <linux/i2c-dev.h> // for I2C constants


MCP9808::MCP9808(const std::string &i2cBusDevice, int address) : i2cBus_(i2cBusDevice, address) {

    if (getManufacturerID() != EXPECTED_MANUFACTURER_ID) {
        throw std::runtime_error("MCP9808: Invalid manufacturer ID");
    }

    if (getDeviceID() != EXPECTED_DEVICE_ID) {
        throw std::runtime_error("MCP9808: Invalid device ID");
    }
}

MCP9808::~MCP9808() {
}


uint16_t MCP9808::getManufacturerID() {
    return i2cBus_.read16(MANUFACTURER_ID_REG);
}

uint16_t MCP9808::getDeviceID() {
    return i2cBus_.read16(DEVICE_ID_REG) & 0xFF00;
}


float MCP9808::convertRawTemperature(uint16_t rawTemp) {
    rawTemp &= 0x1FFF; // Mask to get only temperature bits
    float temperature = rawTemp * 0.0625f;
    if (rawTemp & 0x1000) { // Check if negative
        temperature -= 256.0f;
    }
    return temperature;
}

float MCP9808::getTemperature() {
    uint16_t rawTemp = i2cBus_.read16(TEMP_REG);
    return convertRawTemperature(rawTemp);
}

void MCP9808::setUpperAlarmTemperature(float temperature) {
    if (temperature < -40.0 || temperature > 125.0) {
        throw std::out_of_range("Temperature out of range");
    }
    uint16_t rawTemp = static_cast<uint16_t>(std::round(temperature / 0.0625));
    i2cBus_.write16(TUPPER_REG, rawTemp);
}

void MCP9808::enableComparatorMode() {
    uint16_t config = i2cBus_.read16(CONFIG_REG);
    config &= ~(1 << 9); // Clear interrupt mode bit (set comparator mode)
    i2cBus_.write16(CONFIG_REG, config);
}
