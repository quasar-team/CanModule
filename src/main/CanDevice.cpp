#include "CanDevice.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "CanVendorAnagate.h"
#include "CanVendorLoopback.h"
#include "CanVendorSocketCan.h"

/**
 * @brief Opens the CAN device for communication.
 *
 * This function initializes and opens the CAN device using the vendor-specific
 * implementation.
 *
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
int CanDevice::open() { return vendor_open(); }
/**
 * @brief Closes the CAN device.
 *
 * This function finalizes and closes the CAN device using the vendor-specific
 * implementation.
 *
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
int CanDevice::close() { return vendor_close(); }
/**
 * @brief Sends a CAN frame.
 *
 * This function sends a single CAN frame using the vendor-specific
 * implementation.
 *
 * @param frame The CAN frame to be sent. It must be a valid frame.
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
int CanDevice::send(const CanFrame &frame) {
  if (frame.is_valid()) {
    return vendor_send(frame);
  }
  return 1;
}

/**
 * @brief Sends multiple CAN frames.
 *
 * This function sends a list of CAN frames using the vendor-specific
 * implementation. It returns a vector of integers where each integer represents
 * the result of sending the corresponding CAN frame.
 *
 * @param frames A vector of CAN frames to be sent. Each frame must be a valid
 * frame.
 * @return std::vector<int> A vector of integers where each integer is 0 on
 * success or a non-zero error code on failure for the corresponding frame.
 */
std::vector<int> CanDevice::send(const std::vector<CanFrame> &frames) {
  std::vector<int> result(frames.size());
  std::transform(frames.begin(), frames.end(), result.begin(),
                 [this](const CanFrame &frame) { return send(frame); });
  return result;
}

/**
 * @brief Creates a CAN device instance based on the specified vendor.
 *
 * This function creates and returns a unique pointer to a CAN device object
 * that corresponds to the specified vendor. The created device is configured
 * using the provided configuration.
 *
 * @param vendor A string view representing the vendor name. Supported values
 * are "dummy", "socketcan" (on non-Windows platforms), and "anagate".
 * @param configuration A reference to a CanDeviceConfig object that contains
 * the configuration settings for the CAN device.
 * @return std::unique_ptr<CanDevice> A unique pointer to the created CAN device
 * object. Returns nullptr if the vendor is not recognized.
 */
std::unique_ptr<CanDevice> CanDevice::create(
    std::string_view vendor, const CanDeviceConfig &configuration) {
  if (vendor == "dummy") {
    return std::make_unique<CanVendorLoopback>(configuration);
  }
#ifndef _WIN32
  if (vendor == "socketcan") {
    return std::make_unique<CanVendorSocketCan>(configuration);
  }
#endif

  if (vendor == "anagate") {
    return std::make_unique<CanVendorAnagate>(configuration);
  }

  return nullptr;
}
