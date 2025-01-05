#include "app.hpp"

#include <thread>
#include <iostream>
#include <iomanip>

void pcf8563_thread(Application_state_t  & appState, const PCF8563::Config & pcf8563Config) {
    try {
        PCF8563 pcf8563(pcf8563Config.i2cBusDevice, pcf8563Config.i2cAddress);

        std::cout << __func__ << " started." << std::endl;
        while (appState.keepRunning.load()) {
            struct tm pcf8563_time = pcf8563.getTimeAndDate();
            appState.pcfTime.store(pcf8563_time);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in detection thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}