#ifndef SRC_INCLUDE_CANVENDORLOOPBACK_H_
#define SRC_INCLUDE_CANVENDORLOOPBACK_H_

#include <string>

#include "CanDevice.h"
#include "CanDeviceArguments.h"
#include "CanDiagnostics.h"

/**
 * @brief A dummy implementation of CanDevice for a loopback device.
 */
struct CanVendorLoopback : CanDevice {
  /**
   * @brief Constructor for the CanVendorLoopback class.
   *
   * Initializes the CanVendorLoopback object with the provided configuration.
   *
   * @param configuration A constant reference to the CanDeviceArguments object,
   * which contains configuration parameters for the CAN device.
   */
  inline explicit CanVendorLoopback(
      const CanDeviceArguments& configuration) noexcept
      : CanDevice("loopback", configuration) {}

 private:
  /* A hardware-specific will implement vendor_open, vendor_close, and
   * vendor_send. It will use received(const CanFrame &frame) to communicate
   * incoming CAN frames.
   */
  inline CanReturnCode vendor_open() noexcept override {
    return CanReturnCode::success;
  }
  inline CanReturnCode vendor_close() noexcept override {
    return CanReturnCode::success;
  }
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  inline CanDiagnostics vendor_diagnostics() noexcept override {
    return CanDiagnostics{};
  }
};

#endif  // SRC_INCLUDE_CANVENDORLOOPBACK_H_
