#include "GyroReader.hpp"
#include "spi_interface.hpp"
#include <bmi160_wrapper.hpp>

#include <cmath>
#include <unistd.h>

GyroReader::GyroReader(const char* spiDevice) : spiDevice_(spiDevice) {}

GyroReader::~GyroReader() { stop(); }

void GyroReader::start() {
    running_ = true;
    thread_  = std::thread(&GyroReader::loop, this);
}

void GyroReader::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

GyroReader::Angles GyroReader::read() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return angles_;
}

void GyroReader::loop() {
    SpiInterface   spi(spiDevice_);
    bmi160_wrapper sensor(spi);

    while (running_) {
        sensor.acquire_bmi160_data();
        const auto& acc  = sensor.getAccData();
        const auto& gyro = sensor.getGyroData();

        updateFilter(acc.x, acc.y, acc.z, gyro.x, gyro.y);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            angles_ = { static_cast<float>(pitch_),
                        static_cast<float>(roll_) };
        }

        usleep(SLEEP_US);
    }
}

void GyroReader::updateFilter(int16_t ax, int16_t ay, int16_t az,
                               int16_t gx, int16_t gy) {
    // Accelerometer-derived angles
    double ap = std::atan2(ax, std::sqrt((double)ay*ay + (double)az*az))
                * 180.0 / M_PI;
    double ar = std::atan2(ay, std::sqrt((double)ax*ax + (double)az*az))
                * 180.0 / M_PI;

    // Complementary filter — same as gyro_stream.cpp
    pitch_ = ALPHA * (pitch_ + gy * GYRO_SCALE * DT) + (1.0 - ALPHA) * ap;
    roll_  = ALPHA * (roll_  + gx * GYRO_SCALE * DT) + (1.0 - ALPHA) * ar;
}
