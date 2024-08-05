#ifndef SRC_INCLUDE_CANVENDORSOCKETCAN_H_
#define SRC_INCLUDE_CANVENDORSOCKETCAN_H_

#include "CanVendorDummy.h"

struct CanVendorSocketCan : CanVendorDummy {
  explicit CanVendorSocketCan(const CanDeviceConfig &configuration)
      : CanVendorDummy(configuration) {}
};

#endif /* SRC_INCLUDE_CANVENDORSOCKETCAN_H_ */
