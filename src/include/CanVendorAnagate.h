#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include "CanDevice.h"
#include "CanVendorDummy.h"

struct CanVendorAnagate : CanVendorDummy {
  CanVendorAnagate(std::string_view configuration)
      : CanVendorDummy(configuration) {}
};

#endif /* SRC_INCLUDE_CANVENDORANAGATE_H_ */
