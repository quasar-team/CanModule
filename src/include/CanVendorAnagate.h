#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include "CanDevice.h"
#include "CanVendorDummy.h"

struct CanVendorAnagate : CanVendorDummy {
  explicit CanVendorAnagate(const CanDeviceConfig &configuration)
      : CanVendorDummy(configuration) {}
};

#endif /* SRC_INCLUDE_CANVENDORANAGATE_H_ */
