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
  explicit CanVendorLoopback(const CanDeviceArguments &configuration)
      : CanDevice("loopback", configuration) {}

 private:
  /* A hardware-specific will implement vendor_open, vendor_close, and
   * vendor_send. It will use received(const CanFrame &frame) to communicate
   * incoming CAN frames.
   */
  int vendor_open() override { return 0; }
  int vendor_close() override { return 0; }
  int vendor_send(const CanFrame &frame) override;
  CanDiagnostics vendor_diagnostics() override { return CanDiagnostics{}; }
};

#endif  // SRC_INCLUDE_CANVENDORLOOPBACK_H_
