#pragma once

#include "I2CBus.hpp"
#include <time.h>

class PCF8563 {
public:
    struct Config {
        std::string i2cBusDevice;   // "/dev/i2c-1"
        uint8_t i2cAddress;    // 0x51
    };
    PCF8563(const std::string &i2cBusDevice, uint8_t address = 0x51);
    ~PCF8563();

    void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void setDate(uint8_t day, uint8_t month, uint8_t year);
    std::array<uint8_t, 3> getTime();
    std::array<uint8_t, 4> getDate();
    void setAlarm(uint8_t hour, uint8_t minute, uint8_t day = 0x80, uint8_t weekday = 0x80);
    void clearAlarm();
    bool Start();
    bool Stop();
    void setTimeAndDate(const struct tm & wall);
    struct tm getTimeAndDate();

private:
    I2CBus i2cBus_;

    static uint8_t toBCD(uint8_t value);
    static uint8_t fromBCD(uint8_t value);
};
