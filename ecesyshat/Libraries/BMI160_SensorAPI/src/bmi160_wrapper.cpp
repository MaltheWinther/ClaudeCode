#include "bmi160_wrapper.hpp"
#include <com_interface.hpp>

bmi160_wrapper::bmi160_wrapper(SYSHAT::ICommInterface &bus_ref)
    : bus_ref_(bus_ref) {
  init_bmi160_sensor_driver_interface();
  init_bmi160();
}

bmi160_wrapper::~bmi160_wrapper() {}

bmi160_sensor_data bmi160_wrapper::getAccData() { return acc_data; }

bmi160_sensor_data bmi160_wrapper::getGyroData() { return gyro_data; }

int8_t bmi160_wrapper::update_bmi160_data() {
  // sensor_select 3 should be both sensors as defined in bmi160_select_sensor
  // enum in bmi160_defs.h
  int8_t result = 0;
  result = bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL),
                                  &acc_data, &gyro_data, &bmi160dev);

  // Printing out acc and gyro data for testing.
  printf("ax:%d\tay:%d\taz:%d\n", acc_data.x, acc_data.y, acc_data.z);
  printf("gx:%d\tgy:%d\tgz:%d\n", gyro_data.x, gyro_data.y, gyro_data.z);
  fflush(stdout);

  bmi160dev.delay_ms(10);

  return result;
}

int8_t bmi160_wrapper::acquire_bmi160_data() {
  // sensor_select 3 should be both sensors as defined in bmi160_select_sensor
  // enum in bmi160_defs.h
  return bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL),
                                  &acc_data, &gyro_data, &bmi160dev);

}


//*********************************************************************/
/* functions */
/*********************************************************************/

/* Glue for i2c_wrapper*/
static SYSHAT::ICommInterface *g_spi = nullptr;

static int8_t bmi_write_cb(uint8_t dev, uint8_t reg, const uint8_t *buf,
                           uint16_t len) {
  return g_spi->write(reg, buf, len);
}

static int8_t bmi_read_cb(uint8_t dev, uint8_t reg, uint8_t *buf,
                          uint16_t len) {
  return g_spi->read(reg, buf, len);
}

static void bmi_delay_cb(uint32_t ms) { usleep(1000 * ms); }

void bmi160_wrapper::init_bmi160(void) {
  int8_t rslt;

  rslt = bmi160_init(&bmi160dev);

  if (rslt == BMI160_OK) {
    printf("BMI160 initialization success !\n");
    printf("Chip ID 0x%X\n", bmi160dev.chip_id);
  } else {
    printf("BMI160 initialization failure !\n");
    printf("BMI160 initialization failure :((( !\n");
    exit(1);
  }

  /* Select the Output data rate, range of accelerometer sensor */
  bmi160dev.accel_cfg.odr = BMI160_ACCEL_ODR_1600HZ;
  bmi160dev.accel_cfg.range = BMI160_ACCEL_RANGE_16G;
  bmi160dev.accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;

  /* Select the power mode of accelerometer sensor */
  bmi160dev.accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;

  /* Select the Output data rate, range of Gyroscope sensor */
  bmi160dev.gyro_cfg.odr = BMI160_GYRO_ODR_3200HZ;
  bmi160dev.gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
  bmi160dev.gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;

  /* Select the power mode of Gyroscope sensor */
  bmi160dev.gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;

  /* Set the sensor configuration */
  rslt = bmi160_set_sens_conf(&bmi160dev);
}

void bmi160_wrapper::init_bmi160_sensor_driver_interface(void) {
  g_spi = &bus_ref_;

  /* link read/write/delay function of host system to appropriate
   * bmi160 function call prototypes */
  bmi160dev.write = bmi_write_cb;
  bmi160dev.read = bmi_read_cb;
  bmi160dev.delay_ms = bmi_delay_cb;

  /* set correct SPI config */
  // Set to 0 since it COINES_SHUTTLE_PIN_7 seems to have been set to low
  bmi160dev.id = 0;
  bmi160dev.intf = BMI160_SPI_INTF;
}
