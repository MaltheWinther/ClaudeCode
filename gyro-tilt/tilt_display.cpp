/*
 * tilt_display.cpp
 *
 * Determines tilt direction of the ECE SYS HAT using a complementary filter
 * that combines both the accelerometer and gyroscope from the BMI160 chip.
 *
 * Why both sensors?
 *   - Accelerometer alone: knows absolute tilt but is noisy during movement
 *   - Gyroscope alone:     smooth and fast but drifts over time
 *   - Complementary filter: trusts the gyro for fast changes (98%), and uses
 *     the accelerometer to correct long-term drift (2%)
 *
 *   angle = α × (angle + gyro_rate × dt) + (1 - α) × accel_angle
 *
 * Compile with: make
 * Run with:     ./tilt_display
 */

#include "spi_interface.hpp"
#include <bmi160_wrapper.hpp>
#include <cmath>
#include <cstdio>
#include <csignal>
#include <unistd.h>
#include <time.h>

// ── Configuration ─────────────────────────────────────────────────────────────

// SPI device — typically /dev/spidev0.0 on RPi (SPI0, CE0)
static const char *SPI_DEVICE = "/dev/spidev0.0";

// Complementary filter coefficient (0–1).
// Higher = trust gyro more (smoother, more drift); lower = trust accel more.
static const double ALPHA = 0.98;

// Tilt angle (degrees) required to report a direction
static const double TILT_THRESHOLD_DEG = 15.0;

// Update interval in microseconds (50ms → 20 Hz)
static const int UPDATE_INTERVAL_US = 50000;
static const double DT = UPDATE_INTERVAL_US / 1e6;  // seconds

// BMI160 sensor scaling factors (from bmi160_wrapper init):
//   Accel:  ±16G  → 1G   = 32767/16  ≈ 2048 LSB
//   Gyro:   ±2000 DPS → 1 DPS = 32767/2000 ≈ 16.384 LSB
static const double GYRO_SCALE = 2000.0 / 32767.0;   // LSB → degrees/sec

// ── Signal handling ───────────────────────────────────────────────────────────

static volatile bool running = true;
static void on_sigint(int) { running = false; }

// ── Complementary filter state ────────────────────────────────────────────────

static double pitch = 0.0;  // rotation around Y axis (forward/backward)
static double roll  = 0.0;  // rotation around X axis (left/right)

static void update_filter(const bmi160_sensor_data &acc,
                          const bmi160_sensor_data &gyro) {
    // Accelerometer angle (degrees) — gives absolute tilt from gravity
    // pitch: tilt forward/backward
    double accel_pitch = atan2((double)acc.x,
                               sqrt((double)acc.y * acc.y +
                                    (double)acc.z * acc.z)) * 180.0 / M_PI;
    // roll: tilt left/right
    double accel_roll  = atan2((double)acc.y,
                               sqrt((double)acc.x * acc.x +
                                    (double)acc.z * acc.z)) * 180.0 / M_PI;

    // Gyroscope rate (degrees/sec)
    double gyro_pitch_rate = (double)gyro.y * GYRO_SCALE;
    double gyro_roll_rate  = (double)gyro.x * GYRO_SCALE;

    // Complementary filter: blend gyro integration with accel correction
    pitch = ALPHA * (pitch + gyro_pitch_rate * DT) + (1.0 - ALPHA) * accel_pitch;
    roll  = ALPHA * (roll  + gyro_roll_rate  * DT) + (1.0 - ALPHA) * accel_roll;
}

// ── Display ───────────────────────────────────────────────────────────────────

static void print_tilt(void) {
    bool forward  = pitch >  TILT_THRESHOLD_DEG;
    bool backward = pitch < -TILT_THRESHOLD_DEG;
    bool left     = roll  >  TILT_THRESHOLD_DEG;
    bool right    = roll  < -TILT_THRESHOLD_DEG;
    bool flat     = !forward && !backward && !left && !right;

    printf("\r");

    if (flat) {
        printf("Direction: FLAT/LEVEL          ");
    } else {
        printf("Direction:");
        if (forward)  printf(" FORWARD");
        if (backward) printf(" BACKWARD");
        if (left)     printf(" LEFT");
        if (right)    printf(" RIGHT");
        printf("               ");
    }

    printf("  pitch:%+6.1f°  roll:%+6.1f°", pitch, roll);
    fflush(stdout);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    signal(SIGINT, on_sigint);

    printf("Initialising SPI on %s...\n", SPI_DEVICE);
    SpiInterface spi(SPI_DEVICE);

    printf("Initialising BMI160...\n");
    bmi160_wrapper sensor(spi);

    printf("Running complementary filter — press Ctrl+C to stop.\n\n");

    while (running) {
        sensor.acquire_bmi160_data();

        bmi160_sensor_data acc  = sensor.getAccData();
        bmi160_sensor_data gyro = sensor.getGyroData();

        update_filter(acc, gyro);
        print_tilt();

        usleep(UPDATE_INTERVAL_US);
    }

    printf("\nStopped.\n");
    return 0;
}
