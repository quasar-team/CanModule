#ifndef SRC_INCLUDE_CANDEVICE_H_
#define SRC_INCLUDE_CANDEVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CanDeviceArguments.h"
#include "CanDiagnostics.h"
#include "CanFrame.h"

/**
 * @brief This struct represents a CAN device.
 *
 * It provides methods for opening, closing, sending, and receiving CanFrame,
 * as well as accessing device information and diagnostics.
 */
struct CanDevice {
  int open();
  int close();
  int send(const CanFrame& frame);
  std::vector<int> send(const std::vector<CanFrame>& frames);
  CanDiagnostics diagnostics();

  /**
   * @brief Returns the name of the CAN device vendor.
   *
   * This function retrieves the name of the vendor that manufactures the CAN
   * device.
   *
   * @return A string representing the name of the CAN device vendor.
   */
  inline std::string vendor_name() const { return m_vendor; }

  /**
   * @brief Returns a constant reference to the CanDeviceArguments object.
   *
   * This function provides access to the CanDeviceArguments object, which
   * contains configuration parameters for the CAN device.
   *
   * @return A constant reference to the CanDeviceArguments object.
   */
  inline const CanDeviceArguments& args() const { return m_args; }

  virtual ~CanDevice() = default;

  static std::unique_ptr<CanDevice> create(
      std::string_view vendor, const CanDeviceArguments& configuration);

 protected:
  /**
   * @brief Constructor for the CanDevice class.
   *
   * Initializes the CanDevice object with the provided vendor name and
   * CanDeviceArguments.
   *
   * @param vendor_name A string view representing the name of the CAN device
   * vendor.
   * @param args A constant reference to the CanDeviceArguments object, which
   * contains configuration parameters for the CAN device.
   */
  CanDevice(std::string_view vendor_name, const CanDeviceArguments& args)
      : m_vendor{vendor_name}, m_args{args} {}

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual int vendor_open() = 0;

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual int vendor_close() = 0;

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual int vendor_send(const CanFrame& frame) = 0;

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual CanDiagnostics vendor_diagnostics() = 0;

  /**
   * @brief Handles incoming CAN frames.
   *
   * This function is called whenever a CAN frame is received by the CAN device.
   * It passes the received frame to the receiver function specified in the
   * CanDeviceArguments object.
   *
   * @param frame A const reference to the received CanFrame.
   */
  void received(const CanFrame& frame) { m_args.receiver(frame); }

 private:
  const std::string m_vendor;
  const CanDeviceArguments m_args;
};

namespace CanDeviceError {
constexpr int NO_ERROR = 0;
constexpr int SUCCESS = 0;
constexpr int UNKNOWN_OPEN_ERROR = -1;
constexpr int SOCKET_ERROR = -2;
constexpr int TOO_MANY_CONNECTIONS = -3;
constexpr int TIMEOUT = -4;
constexpr int NOT_CONNECTED = -5;
constexpr int UNACKNOWLEDMENT = -6;
constexpr int INTERNAL_API_ERROR = -7;
constexpr int UNKNOWN_SEND_ERROR = -20;
constexpr int CAN_NACK = -21;
constexpr int CAN_TX_ERROR = -22;
constexpr int CAN_TX_BUFFER_OVERFLOW = -23;
constexpr int CAN_LOST_ARBITRATION = -24;
constexpr int CAN_INVALID_BITRATE = -25;
constexpr int UNKNOWN_CLOSE_ERROR = -40;
};  // namespace CanDeviceError

#endif  // SRC_INCLUDE_CANDEVICE_H_
