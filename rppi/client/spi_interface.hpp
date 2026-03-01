#pragma once

#include <com_interface.hpp>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

/**
 * SpiInterface — concrete implementation of SYSHAT::ICommInterface for Linux
 * spidev. Used to communicate with the BMI160 over SPI on the ECE SYS HAT.
 *
 * BMI160 SPI protocol:
 *   Read:  tx = [reg | 0x80, 0x00 * len], rx[1..len] = data
 *   Write: tx = [reg & 0x7F, data[0..len-1]]
 */
class SpiInterface : public SYSHAT::ICommInterface {
public:
    SpiInterface(const char *device, uint32_t speed_hz = 10000000) {
        fd_ = open(device, O_RDWR);
        if (fd_ < 0) {
            perror("SpiInterface: open");
            return;
        }

        uint8_t mode  = SPI_MODE_3;
        uint8_t bits  = 8;

        if (ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0)        perror("SPI_IOC_WR_MODE");
        if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) perror("SPI_IOC_WR_BITS_PER_WORD");
        if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) perror("SPI_IOC_WR_MAX_SPEED_HZ");
    }

    ~SpiInterface() {
        if (fd_ >= 0) close(fd_);
    }

    /**
     * Write `length` bytes from `buf` to register `slaveAddress`.
     * tx = [reg, buf[0], buf[1], ..., buf[length-1]]
     */
    int8_t write(const uint8_t slaveAddress, const uint8_t *buf,
                 const uint16_t length) override {
        uint8_t tx[length + 1];
        tx[0] = slaveAddress & 0x7F;   // bit7 = 0 → write
        for (uint16_t i = 0; i < length; i++) tx[i + 1] = buf[i];

        struct spi_ioc_transfer tr = {};
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = 0;
        tr.len    = length + 1;

        return (ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) < 0) ? -1 : 0;
    }

    /**
     * Read `length` bytes into `buf` from register `slaveAddress`.
     * tx = [reg | 0x80, 0x00 * length], rx[1..length] = data
     */
    int8_t read(const uint8_t slaveAddress, uint8_t *buf,
                const uint16_t length) override {
        uint8_t tx[length + 1];
        uint8_t rx[length + 1];
        tx[0] = slaveAddress | 0x80;   // bit7 = 1 → read
        for (uint16_t i = 1; i <= length; i++) tx[i] = 0x00;

        struct spi_ioc_transfer tr = {};
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len    = length + 1;

        if (ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) < 0) return -1;

        for (uint16_t i = 0; i < length; i++) buf[i] = rx[i + 1];
        return 0;
    }

private:
    int fd_ = -1;
};
