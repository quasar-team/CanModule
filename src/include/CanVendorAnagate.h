#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include "CanDevice.h"
#include "CanVendorLoopback.h"

struct CanVendorAnagate : CanVendorLoopback {
  explicit CanVendorAnagate(const CanDeviceConfig &configuration)
      : CanVendorLoopback(configuration) {}

  // AnaInt32 m_handler{nullptr};
};

#endif  // SRC_INCLUDE_CANVENDORANAGATE_H_
