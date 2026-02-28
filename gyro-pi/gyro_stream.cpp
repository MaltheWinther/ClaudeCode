/*
 * gyro_stream.cpp
 *
 * Reads the BMI160 gyroscope + accelerometer via SPI, applies a complementary
 * filter, and streams pitch/roll angles as JSON lines to stdout.
 *
 * Designed to be piped into ws_bridge.py:
 *   ./gyro_stream | python3 ws_bridge.py
 *
 * Output format (one line per update at ~20 Hz):
 *   {"pitch":23.4,"roll":-5.2}
 */

#include "spi_interface.hpp"
#include <bmi160_wrapper.hpp>
#include <cmath>
#include <cstdio>
#include <csignal>
#include <unistd.h>

static const char   *SPI_DEVICE       = "/dev/spidev0.0";
static const double  ALPHA            = 0.98;
static const double  GYRO_SCALE       = 2000.0 / 32767.0;  // LSB → deg/s
static const int     UPDATE_US        = 25000;              // 25 ms → 40 Hz
static const double  DT               = UPDATE_US / 1e6;

static volatile bool running = true;
static void on_sigint(int) { running = false; }

static double pitch = 0.0;
static double roll  = 0.0;

static void update_filter(const bmi160_sensor_data &acc,
                          const bmi160_sensor_data &gyro)
{
    double ap = atan2((double)acc.x,
                      sqrt((double)acc.y * acc.y + (double)acc.z * acc.z))
                * 180.0 / M_PI;
    double ar = atan2((double)acc.y,
                      sqrt((double)acc.x * acc.x + (double)acc.z * acc.z))
                * 180.0 / M_PI;

    pitch = ALPHA * (pitch + (double)gyro.y * GYRO_SCALE * DT) + (1.0 - ALPHA) * ap;
    roll  = ALPHA * (roll  + (double)gyro.x * GYRO_SCALE * DT) + (1.0 - ALPHA) * ar;
}

int main(void)
{
    signal(SIGINT, on_sigint);

    SpiInterface    spi(SPI_DEVICE);
    bmi160_wrapper  sensor(spi);

    while (running) {
        sensor.acquire_bmi160_data();
        update_filter(sensor.getAccData(), sensor.getGyroData());

        printf("{\"pitch\":%.2f,\"roll\":%.2f}\n", pitch, roll);
        fflush(stdout);

        usleep(UPDATE_US);
    }

    return 0;
}
