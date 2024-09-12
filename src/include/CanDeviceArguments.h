#ifndef SRC_INCLUDE_CANDEVICEARGUMENTS_H_
#define SRC_INCLUDE_CANDEVICEARGUMENTS_H_

#include <functional>
#include <string>

#include "CanDeviceConfiguration.h"
#include "CanFrame.h"

enum class CanReturnCode;

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
  const CanDeviceConfiguration config;

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

/**
 * @brief Callback function to handle errors.
 *
 * This function is called whenever an error occurs during subscription.
 * It takes a CanReturnCode and a string_view message as its parameters.
 *
 * @param return_code The error code indicating the type of error.
 * @param message A string_view containing a description of the error.
 */
const std::function<void(const CanReturnCode, std::string_view)> on_error;
};

#endif  // SRC_INCLUDE_CANDEVICEARGUMENTS_H_
