#include "app.hpp"

void rotary_encoder_thread(Application_state_t  & appState, const GPIO_config& SIA_config, const GPIO_config& SIB_config) {
    try {
        const int tempThresholdDelta = 1 ;
        const int tempThresholdMin = 0 ;
        const int tempThresholdMax = 60 ;
        // full quadrature decoding
        auto rotary_decoder = [](int clk, int dt) -> int {
                if (clk == 1 && dt == 0) {
                    return 0; // CW on rising edge of CLK with DT=0
                } else if (clk == -1 && dt == 1) {
                    return -1; // CW on falling edge of CLK with DT=1
                } else if (clk == 1 && dt == 1) {
                    return 1; // CCW on rising edge of CLK with DT=1
                } else if (clk == -1 && dt == 0) {
                    return 0; // CCW on falling edge of CLK with DT=0
                }
                return 0; // No rotation
            };
        // get lines
        gpiod::chip SIA_chip(SIA_config.chipName);
        gpiod::line SIA_line = SIA_chip.get_line(SIA_config.lineNum);
        SIA_line.request(SIA_config.lineRequest);

        gpiod::chip chip(SIB_config.chipName);
        gpiod::line SIB_line = chip.get_line(SIB_config.lineNum);
        SIB_line.request(SIB_config.lineRequest);

        std::cout << __func__ << " started." << std::endl;
        while ( appState.keepRunning.load() ) {
            // Wait for an edge event
            auto event = SIA_line.event_wait(std::chrono::milliseconds(100));
            if (event) {
                auto SIA_event = SIA_line.event_read();
                int SIA_Value = SIA_line.get_value();
                int SIB_Value = SIB_line.get_value();
                // if true rising edge (i.e. edge and correct value) or falling edge
                if ( (SIA_event.event_type == gpiod::line_event::RISING_EDGE && SIA_Value == 1) || (SIA_event.event_type == gpiod::line_event::FALLING_EDGE && SIA_Value == 0) ) { 
                    int rotation = rotary_decoder(
                        SIA_event.event_type == gpiod::line_event::RISING_EDGE ? 1 : -1, SIB_Value);
                    int newThreshold = appState.tempThreshold.load() ;
                    newThreshold += rotation * tempThresholdDelta ; // increase or decrease the threshold
                    newThreshold = std::max(tempThresholdMin, std::min(tempThresholdMax, newThreshold)) ; // constraint threshold to predefined limits
                    appState.tempThreshold.store(newThreshold) ;
                }
            }
        }
        // this is not required because of RAII
        // SIA_line.release();
        // SIB_line.release();
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in GPIO monitoring thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
