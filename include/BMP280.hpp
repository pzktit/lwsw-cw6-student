#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <linux/spi/spidev.h>

class BMP280 {
public:
    struct Config {
        std::string spiDevice;   // e.g., "/dev/spidev0.1"
        unsigned int speedHz;    // SPI speed in Hz
    };

    explicit BMP280(const Config& config);
    ~BMP280();
    
    // prevent copying
    BMP280(const BMP280&) = delete;
    BMP280& operator=(const BMP280&) = delete;

    float getTemperature();
    float getPressure();

private:
    int spi_fd;
    std::string spi_device;
    uint8_t spi_mode;
    uint8_t spi_bits;
    uint32_t spi_speed;

    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    int32_t t_fine;

    void initializeSensor();

    uint8_t read8(uint8_t reg);
    uint16_t read16(uint8_t reg);
    uint32_t read24(uint8_t reg);
    void write8(uint8_t reg, uint8_t value);
    void spiTransfer(uint8_t *tx, uint8_t *rx, size_t length);
};
