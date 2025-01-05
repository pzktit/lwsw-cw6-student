#include "app.hpp"
#include <thread>

void servo_thread(Application_state_t  & appState, const PWM_Servo::Config & pwmServo_config) {
    try {
        const auto loop_time_step = std::chrono::milliseconds(50); 
        const auto waving_interval = std::chrono::milliseconds(500);
        PWM_Servo pwmSrv(pwmServo_config);
        std::cout << __func__ << " started." << std::endl;
        while (appState.keepRunning.load()) {
            if (appState.setAlarm.load()) { // if alarm then enter servo arm waving procedure
                auto now = std::chrono::steady_clock::now();
                auto last_waving(now); 
                int servo_direction = 1; // 1 = CW, -1 = CCW
                pwmSrv.setAngle(servo_direction * 45); // first move
                while (appState.keepRunning.load() && appState.setAlarm.load()) { 
                    std::this_thread::sleep_for(std::chrono::milliseconds(loop_time_step));
                    now = std::chrono::steady_clock::now();
                    if (now - last_waving > waving_interval) {
                        last_waving = now;
                        servo_direction = -servo_direction;
                        pwmSrv.setAngle(servo_direction * 45);
                    }
                }
                pwmSrv.setAngle(0); // alarm finished, reset servo
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in Servo thread: " << e.what() << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
