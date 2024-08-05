#ifndef SRC_INCLUDE_CANDEVICECONFIG_H_
#define SRC_INCLUDE_CANDEVICECONFIG_H_

#include <functional>
#include <string>

#include "CanFrame.h"

struct CanDeviceConfig {
  const std::string vendor_config;
  const std::function<void(const CanFrame&)> receiver;
};

#endif /* SRC_INCLUDE_CANDEVICECONFIG_H_ */
