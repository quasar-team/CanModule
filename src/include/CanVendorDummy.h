#ifndef SRC_INCLUDE_CANVENDORDUMMY_H_
#define SRC_INCLUDE_CANVENDORDUMMY_H_

#include <string>

#include "CanDevice.h"
#include "CanDeviceConfig.h"

struct CanVendorDummy : CanDevice {
  explicit CanVendorDummy(const CanDeviceConfig &configuration)
      : CanDevice("dummy", configuration) {}

 protected:
  int vendor_open() override { return 0; }
  int vendor_close() override { return 0; }
  int vendor_send(const CanFrame &frame) override;
};

#endif /* SRC_INCLUDE_CANVENDORDUMMY_H_ */
