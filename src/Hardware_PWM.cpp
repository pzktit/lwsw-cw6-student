#include "Hardware_PWM.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <system_error>
#include <iostream>

Hardware_PWM::Hardware_PWM(int pwm_chip, int pwm_channel, unsigned long int period, unsigned long int duty_cycle)
    : period_(period)
    , duty_cycle_(duty_cycle)
    , pwm_chip_(pwm_chip)
    , pwm_channel_(pwm_channel) 
    {
        exportPWM();        
        if (period_ < duty_cycle_) {
            throw std::runtime_error("Duty cycle cannot be larger than the period");
        }
        if (isEnabled()) {
            disable();
        }
        setPeriod(period_);
        setDuty(duty_cycle_);
        enable();
}

Hardware_PWM::~Hardware_PWM() {
    disable();
    unexportPWM();
}

void Hardware_PWM::exportPWM() {
    writeToFile(getPWMChipPath() + "/export", pwm_channel_);
    usleep(100000); // Sleep for 100 milliseconds to give the PWM driver a chance to instantiate the sysfs interface
}

void Hardware_PWM::unexportPWM() {
    writeToFile(getPWMChipPath() + "/unexport", pwm_channel_);
}

void Hardware_PWM::setDuty(unsigned long int duty_cycle) {
    if (duty_cycle > period_) {
        throw std::runtime_error("Duty cycle cannot be larger than the period");
    }
    duty_cycle_ = duty_cycle;
    writeToFile(getPWMPath("duty_cycle"), duty_cycle);
}

void Hardware_PWM::setPeriod(unsigned long int period) {
    // Period can't be updated for a working generator
    std::string enable_path = getPWMPath("enable");
    if ( isEnabled() ) { 
        throw std::runtime_error("Cannot set period while PWM is enabled: " + enable_path);
    }
    period_ = period;
    writeToFile(getPWMPath("period"), period);
    setDuty(period / 2); // Set duty cycle to 50% of the new period, as the duty cycle cannto be greater than the period
}

void Hardware_PWM::enable() {
    writeToFile(getPWMPath("enable"), 1);
}

bool Hardware_PWM::isEnabled() {
    std::string enable_status = readFromFile(getPWMPath("enable"));
    return enable_status == "1\n";
}

void Hardware_PWM::disable() {
    writeToFile(getPWMPath("enable"), 0);
}

std::string Hardware_PWM::getPWMChipPath() const {
    return "/sys/class/pwm/pwmchip" + std::to_string(pwm_chip_);
}

std::string Hardware_PWM::getPWMPath(const std::string& file) const {
    return getPWMChipPath() + "/pwm" + std::to_string(pwm_channel_) + "/" + file;
}

std::string Hardware_PWM::readFromFile(const std::string& path) const {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "Unable to open file: " + path);
    }

    char buffer[256];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        close(fd);
        throw std::system_error(errno, std::generic_category(), "Unable to read from file: " + path);
    }

    buffer[bytes_read] = '\0';
    close(fd);
    return std::string(buffer);
}

void Hardware_PWM::writeToFile(const std::string& path, int value) {
    int fd = open(path.c_str(), O_WRONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "Unable to open file: " + path);
    }

    std::string value_str = std::to_string(value);
    // std::cout << "Writing to file: " << path << " value: " << value_str << " of size " << value_str.size() << std::endl;
    if (write(fd, value_str.c_str(), value_str.size()) == -1) {
        close(fd);
        throw std::system_error(errno, std::generic_category(), "Unable to write to file: " + path);
    }

    close(fd);
}