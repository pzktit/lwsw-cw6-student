#include "BMP280.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cmath>

BMP280::BMP280(const Config& config)
    : spi_fd(-1)
    , spi_device(config.spiDevice)
    , spi_mode(SPI_MODE_0)
    , spi_bits(8)
    , spi_speed(config.speedHz) {

    // Open the SPI device
    spi_fd = open(spi_device.c_str(), O_RDWR);
    if (spi_fd < 0) {
        throw std::runtime_error("Failed to open SPI device");
    }

    // Configure SPI mode, bits per word, and speed
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) == -1) {
        close(spi_fd);
        throw std::runtime_error("Failed to configure SPI device");
    }

    // get sensor ID
    uint8_t id = read8(0xD0);
    if (id != 0x58) {
        throw std::runtime_error("BMP280 sensor not found");
    }

    // Initialize BMP280
    initializeSensor();
}

BMP280::~BMP280() {
    if (spi_fd >= 0) {
        close(spi_fd);
    }
}

float BMP280::getTemperature() {
    int32_t adc_T = read24(0xFA);
    adc_T >>= 4;
    int32_t var1 = ((((adc_T >> 3) - (dig_T1 << 1))) * dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

float BMP280::getPressure() {
    int32_t adc_P = read24(0xF7);
    adc_P >>= 4;
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0) {
        throw std::runtime_error("Division by zero in pressure calculation");
    }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (float)p / 25600.0f;
}

void BMP280::initializeSensor() {
    // Read calibration data from BMP280
    dig_T1 = read16(0x88);
    dig_T2 = read16(0x8A);
    dig_T3 = read16(0x8C);
    dig_P1 = read16(0x8E);
    dig_P2 = read16(0x90);
    dig_P3 = read16(0x92);
    dig_P4 = read16(0x94);
    dig_P5 = read16(0x96);
    dig_P6 = read16(0x98);
    dig_P7 = read16(0x9A);
    dig_P8 = read16(0x9C);
    dig_P9 = read16(0x9E);

    // Configure BMP280 for temperature and pressure measurement
    write8(0xF4, 0x27); // Normal mode, temperature and pressure oversampling x1
    write8(0xF5, 0xA0); // Config register with standby and filter settings
}

uint8_t BMP280::read8(uint8_t reg) {
    uint8_t tx[] = { static_cast<uint8_t>(reg | 0x80), 0 };
    uint8_t rx[2] = { 0 };
    spiTransfer(tx, rx, sizeof(tx));
    return rx[1];
}

uint16_t BMP280::read16(uint8_t reg) {
    uint8_t tx[] = { static_cast<uint8_t>(reg | 0x80), 0, 0 };
    uint8_t rx[3] = { 0 };
    spiTransfer(tx, rx, sizeof(tx));
    return (rx[2] << 8) | rx[1];
}

uint32_t BMP280::read24(uint8_t reg) {
    uint8_t tx[] = { static_cast<uint8_t>(reg | 0x80), 0, 0, 0 };
    uint8_t rx[4] = { 0 };
    spiTransfer(tx, rx, sizeof(tx));
    return (rx[1] << 16) | (rx[2] << 8) | rx[3];
}

void BMP280::write8(uint8_t reg, uint8_t value) {
    uint8_t tx[] = { static_cast<uint8_t>(reg & 0x7F), value };
    uint8_t rx[2] = { 0 };
    spiTransfer(tx, rx, sizeof(tx));
}

void BMP280::spiTransfer(uint8_t *tx, uint8_t *rx, size_t length) {
    struct spi_ioc_transfer spi = {};
    spi.tx_buf = reinterpret_cast<unsigned long>(tx);
    spi.rx_buf = reinterpret_cast<unsigned long>(rx);
    spi.len = length;
    spi.speed_hz = spi_speed;
    spi.bits_per_word = spi_bits;
    spi.cs_change = 0;
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi) < 0) {
        throw std::runtime_error("SPI transfer failed");
    }
}
