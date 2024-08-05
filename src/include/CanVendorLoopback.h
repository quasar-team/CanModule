#ifndef SRC_INCLUDE_CANVENDORLOOPBACK_H_
#define SRC_INCLUDE_CANVENDORLOOPBACK_H_

#include <string>

#include "CanDevice.h"
#include "CanDeviceConfig.h"

struct CanVendorLoopback : CanDevice {
  explicit CanVendorLoopback(const CanDeviceConfig &configuration)
      : CanDevice("dummy", configuration) {}

 protected:
  /* A hardware-specific will implement vendor_open, vendor_close, and
   * vendor_send. It will use received(const CanFrame &frame) to communicate
   * incoming CAN frames.
   */
  int vendor_open() override { return 0; }
  int vendor_close() override { return 0; }
  int vendor_send(const CanFrame &frame) override;
};

#endif /* SRC_INCLUDE_CANVENDORLOOPBACK_H_ */
