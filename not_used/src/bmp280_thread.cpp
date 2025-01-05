#include "app.hpp"

#include <thread>
#include <iostream>
#include <iomanip>

void bmp280_thread(Application_state_t  & appState, const BMP280::Config & bmp280Config) {
    try {
        BMP280 bmp280(bmp280Config);
        // sart detection
        std::cout << __func__ << " started." << std::endl;
        while (appState.keepRunning.load()) {
            float temperature = bmp280.getTemperature();
            float pressure = bmp280.getPressure();
            // std::cout << "Temperature: " << std::fixed << std::showpos << std::setprecision(1) << std::setw(6) << temperature << " C, \t";
            // std::cout << "Pressure: " << std::fixed << std::setprecision(1) << std::setw(6) << std::noshowpos << pressure << " hPa" << std::endl;
            appState.bmpTemperature.store(temperature);
            appState.bmpPressure.store(pressure);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in detection thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}