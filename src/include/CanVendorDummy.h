#ifndef SRC_INCLUDE_CANVENDORDUMMY_H_
#define SRC_INCLUDE_CANVENDORDUMMY_H_

#include <string>

#include "CanDevice.h"

struct CanVendorDummy : CanDevice {
  CanVendorDummy(std::string_view configuration = "")
      : CanDevice(configuration) {}

 protected:
  int vendor_open() override { return 0; }
  int vendor_close() override { return 0; }
  int vendor_send(const CanFrame &frame) override { return 0; }

  const std::string m_vendor_name{"dummy"};
};

#endif /* SRC_INCLUDE_CANVENDORDUMMY_H_ */
