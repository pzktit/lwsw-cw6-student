#include "app.hpp"

void rotary_button_thread( Application_state_t  & appState, const GPIO_config& SW_config) {
    try {
        gpiod::chip chip(SW_config.chipName);
        gpiod::line SW_line = chip.get_line(SW_config.lineNum);
        SW_line.request(SW_config.lineRequest);

        std::cout << "Monitoring Rotary button" << std::endl;

        std::cout << __func__ << " started." << std::endl;
        static std::chrono::steady_clock::time_point press_time = std::chrono::steady_clock::time_point::min();
        // static bool button_pressed = false;
        while ( appState.keepRunning.load() ) {
            auto event = SW_line.event_wait(std::chrono::milliseconds(100));
            if (event) {
                auto SW_event = SW_line.event_read();
                if (SW_event.event_type == gpiod::line_event::RISING_EDGE) { // button pressed, see the hardware configuration
                    press_time = std::chrono::steady_clock::now();
                    // button_pressed = true;
                } else if (SW_event.event_type == gpiod::line_event::FALLING_EDGE) { // button released
                    auto press_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - press_time).count();
                    // button_pressed = false;
                    if (press_duration > 500) { // Press longer than 500ms
                        appState.rotaryButtonLongPress.store(true);
                    } else {
                        appState.rotaryButtonShortPress.store(true);
                    }
                }
                else { // ignored event type
                    
                }
            }
        }
        SW_line.release();
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in rotary button monitoring thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
