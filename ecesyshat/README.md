# EceSysHat

This repo contains material for the ECE SYS HAT with the following devices:

* Six push-buttons
* Three LEDs
* An OLED Display
* An BMI160 Accelerometer / Gyro

### Documents

Contains pdf documentation, schematics

### PCB

Contains KiCad files for schematic and PCB

### TestPrograms

Contains small test programs to check correct assembly

Remember to enable i2c and spi with 'raspi_config' and install Libraries before running them. 

### Install Libraries 

#### Dependencies

To build and install the included libraries one needs to install some dependencies.

Run the following to install needed dependencies:

```bash
sudo apt update 
sudo apt install build-essential cmake git
```

#### Enabling i2c and spi 

As the oled screen communicates with the rpi using i2c, and the gyroscope communicates with the rpi using spi, one needs to enable those interfaces.

```bash
sudo raspi-config nonint do_i2c 0
sudo raspi-config nonint do_spi 0
```

#### Building and Installing

Enter Libraries dir:

```bash
cd Libraries/
```

You should now be able to run the following, to build and install the project.

```bash
cmake -S . -B build
cmake --build build
sudo cmake --install build --prefix /usr/
```


