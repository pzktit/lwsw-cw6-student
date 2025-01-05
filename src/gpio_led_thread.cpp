#include "app.hpp"

#include <thread> // because of sleep_for call

void gpio_led_thread(Application_state_t  & appState, const GPIO_Led::Config & led_config) {
    try {
        GPIO_Led led(led_config);
        led.setTrigger("default-on");
        led.set(GPIO_Led::ON);
        static const auto loop_time_step = std::chrono::milliseconds(100); 
        std::cout << __func__ << " started." << std::endl;
        while (appState.keepRunning.load()) {
            auto now=std::chrono::steady_clock::now();
            if (now < appState.alarmTime.load()) {
                // set the alarm state
                led.setTrigger("heartbeat");
                // wait for the alarm to expire
                while (appState.keepRunning.load() && now < appState.alarmTime.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    now = std::chrono::steady_clock::now();
                }
                led.setTrigger("default-on");
            }
            std::this_thread::sleep_for(loop_time_step);
        }
        led.setTrigger("none");
        led.set(GPIO_Led::OFF) ;
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in LED thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
