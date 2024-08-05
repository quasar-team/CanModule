#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include "CanDevice.h"
#include "CanVendorLoopback.h"

struct CanVendorAnagate : CanVendorLoopback {
  explicit CanVendorAnagate(const CanDeviceConfig &configuration)
      : CanVendorLoopback(configuration) {}
};

#endif /* SRC_INCLUDE_CANVENDORANAGATE_H_ */
