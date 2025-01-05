
#include "app.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/input.h>
#include <unistd.h>


static void handle_gpio_key_event(const input_event &input_event, Application_state_t  &appState) {
    static std::chrono::steady_clock::time_point press_time = std::chrono::steady_clock::time_point::min();
    static bool button_pressed = false;

    if (input_event.value == 1 && !button_pressed) { // Press
        press_time = std::chrono::steady_clock::now();
        button_pressed = true;
    } else if (input_event.value == 0 && button_pressed) { // Release
        auto press_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - press_time).count();
        if (press_duration > 500) { // Press longer than 500ms
            button_pressed = false;
            std::cout << "Application exit" << std::endl;
            appState.keepRunning.store(false);            
        } else {
            appState.gpioButtonShortPress.store(true);
            button_pressed = false;
            // do nothing on short press
        } 
    }
}

void button_thread(Application_state_t  &appState, const std::string &inputDevice) {
    try {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd < 0) {
            throw std::runtime_error("Failed to create epoll instance");
        }

        int fd = open(inputDevice.c_str(), O_RDONLY);
        if (fd < 0) {
            throw std::runtime_error("Failed to open device: " + inputDevice);
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            close(fd);
            throw std::runtime_error("Failed to add file descriptor to epoll: " + inputDevice);
        }

        std::cout << __func__ << " started for device: " << inputDevice << std::endl;

        struct epoll_event event;
        struct input_event input_event;

        while (appState.keepRunning.load()) {
            // Wait for single event with 10ms timeout
            int ready = epoll_wait(epoll_fd, &event, 1, 10);
            
            if (ready < 0) {
                throw std::runtime_error("Epoll wait error occurred");
            }
            
            if (ready > 0 && (event.events & EPOLLIN)) {
                ssize_t bytes = read(event.data.fd, &input_event, sizeof(struct input_event));
                
                if (bytes == sizeof(struct input_event) && input_event.type == EV_KEY) {
                    handle_gpio_key_event(input_event, appState);
                }
            }
        }        
        // this is C API, always clean up after yourself
        close(fd);
        close(epoll_fd);
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in Button monitoring thread: " << e.what() << " in " << __func__ << std::endl;
    }
    std::cout << __func__ << " thread finished." << std::endl;
}
