#include "CanDevice.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "CanVendorAnagate.h"
#include "CanVendorDummy.h"
#include "CanVendorSocketCan.h"

/**
 * @brief Sends a vector of CAN frames.
 *
 * This function sends each CAN frame in the provided vector and returns a
 * vector of integers representing the result of each send operation.
 *
 * @param frames A vector of CanFrame objects to be sent.
 * @return A vector of integers where each integer represents the result of
 * sending the corresponding CanFrame.
 */
int CanDevice::send(const CanFrame &frame) {
  if (frame.is_valid()) {
    return vendor_send(frame);
  }
  return 1;
}

std::vector<int> CanDevice::send(const std::vector<CanFrame> &frames) {
  std::vector<int> result(frames.size());
  std::transform(frames.begin(), frames.end(), result.begin(),
                 [this](const CanFrame &frame) { return send(frame); });
  return result;
}

std::unique_ptr<CanDevice> CanDevice::create(
    std::string_view vendor, const CanDeviceConfig &configuration) {
  if (vendor == "dummy") {
    return std::make_unique<CanVendorDummy>(configuration);
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
