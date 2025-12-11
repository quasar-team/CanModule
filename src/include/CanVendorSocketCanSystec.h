#ifndef SRC_INCLUDE_CANVENDORSOCKETCANSYSTEC_H_
#define SRC_INCLUDE_CANVENDORSOCKETCANSYSTEC_H_

#include <memory>

#include "CanDevice.h"
#include "CanVendorSocketCan.h"

/**
 * @struct CanVendorSocketCanSystec
 * @brief Represents a SocketCAN Systec specific implementation of a CanDevice.
 *
 * This class provides the implementation for interacting with a SocketCAN
 * Systec device. It extends the CanVendorSocketCan class and overrides the
 * necessary methods to open, close, and send CAN frames using the SocketCAN
 * interface. It provides a custom mechanishm for handling BUS_OFF errors,
 * restarting the interface instead of using the built-in restart mechanism of
 * SocketCan due to a kernel-panic bug on Systec linux module.
 */
struct CanVendorSocketCanSystec : CanDevice {
  explicit CanVendorSocketCanSystec(const CanDeviceArguments& args);
  ~CanVendorSocketCanSystec() { vendor_close(); }

 private:
  CanReturnCode vendor_open() noexcept override;
  CanReturnCode vendor_close() noexcept override;
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  CanDiagnostics vendor_diagnostics() noexcept override;

  std::unique_ptr<CanDevice> m_can_vendor_socketcan;
};

#endif  // SRC_INCLUDE_CANVENDORSOCKETCANSYSTEC_H_
