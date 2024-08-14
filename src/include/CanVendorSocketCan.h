#ifndef SRC_INCLUDE_CANVENDORSOCKETCAN_H_
#define SRC_INCLUDE_CANVENDORSOCKETCAN_H_

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <thread>  // NOLINT

#include "CanVendorLoopback.h"

/**
 * @struct CanVendorSocketCan
 * @brief Represents a SocketCAN vendor implementation of a CAN device.
 *
 * This class provides the implementation for interacting with a SocketCAN
 * device. It extends the CanVendorLoopback class and overrides the necessary
 * methods to open, close, and send CAN frames using the SocketCAN interface.
 */
struct CanVendorSocketCan : CanVendorLoopback {
  /**
   * @brief Constructor for CanVendorSocketCan.
   *
   * @param configuration The configuration details for the CAN device.
   */
  explicit CanVendorSocketCan(const CanDeviceConfig &configuration)
      : CanVendorLoopback(configuration) {}

 private:
  /**
   * @brief Opens the SocketCAN device and initializes the epoll instance.
   *
   * This method opens the SocketCAN device, initializes the epoll instance,
   * and starts the subscriber loop in a new thread.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int vendor_open() override;

  /**
   * @brief Stops the subscriber loop and closes the SocketCAN device.
   *
   * This method stops the subscriber loop and closes the SocketCAN device.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int vendor_close() override;

  /**
   * @brief Sends a CAN frame to the SocketCAN device.
   *
   * This method sends a CAN frame to the SocketCAN device.
   *
   * @param frame The CAN frame to be sent.
   * @return 0 on success, or a negative error code on failure.
   */
  int vendor_send(const CanFrame &frame) override;

  /**
   * @brief Monitors the SocketCAN device with epoll.
   *
   * This method monitors the SocketCAN device using epoll. In the event of a
   * new message, it calls received(translate(socketCan message)).
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int subscriber();

  /**
   * @brief Translates a SocketCAN frame to a CanFrame.
   *
   * This method translates a SocketCAN frame to a CanFrame.
   *
   * @param message The SocketCAN frame to be translated.
   * @return The translated CanFrame.
   */
  static CanFrame translate(const struct can_frame &message);

  /**
   * @brief Translates a CanFrame to a SocketCAN frame.
   *
   * This method translates a CanFrame to a SocketCAN frame.
   *
   * @param frame The CanFrame to be translated.
   * @return The translated SocketCAN frame.
   */
  static struct can_frame translate(const CanFrame &frame);

  int socket_fd;                  // File descriptor for the SocketCAN device
  int epoll_fd;                   // File descriptor for the epoll instance
  std::thread subscriber_thread;  // Thread for the subscriber loop
};

#endif  // SRC_INCLUDE_CANVENDORSOCKETCAN_H_
