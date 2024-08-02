#ifndef SRC_INCLUDE_CANVENDORSOCKETCAN_H_
#define SRC_INCLUDE_CANVENDORSOCKETCAN_H_

#include "CanVendorDummy.h"

struct CanVendorSocketCan : CanVendorDummy {
  CanVendorSocketCan(std::string_view configuration)
      : CanVendorDummy(configuration) {}
};

#endif /* SRC_INCLUDE_CANVENDORSOCKETCAN_H_ */
