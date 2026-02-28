#pragma once
#include <cstdint>
namespace SYSHAT {
class ICommInterface {
public:
  virtual ~ICommInterface() = default;
  virtual int8_t write(const uint8_t slaveAddress, const uint8_t *buf,
                       const uint16_t length) = 0;
  virtual int8_t read(const uint8_t slaveAddress, uint8_t *buf,
                      const uint16_t length) = 0;

private:
};

} // namespace SYSHAT
