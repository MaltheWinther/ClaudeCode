#pragma once

#include <atomic>
#include <mutex>
#include <thread>

// Reads BMI160 via SPI, applies a complementary filter, and exposes
// the latest pitch/roll angles thread-safely.
// Call start() once — it spawns a background thread at ~40Hz.

class GyroReader {
public:
    struct Angles { float pitch = 0.f; float roll = 0.f; };

    explicit GyroReader(const char* spiDevice = "/dev/spidev0.0");
    ~GyroReader();

    void   start();
    void   stop();
    Angles read() const;   // thread-safe snapshot of latest angles

private:
    void loop();
    void updateFilter(int16_t ax, int16_t ay, int16_t az,
                      int16_t gx, int16_t gy);

    const char*  spiDevice_;
    std::thread  thread_;
    std::atomic<bool> running_{ false };

    mutable std::mutex mutex_;
    Angles angles_;

    // Filter state (same as gyro_stream.cpp)
    double pitch_ = 0.0;
    double roll_  = 0.0;

    static constexpr double ALPHA      = 0.98;
    static constexpr double GYRO_SCALE = 2000.0 / 32767.0;  // LSB → deg/s
    static constexpr double DT         = 0.025;              // 25 ms → 40 Hz
    static constexpr int    SLEEP_US   = 25000;
};
