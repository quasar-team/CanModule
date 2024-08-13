#ifndef SRC_INCLUDE_CANVENDORSOCKETCAN_H_
#define SRC_INCLUDE_CANVENDORSOCKETCAN_H_

#include "CanVendorLoopback.h"

struct CanVendorSocketCan : CanVendorLoopback {
  explicit CanVendorSocketCan(const CanDeviceConfig &configuration)
      : CanVendorLoopback(configuration) {}
};

#endif  // SRC_INCLUDE_CANVENDORSOCKETCAN_H_
