/* ***************************************************************** 

Exercise 6: The use of I2C bus
by Piotr Zawadzki

This application demonstrates the utilization of the I2C bus to communicate with peripherals. 

Using the I2C bus, the following devices are connected:
- MCP9808 temperature sensor,
- RTC PCF8663 real-time clock.

Display content

- Top left corner: current temperature,
- Top right corner: set value in the range of 0-59 set by the rotary encoder,
- First line on the left: current time set in the RTC,
- First line on the right: current date set in the RTC,
- Bottom left part of the screen: system time `target`,
- Bottom right part of the screen: system date `target`.

Events

- Long press of the button -- stop the application.
- Short press of the button -- copy the system time to the RTC,
- Rotary encoder - change the set value,
- Set value below the temperature triggers an alarm,
- Set the value 60 stops RTC,
- Set the value 59 starts RTC, 
- Short press of the rotary encoder button - copy the set value to the RTC seconds field,
- Long press of the rotary encoder button - set the system time and date `target` based on the RTC content,
- The application can be terminated with the combination _Ctrl+C_. **Note! _Ctrl+C_ does not work in the Visual Studio Code terminal**.

Exercise:

- Follow the TODO-s in the code to complete the exercise.

 **************************************************************** */

#include "app.hpp"
#include <csignal>     // for signal, SIGINT
#include <thread>      // for std::thread
#include <fcntl.h>     // for open
#include <sys/ioctl.h> // for ioctl
#include <sys/time.h>  // for settimeofday
#include <iomanip>     // for std::setw, std::setfill

// Hardware configuration
Hardware_config_t hardwareConfig = {
    .displayConfig = {
        .spiDevice = "/dev/spidev0.0",
        .speedHz = 30000000,  // 30 MHz, in theory ST7789v2 should support up to 62.5 MHz
        .gpioChip = "gpiochip0",
        .dcPin = 22,
        .resetPin = 27
    }
    ,
    .bmp280Config = {
        .spiDevice = "/dev/spidev0.1",
        .speedHz = 100000,  // 100kHz, in theory BMP280 should support up to 10.0 MHz
    }
    ,
    .mcp9808Config = {
        .i2cBusDevice = "/dev/i2c-1",
        .i2cAddress = 0x18
    }
    ,
    .pcf8563Config = {
        .i2cBusDevice = "/dev/i2c-1",
        .i2cAddress = 0x51
    }
    ,
    .LED = {"lwsw-led"}
    ,
    .buttonEvents = "/dev/input/event0"
    ,
    .rotary_SIA = {
        .chipName = "gpiochip0",
        .lineNum = 16,
        .lineRequest = {
            .consumer = "rotary_SIA",
            .request_type = gpiod::line_request::EVENT_BOTH_EDGES,
            .flags = gpiod::line_request::FLAG_BIAS_PULL_DOWN
        }
    }
    ,
    .rotary_SIB = {
        .chipName = "gpiochip0",
        .lineNum = 20,
        .lineRequest = {
            .consumer = "rotary_SIB",
            .request_type = gpiod::line_request::DIRECTION_INPUT,
            .flags = gpiod::line_request::FLAG_BIAS_PULL_DOWN
        }
    }
    ,
    .rotary_SW = {
        .chipName = "gpiochip0",
        .lineNum = 21,
        .lineRequest = {
            .consumer = "rotary_SW",
            .request_type = gpiod::line_request::EVENT_BOTH_EDGES,
            .flags = gpiod::line_request::FLAG_BIAS_PULL_DOWN
        }
    }
    ,
    .PWM_BL = {
        .pwmChip=2, 
        .pwmChannel=3
    }
    ,
    .PWM_Srv = {
        .pwmChip=2, 
        .pwmChannel=2, 
        .minAngle=-45, 
        .maxAngle=45
    }
};

// Global variable for synchronization and state sharing
Application_state_t appState {
    .keepRunning = true,
    .setAlarm = false,
    .alarmTime = std::chrono::steady_clock::time_point::min(),
    .tempThreshold = 28,
    .mcpTemperature = 0.0,
    .pcfTime = {},
    .gpioButtonShortPress = false,
    .rotaryButtonShortPress = false,
    .rotaryButtonLongPress = false
};

// Graceful program close on Ctrl+C
void sigint_handler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    appState.keepRunning.store(false);
}

// Prototypes of threads
void button_thread( Application_state_t  & appState, const std::string & inputDevice) ;
void rotary_encoder_thread(Application_state_t  & appState, const GPIO_config & SIA_config, const GPIO_config & SIB_config) ;
void rotary_button_thread( Application_state_t  & appState, const GPIO_config & SW_config) ;
void gpio_led_thread(Application_state_t  & appState, const GPIO_Led::Config & led_config) ;
void pwmBacklight_thread(Application_state_t  & appState, const PWM_Backlight::Config & pwmBacklight_config) ;
void servo_thread(Application_state_t  & appState, const PWM_Servo::Config & pwmServo_config) ;
void display_thread( Application_state_t  & appState, const ST7789::Config & displayConfig) ;
void bmp280_thread( Application_state_t  & appState, const BMP280::Config & bmp280Config) ;
void mcp9808_thread( Application_state_t  & appState, const MCP9808::Config & mcp9808Config) ;
void pcf8563_thread( Application_state_t  & appState, const PCF8563::Config & pcf8563Config) ;

void test_i2c(const MCP9808::Config & mcp9808Config, const PCF8563::Config & pcf8563Config) {
    MCP9808 mcp9808(mcp9808Config.i2cBusDevice, mcp9808Config.i2cAddress);
    PCF8563 pcf8563(pcf8563Config.i2cBusDevice, pcf8563Config.i2cAddress);

    int id = mcp9808.getDeviceID();
    float temperature = mcp9808.getTemperature();
    std::cout << "MCP9808 Device ID: 0x" << std::setw(4) << std::setfill('0') << std::hex << id << " Temp: " << temperature << std::endl;

    pcf8563.setTime(12, 59, 56);
    pcf8563.setDate(5, 1, 24);
    std::array<uint8_t, 4> date = pcf8563.getDate();
    std::cout << "PCF8563 Date: " << std::dec << static_cast<int>(date[0]) << "-" << static_cast<int>(date[2]) << "-" << static_cast<int>(date[3]) << " DOW: " << static_cast<int>(date[1]) << std::endl;

    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::array<uint8_t, 3> time = pcf8563.getTime();
        std::cout << std::setw(2) << std::setfill('0') << std::dec << static_cast<int>(time[2]) << ":" ;
        std::cout << std::setw(2) << std::setfill('0') << std::dec << static_cast<int>(time[1]) << ":" ;
        std::cout << std::setw(2) << std::setfill('0') << std::dec << static_cast<int>(time[0]) ;
        std::cout << std::endl;
    }
}

//main thread
int main() {
    // test_i2c(hardwareConfig.mcp9808Config, hardwareConfig.pcf8563Config);
    // return 0;
    try {
        signal(SIGINT, sigint_handler); // Register signal handler for Ctrl+C
        std::thread display_task( display_thread, std::ref(appState), hardwareConfig.displayConfig) ;
        // std::thread bmp280_task( bmp280_thread, std::ref(appState), hardwareConfig.bmp280Config) ;
        std::thread mcp9808_task( mcp9808_thread, std::ref(appState), hardwareConfig.mcp9808Config) ;
        std::thread pcf8563_task( pcf8563_thread, std::ref(appState), hardwareConfig.pcf8563Config) ;
        std::thread lwsw_button_task( button_thread, std::ref(appState), hardwareConfig.buttonEvents) ;
        std::thread rotary_task( rotary_encoder_thread, std::ref(appState), hardwareConfig.rotary_SIA, hardwareConfig.rotary_SIB) ;
        std::thread rotary_button_task( rotary_button_thread, std::ref(appState), hardwareConfig.rotary_SW) ;
        std::thread alarm_led_task( gpio_led_thread, std::ref(appState), hardwareConfig.LED ) ;
        std::thread servo_task( servo_thread, std::ref(appState), hardwareConfig.PWM_Srv) ;
        std::thread displayBacklight_task( pwmBacklight_thread, std::ref(appState), hardwareConfig.PWM_BL) ;        

        unsigned int counter = 0;
        bool rtc_is_running = true;
        int mcpTemperatureMessage=-1 ;
        while (appState.keepRunning.load()) {
            counter = (counter + 1) % 10 ; 
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (appState.mcpTemperature.load() > appState.tempThreshold.load() && mcpTemperatureMessage != 1) { // Alarm condition
                std::cout << "ALARM! Temperature " << appState.mcpTemperature.load() << "C above threshold" << std::endl;
                mcpTemperatureMessage = 1;
                appState.setAlarm.store(true);
            } 
            if (appState.mcpTemperature.load() <= appState.tempThreshold.load() && mcpTemperatureMessage != 0) {
                mcpTemperatureMessage = 0;
                std::cout << "Temperature " << appState.mcpTemperature.load() << "C below threshold. Normal operation" << std::endl;
                appState.setAlarm.store(false);
            }
            if (appState.gpioButtonShortPress.load()) {
                appState.gpioButtonShortPress.store(false);
                time_t sys_time = std::time(nullptr);
                struct tm sys_time_tm = *std::localtime(&sys_time);
                PCF8563 pcf8563(hardwareConfig.pcf8563Config.i2cBusDevice, hardwareConfig.pcf8563Config.i2cAddress);
                pcf8563.setTimeAndDate(sys_time_tm);
                sys_time_tm = pcf8563.getTimeAndDate();
                appState.pcfTime.store(sys_time_tm);
            }
            if (appState.rotaryButtonShortPress.load()) {
                appState.rotaryButtonShortPress.store(false);
                std::cout << "Rotary button short press" << std::endl;
                PCF8563 pcf8563(hardwareConfig.pcf8563Config.i2cBusDevice, hardwareConfig.pcf8563Config.i2cAddress);
                auto rtc_time = pcf8563.getTime();
                pcf8563.setTime(rtc_time[2], rtc_time[1], appState.tempThreshold.load());
            }
            if (appState.rotaryButtonLongPress.load()) {
                appState.rotaryButtonLongPress.store(false);
                std::cout << "Rotary button long press" << std::endl;
                PCF8563 pcf8563(hardwareConfig.pcf8563Config.i2cBusDevice, hardwareConfig.pcf8563Config.i2cAddress);
                auto new_sys_time_tm = pcf8563.getTimeAndDate();
                std::time_t new_sys_time = std::mktime(&new_sys_time_tm);
                struct timeval tv = {new_sys_time, 0};
                if (settimeofday(&tv, NULL) != 0) {
                    std::cerr << "Failed to set system time: " << strerror(errno) << std::endl;
                } else {
                    std::cout << "System time updated successfully" << std::endl;
                }
            }
            int tempThreshold = appState.tempThreshold.load();
            if (tempThreshold == 60 && rtc_is_running) {
                rtc_is_running = false;
                PCF8563 pcf8563(hardwareConfig.pcf8563Config.i2cBusDevice, hardwareConfig.pcf8563Config.i2cAddress);
                pcf8563.Stop();
                std::cout << "PCF8563 stopped" << std::endl;
            } else if (tempThreshold == 59 && !rtc_is_running) {
                rtc_is_running = true;
                PCF8563 pcf8563(hardwareConfig.pcf8563Config.i2cBusDevice, hardwareConfig.pcf8563Config.i2cAddress);
                pcf8563.Start();
                std::cout << "PCF8563 started" << std::endl;
            }
        }

        std::cout << "Main thread: waiting for child threads stop." << std::endl;
        displayBacklight_task.join();
        servo_task.join();
        alarm_led_task.join();
        rotary_button_task.join();
        rotary_task.join();
        lwsw_button_task.join() ;
        // bmp280_task.join();
        pcf8563_task.join();
        mcp9808_task.join();
        display_task.join();
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in main thread: " << e.what() << std::endl;
    }
    std::cout << "Application gracefully stopped." << std::endl;
    return 0;
}
