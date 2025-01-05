#pragma once

#include <atomic>

#include "GPIO_config.hpp"
#include "st7789v2.hpp"
#include "GPIO_Led.hpp"
#include "BMP280.hpp"
#include "PWM_Backlight.hpp"
#include "PWM_Servo.hpp"
#include "mcp9808.hpp"
#include "pcf8563.hpp"
#include <time.h>

// Hardware configuration structure
typedef struct {
    ST7789::Config displayConfig;
    BMP280::Config bmp280Config;
    MCP9808::Config mcp9808Config;
    PCF8563::Config pcf8563Config;
    GPIO_Led::Config LED;
    std::string buttonEvents;
    GPIO_config rotary_SIA;
    GPIO_config rotary_SIB;
    GPIO_config rotary_SW;
    PWM_Backlight::Config PWM_BL;
    PWM_Servo::Config PWM_Srv;
} Hardware_config_t;

// Application state structure 
typedef struct {
    std::atomic<bool> keepRunning;
    std::atomic<bool> setAlarm;
    std::atomic<std::chrono::steady_clock::time_point> alarmTime;
    std::atomic<int> tempThreshold; // in Celsius, temperature threshold for alarm
    std::atomic<float> mcpTemperature; // in Celsius, temperature measured by the sensor
    std::atomic<struct tm> pcfTime;
    std::atomic<bool> gpioButtonShortPress;
    std::atomic<bool> rotaryButtonShortPress;
    std::atomic<bool> rotaryButtonLongPress;
    const unsigned int alarmDuration_ms = 20000;
    const int tempThresholdDefault = 28;
} Application_state_t ;