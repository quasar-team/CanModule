#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include "CanDevice.h"
#include "CanVendorLoopback.h"

struct CanVendorAnagate : CanDevice {
  explicit CanVendorAnagate(const CanDeviceConfig &configuration)
      : CanDevice("anagate", configuration) {}

 private:
  AnaInt32 m_handler{nullptr};
};

#endif /* SRC_INCLUDE_CANVENDORANAGATE_H_ */
