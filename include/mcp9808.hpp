#pragma once

#include "I2CBus.hpp"

class MCP9808 {
public:
    struct Config {
       std::string i2cBusDevice;   // "/dev/i2c-1"
        uint8_t i2cAddress;    // 0x18
    };
    MCP9808(const std::string &i2cBusDevice, int address = 0x18);
    ~MCP9808();

    float getTemperature();
    void setUpperAlarmTemperature(float temperature);
    void enableComparatorMode();
    uint16_t getManufacturerID();
    uint16_t getDeviceID();

private:
    I2CBus i2cBus_;

    static constexpr uint8_t MANUFACTURER_ID_REG = 0x06;
    static constexpr uint8_t DEVICE_ID_REG = 0x07;
    static constexpr uint8_t CONFIG_REG = 0x01;
    static constexpr uint8_t TEMP_REG = 0x05;
    static constexpr uint8_t TUPPER_REG = 0x02;
    static constexpr uint8_t TLOWER_REG = 0x03;
    static constexpr uint8_t TCRIT_REG = 0x04;

    // Expected Manufacturer and Device ID
    static constexpr uint16_t EXPECTED_MANUFACTURER_ID = 0x0054;
    static constexpr uint16_t EXPECTED_DEVICE_ID = 0x0400; // Example device ID

    float convertRawTemperature(uint16_t rawTemp);
};

