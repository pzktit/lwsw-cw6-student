#include "app.hpp"
#include <thread>


void pwmBacklight_thread(Application_state_t  & appState, const PWM_Backlight::Config & pwmBacklight_config) {
    try {
        const int defaultBrightness = 100;
        PWM_Backlight pwmBacklight(pwmBacklight_config);
        std::cout << __func__ << " started." << std::endl;
        pwmBacklight.setBrightness(defaultBrightness);
        while (appState.keepRunning.load()) {
            if (appState.setAlarm.load()) { // enter alarm state
                const auto loop_time_step = std::chrono::milliseconds(10); 
                auto now = std::chrono::steady_clock::now();
                int brightness_direction = 1; // 1 = increase, -1 = decrease
                const int brightness_step = 2;
                int brightness = defaultBrightness;
                while (appState.keepRunning.load() && appState.setAlarm.load() && now < appState.alarmTime.load()) { 
                    std::this_thread::sleep_for(loop_time_step);
                    now = std::chrono::steady_clock::now();
                    brightness += brightness_direction * brightness_step;
                    if (brightness >= 100) {
                        brightness = 100;
                        brightness_direction = -1;
                    } else if (brightness <= 0) {
                        brightness = 0;
                        brightness_direction = 1;
                    }
                    pwmBacklight.setBrightness(brightness);
                }
            }
            pwmBacklight.setBrightness(defaultBrightness); // restore default Brightness
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in PWM Backlight thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}

