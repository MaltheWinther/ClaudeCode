// /***Copyright(C) 2021 Bosch Sensortec GmbH.All rights reserved.**SPDX -
// License
// - Identifier : BSD
//       -
//       3 -
//       Clause * /
//
// *********************************************************************/
// /* system header files */
// /*********************************************************************/
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
//
// /*********************************************************************/
// /* own header files */
// /*********************************************************************/
// #include "bmi160.h"
// #include "i2c_wrapper.hpp"
// /*********************************************************************/
// /* local macro definitions */
// /*! I2C interface communication, 1 - Enable; 0- Disable */
// #define BMI160_INTERFACE_I2C 1
//
// /*! SPI interface communication, 1 - Enable; 0- Disable */
// #define BMI160_INTERFACE_SPI 0
//
// #if (!((BMI160_INTERFACE_I2C == 1) && (BMI160_INTERFACE_SPI == 0)) && \
//      (!((BMI160_INTERFACE_I2C == 0) && (BMI160_INTERFACE_SPI == 1))))
// #error \
//     "Invalid value given for the macros BMI160_INTERFACE_I2C /
//     BMI160_INTERFACE_SPI"
// #endif
//
// /*! bmi160 shuttle id */
// #define BMI160_SHUTTLE_ID 0x38
//
// /*! bmi160 Device address */
// #define BMI160_DEV_ADDR BMI160_I2C_ADDR
//
// /*********************************************************************/
// /* global variables */
// /*********************************************************************/
//
// /*! @brief This structure containing relevant bmi160 info */
// struct bmi160_dev bmi160dev;
//
// /*! @brief variable to hold the bmi160 accel data */
// struct bmi160_sensor_data bmi160_accel;
//
// /*! @brief variable to hold the bmi160 gyro data */
// struct bmi160_sensor_data bmi160_gyro;
//
// /*********************************************************************/
// /* static function declarations */
// /*********************************************************************/
//
// /*!
//  * @brief   internal API is used to initialize the sensor interface
//  */
// static void init_sensor_interface(void);
//
// /*!
//  * @brief   This internal API is used to initialize the bmi160 sensor with
//  * default
//  */
// static void init_bmi160(void);
//
// /*!
//  * @brief   This internal API is used to initialize the sensor driver
//  interface
//  */
// static void init_bmi160_sensor_driver_interface(I2C_Wrapper &bus_ref);
