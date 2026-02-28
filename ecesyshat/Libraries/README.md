# SYS_HAT

This repo contains a collection of two libraries which drive:

1. The SSD1306
   * The SSD1306 is a OLED/PLED Segment/Common Driver with Controller. Which in the case of the SYS-HAT controls a 128x32 OLED display.
2. The BMI160
   * The BMI160 is a chip including an accelerometer and a gyroscope.

Both drivers take an instance of the interface IComInterface. For which one can implement the correct communication interface by inheritance.

## Getting started

### Dependencies

To build and install the included libraries one needs to install some dependencies.

Run the following to install needed dependencies:

```bash
sudo apt update 
sudo apt install build-essential cmake git
```

### Enabling i2c and spi 

As the oled screen communicates with the rpi using i2c, and the gyroscope communicates with the rpi using spi, one needs to enable those interfaces.

```bash
sudo raspi-config nonint do_i2c 0
sudo raspi-config nonint do_spi 0
```

### Building and Installing

You should now be able to run the following, to build and install the project.

```bash
cmake -S . -B build
cmake --build build
sudo cmake --install build --prefix /usr/
```

