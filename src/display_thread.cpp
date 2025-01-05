#include "app.hpp"

#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

void display_thread( Application_state_t  & appState, const ST7789::Config & displayConfig) {
    try {
        ST7789 display(displayConfig);
        display.clearScreen(ST7789::Colors::BLACK);
        display.showLogo();
        static float temperature = -273;
        static float tempThreshold = -99;
        time_t last_time = std::numeric_limits<time_t>::min();
        time_t last_sys_time = std::numeric_limits<time_t>::min();
        std::cout << __func__ << " started." << std::endl;
        while (appState.keepRunning.load()) {
            float new_bmpTemperature = appState.mcpTemperature.load();
            if (new_bmpTemperature != temperature) { // update temperature readings only when required
                temperature = new_bmpTemperature;
                std::ostringstream oss;
                oss << std::fixed << std::showpos << std::setprecision(1) << std::setw(6) << std::setfill(' ') << temperature << " C";
                if (appState.setAlarm.load()) {
                    display.drawString(0, 16, oss.str(), ST7789::Colors::RED, ST7789::Colors::BLACK);
                } else {
                    display.drawString(0, 16, oss.str(), ST7789::Colors::WHITE, ST7789::Colors::BLACK);
                }
            }
            int new_tempThreshold = appState.tempThreshold.load();
            if (new_tempThreshold != tempThreshold) { // update temperature threshold only when required
                tempThreshold = new_tempThreshold;
                std::ostringstream oss;
                oss << std::fixed << std::showpos << std::setprecision(0) << std::setw(3) << std::setfill(' ') << tempThreshold << " ";
                display.drawString(240-(3)*16, 16, oss.str(), ST7789::Colors::WHITE, ST7789::Colors::BLACK);

            }

            // Update time only when required
            auto pcfTime = appState.pcfTime.load();
            auto new_time = std::mktime(&pcfTime);
            if (new_time != last_time) {
                last_time = new_time;
                std::ostringstream oss;
                oss << std::put_time(&pcfTime, "%H:%M:%S");
                display.drawString(0, 40, oss.str(), ST7789::Colors::WHITE, ST7789::Colors::BLACK);
                oss.str("");
                oss << " " << std::put_time(&pcfTime, "%d-%m-%y");
                display.drawString(160, 40, oss.str(), ST7789::Colors::WHITE, ST7789::Colors::BLACK);
            }
            time_t sys_time = std::time(nullptr);
            if (sys_time != last_sys_time) {
                last_sys_time = sys_time;
                std::ostringstream oss;
                oss << std::put_time(std::localtime(&sys_time), "%H:%M:%S");
                display.drawString(0, 230, oss.str(), ST7789::Colors::WHITE, ST7789::Colors::BLACK);
                oss.str("");
                oss << " " << std::put_time(std::localtime(&sys_time), "%d-%m-%y");
                display.drawString(160, 230, oss.str(), ST7789::Colors::WHITE, ST7789::Colors::BLACK);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        display.clearScreen( ST7789::Colors::BLACK );
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in display thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
