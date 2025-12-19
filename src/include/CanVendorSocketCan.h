#ifndef SRC_INCLUDE_CANVENDORSOCKETCAN_H_
#define SRC_INCLUDE_CANVENDORSOCKETCAN_H_

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <thread>  // NOLINT

#include "CanDevice.h"
#include "CanDiagnostics.h"
#include "CanFrame.h"

/**
 * @struct CanVendorSocketCan
 * @brief Represents a SocketCAN vendor implementation of a CanDevice.
 *
 * This class provides the implementation for interacting with a SocketCAN
 * device. It extends the CanVendorLoopback class and overrides the necessary
 * methods to open, close, and send CAN frames using the SocketCAN interface.
 */
struct CanVendorSocketCan : CanDevice {
  explicit CanVendorSocketCan(const CanDeviceArguments& args);

  ~CanVendorSocketCan() { vendor_close(); }

 private:
  CanReturnCode vendor_open() noexcept override;
  CanReturnCode vendor_close() noexcept override;
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  CanDiagnostics vendor_diagnostics() noexcept override;

  int subscriber() noexcept;
  static const CanFrame translate(const struct can_frame& message) noexcept;
  static struct can_frame translate(const CanFrame& frame) noexcept;

  int m_socket_fd{-1};              // File descriptor for the SocketCAN device
  int m_epoll_fd{-1};               // File descriptor for the epoll instance
  std::thread m_subscriber_thread;  // Thread for the subscriber loop
  bool m_subcriber_run{
      false};  // Flag to indicate that the subscriber loop should run
};

#endif  // SRC_INCLUDE_CANVENDORSOCKETCAN_H_
