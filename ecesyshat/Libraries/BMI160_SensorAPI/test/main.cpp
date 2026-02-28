#include "bmi160_wrapper.hpp"
// #include "i2c_wrapper.hpp"

int main(void) {
  I2C_Wrapper i2c_driv;
  i2c_driv.begin();
  bmi160_wrapper bmi160_driv(i2c_driv);

  while (1) {
    bmi160_driv.update_bmi160_data();
  }
  // Setting up

  return 0;
}
