#ifndef BMI160_WRAPPER_H_
#define BMI160_WRAPPER_H_

#include "bmi160.h"
#include "bmi160_defs.h"
#include "com_interface.hpp"
#include "unistd.h"
#include <stdio.h>

/* local macro definitions */
/*! I2C interface communication, 1 - Enable; 0- Disable */
#define BMI160_INTERFACE_I2C 0

/*! SPI interface communication, 1 - Enable; 0- Disable */
#define BMI160_INTERFACE_SPI 1

/**
 * @brief This class serves the purpose of wrapping the sensor functionality
 * into a easy to use interface. Notice that measurements must be updated before
 * the internal attributes update. Once updated, the newest measurements can be
 * retrieved by using the get methods for the two types of data.
 */
class bmi160_wrapper {
public:
  bmi160_wrapper(SYSHAT::ICommInterface &bus_ref);
  bmi160_wrapper(bmi160_wrapper &&) = default;
  bmi160_wrapper(const bmi160_wrapper &) = default;
  bmi160_wrapper &operator=(bmi160_wrapper &&) = default;
  bmi160_wrapper &operator=(const bmi160_wrapper &) = default;
  ~bmi160_wrapper();

  // Wrapper get methods for gyro and acc
  bmi160_sensor_data getAccData();
  bmi160_sensor_data getGyroData();
  int8_t update_bmi160_data();
  int8_t acquire_bmi160_data();

private:
  // Internal init functions
  void init_bmi160(void);
  void init_bmi160_sensor_driver_interface(void);

  // attributes
  SYSHAT::ICommInterface &bus_ref_;
  bmi160_sensor_data acc_data;
  bmi160_sensor_data gyro_data;
  struct bmi160_dev bmi160dev; // Contains relevant bmi160 inf
  const int BMI160_SHUTTLE_ID = 0x38;
  const int BMI160_DEV_ADDR = BMI160_I2C_ADDR;
};

#endif /* BMI160_WRAPPER_H_ */