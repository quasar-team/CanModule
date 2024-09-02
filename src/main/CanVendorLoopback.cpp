#include "CanVendorLoopback.h"

/**
 * @brief Sends a CAN frame to the vendor loopback.
 *
 * This function simulates sending a CAN frame by immediately invoking the
 * received callback with the provided frame.
 *
 * @param frame The CAN frame to be sent.
 * @return Always returns 0 indicating success.
 */
CanReturnCode CanVendorLoopback::vendor_send(const CanFrame &frame) noexcept {
  received(frame);
  return CanReturnCode::success;
}
