#include "app.hpp"

#include <thread>
#include <iostream>
#include <iomanip>

void mcp9808_thread(Application_state_t  & appState, const MCP9808::Config & mcp9808Config) {
    try {
        MCP9808 mcp9808(mcp9808Config.i2cBusDevice, mcp9808Config.i2cAddress);
        std::cout << __func__ << " started." << std::endl;
        while (appState.keepRunning.load()) {
            float temperature = mcp9808.getTemperature();
            appState.mcpTemperature.store(temperature);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in detection thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
