#include "pcf8563.hpp"

#include <stdexcept>
#include <iostream>
#include <iomanip>

PCF8563::PCF8563(const std::string &i2cBusDevice, uint8_t address)
    : i2cBus_(i2cBusDevice, address) {
    uint8_t control1 = 0x00;
    uint8_t control2 = 0x02;
    i2cBus_.write8(0x00, control1);
    i2cBus_.write8(0x01, control2);
}

PCF8563::~PCF8563() {
}

void PCF8563::setTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    //TODO: This function not quite correctly uses separate I2C transactions to write the time data.
    //      It should use a single I2C transaction to write all three bytes at once.
    //      Implement this feature. Please use the writeBlock() method of the I2CBus class.
    //      Represent data as a std::array<uint8_t, 3> and send it to the I2C bus.
    i2cBus_.write8(0x02, toBCD(seconds) & 0x7F);
    i2cBus_.write8(0x03, toBCD(minutes) & 0x7F);
    i2cBus_.write8(0x04, toBCD(hours) & 0x3F);
}

void PCF8563::setDate(uint8_t day,uint8_t month, uint8_t year) {
    // Zeller's Congruence algorithm to calculate the day of the week
    // https://en.wikipedia.org/wiki/Zeller%27s_congruence
    unsigned Y = 2000 + year;
    unsigned M = month ;
    if (M < 3) {
        M += 12;
        Y -= 1;
    }
    unsigned D = day;
    unsigned h = (D + 13*(M+1)/5 + Y + Y/4 - Y/100 + Y/400) % 7;
    uint8_t weekday = (h + 5) % 7; // Adjusting result to be 0 = Sunday, 1 = Monday, ..., 6 = Saturday

    i2cBus_.write8(0x05, toBCD(day & 0x3F));
    i2cBus_.write8(0x06, toBCD(weekday & 0x07));
    i2cBus_.write8(0x07, toBCD(month & 0x1F));
    i2cBus_.write8(0x08, toBCD(year));
}

std::array<uint8_t, 3> PCF8563::getTime() {
    //TODO: This function not quite correctly uses separate I2C transactions to read the time data.
    //      It should use a single I2C transaction to read all three bytes at once.
    //      Implement this feature. Please use the readBlock() method of the I2CBus class.
    //      Represent data as a std::array<uint8_t, 3>.
    return {fromBCD(i2cBus_.read8(0x02) & 0x7F),
            fromBCD(i2cBus_.read8(0x03) & 0x7F),
            fromBCD(i2cBus_.read8(0x04) & 0x3F)};
}

std::array<uint8_t, 4> PCF8563::getDate() {
    return {fromBCD(i2cBus_.read8(0x05) & 0x3F),
            fromBCD(i2cBus_.read8(0x06) & 0x07),
            fromBCD(i2cBus_.read8(0x07) & 0x1F),
            fromBCD(i2cBus_.read8(0x08) )};
}

void PCF8563::setAlarm(uint8_t hour, uint8_t minute, uint8_t day, uint8_t weekday) {
    i2cBus_.write8(0x09, toBCD(minute) & 0x7F);
    i2cBus_.write8(0x0A, toBCD(hour) & 0x3F);
    i2cBus_.write8(0x0B, toBCD(day) & 0x3F);
    i2cBus_.write8(0x0C, toBCD(weekday) & 0x07);
}

void PCF8563::clearAlarm() {
    uint8_t control2 = i2cBus_.read8(0x01);
    i2cBus_.write8(0x01, control2 & ~0x02);
}

uint8_t PCF8563::toBCD(uint8_t value) {
    return ((value / 10) << 4) | (value % 10);
}

uint8_t PCF8563::fromBCD(uint8_t value) {
    return ((value >> 4) * 10) + (value & 0x0F);
}

bool PCF8563::Start() {
    //TODO: Implement this function
    // Clear the STOP bit in the "Control/Status 1" register
    // Please study the datasheet and implement this feature
    std::cout << "PCF8563::Start() not implemented" << std::endl;
    std::cout << "Please study the datasheet and implement this feature in " << __func__  << "from " << __FILE__ << std::endl;
    return true;
}

bool PCF8563::Stop() {
    //TODO: Implement this function
    // Set the STOP bit in the "Control/Status 1" register
    // Please study the datasheet and implement this feature
    std::cout << "PCF8563::Stop() not implemented" << std::endl;
    std::cout << "Please study the datasheet and implement this feature in " << __func__ << "from " << __FILE__ << std::endl;
    return true;
}

void PCF8563::setTimeAndDate(const struct tm & wall) {
    setTime(wall.tm_hour, wall.tm_min, wall.tm_sec);
    setDate(wall.tm_mday, wall.tm_mon + 1, wall.tm_year - 100);
}

struct tm PCF8563::getTimeAndDate() {
    std::array<uint8_t, 3> pcfTime = getTime();
    std::array<uint8_t, 4> pcfDate = getDate();
    struct tm wall = {};
    wall.tm_sec = pcfTime[0];
    wall.tm_min = pcfTime[1];
    wall.tm_hour = pcfTime[2];
    wall.tm_mday = pcfDate[0];
    wall.tm_mon = pcfDate[2] - 1;
    wall.tm_year = pcfDate[3] + 100;
    // std::cout << "PCF8563: " ;
    // std::cout << std::dec << static_cast<int>(pcfDate[0]) << "-" ;
    // std::cout << static_cast<int>(pcfDate[2]) << "-" ;
    // std::cout << static_cast<int>(pcfDate[3]) << " " ;
    // std::cout << std::dec << std::setw(2) << std::setfill(' ') << static_cast<int>(pcfTime[2]) << ":" ;
    // std::cout << std::dec << std::setw(2) << std::setfill('0') << static_cast<int>(pcfTime[1]) << ":" ;
    // std::cout << std::dec << std::setw(2) << std::setfill('0') << static_cast<int>(pcfTime[0]) << std::endl;
    return wall;
}

