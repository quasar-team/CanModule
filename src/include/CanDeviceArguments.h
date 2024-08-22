#ifndef SRC_INCLUDE_CANDEVICEARGUMENTS_H_
#define SRC_INCLUDE_CANDEVICEARGUMENTS_H_

#include <functional>
#include <string>

#include "CanDeviceConfiguration.h"
#include "CanFrame.h"

/**
 * @brief Configuration structure for a CanDevice.
 *
 * This structure holds the configuration details for a CAN device, including
 * the vendor-specific configuration and a callback function to handle received
 * CAN frames.
 */
struct CanDeviceArguments {
  /**
   * @brief Vendor-specific configuration string.
   *
   * This string contains configuration details specific to the vendor of the
   * CAN device.
   */
  CanDeviceConfiguration config;

  /**
   * @brief Callback function to handle received CanFrame.
   *
   * This function is called whenever a CAN frame is received. It takes a
   * reference to a CanFrame object as its parameter.
   *
   * @param receiver A function, lambda or functor that takes a const reference
   * to a CanFrame object and returns void.
   */
  const std::function<void(const CanFrame&)> receiver;
};

#endif  // SRC_INCLUDE_CANDEVICEARGUMENTS_H_
