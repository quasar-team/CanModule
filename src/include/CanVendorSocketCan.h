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
  /**
   * @brief Constructor for the CanVendorSocketCan class.
   *
   * Initializes the CanVendorSocketCan object with the provided configuration.
   *
   * @param configuration A constant reference to the CanDeviceArguments object,
   * which contains configuration parameters for the CAN device.
   */
  explicit CanVendorSocketCan(const CanDeviceArguments &configuration)
      : CanDevice("socketcan", configuration) {}

  ~CanVendorSocketCan() { vendor_close(); }

 private:
  CanReturnCode vendor_open() override;
  CanReturnCode vendor_close() override;
  CanReturnCode vendor_send(const CanFrame &frame) override;
  CanDiagnostics vendor_diagnostics() override;

  int subscriber();
  static const CanFrame translate(const struct can_frame &message);
  static struct can_frame translate(const CanFrame &frame);

  int socket_fd{0};               // File descriptor for the SocketCAN device
  int epoll_fd{0};                // File descriptor for the epoll instance
  std::thread subscriber_thread;  // Thread for the subscriber loop
  bool m_subcriber_run{
      false};  // Flag to indicate that the subscriber loop should run
};

#endif  // SRC_INCLUDE_CANVENDORSOCKETCAN_H_
