#ifndef SRC_INCLUDE_CANDEVICE_H_
#define SRC_INCLUDE_CANDEVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CanDerivedStats.h"
#include "CanDeviceArguments.h"
#include "CanDiagnostics.h"
#include "CanFrame.h"
#include "CanLogIt.h"

enum class CanReturnCode {
  success,
  unknown_open_error,
  socket_error,
  too_many_connections,
  timeout,
  disconnected,
  internal_api_error,
  unknown_send_error,
  not_ack,
  rx_error,
  tx_error,
  tx_buffer_overflow,
  lost_arbitration,
  invalid_bitrate,
  unknown_close_error,
};

std::ostream& operator<<(std::ostream& os, CanReturnCode code);

/**
 * @brief This struct represents a CAN device.
 *
 * It provides methods for opening, closing, sending, and receiving CanFrame,
 * as well as accessing device information and diagnostics.
 */
struct CanDevice {
  CanReturnCode open() noexcept;
  CanReturnCode close() noexcept;
  CanReturnCode send(const CanFrame& frame) noexcept;
  std::vector<CanReturnCode> send(const std::vector<CanFrame>& frames) noexcept;
  CanDiagnostics diagnostics() noexcept;

  /**
   * @brief Returns the name of the CAN device vendor.
   *
   * This function retrieves the name of the vendor that manufactures the CAN
   * device.
   *
   * @return A string representing the name of the CAN device vendor.
   */
  inline std::string vendor_name() const noexcept { return m_vendor; }

  /**
   * @brief Returns a constant reference to the CanDeviceArguments object.
   *
   * This function provides access to the CanDeviceArguments object, which
   * contains configuration parameters for the CAN device.
   *
   * @return A constant reference to the CanDeviceArguments object.
   */
  inline const CanDeviceArguments& args() const noexcept { return m_args; }

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
  inline CanDevice(std::string_view vendor_name, const CanDeviceArguments& args)
      : m_vendor{vendor_name}, m_args{args} {}

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual CanReturnCode vendor_open() noexcept = 0;

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual CanReturnCode vendor_close() noexcept = 0;

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual CanReturnCode vendor_send(const CanFrame& frame) noexcept = 0;

  /**
   * @brief Pure virtual functions for vendor-specific CAN device operations.
   *
   * These functions are intended to be overridden by specific vendor
   * implementations to provide the necessary functionality for opening,
   * closing, sending, and retrieving diagnostics from the CAN device.
   */
  virtual CanDiagnostics vendor_diagnostics() noexcept = 0;

  /**
   * @brief Handles incoming CAN frames.
   *
   * This function is called whenever a CAN frame is received by the CAN device.
   * It passes the received frame to the receiver function specified in the
   * CanDeviceArguments object.
   *
   * @param frame A const reference to the received CanFrame.
   */
  inline void received(const CanFrame& frame) const noexcept {
    LOG(Log::DBG, CanLogIt::h()) << "Received CAN frame: " << frame;
    if (m_args.receiver != nullptr) {
      LOG(Log::DBG, CanLogIt::h()) << "Calling receiver function";
      m_args.receiver(frame);
    }
  }

 private:
  const std::string m_vendor;
  const CanDeviceArguments m_args;
  CanDerivedStats m_derived_stats;
};

#endif  // SRC_INCLUDE_CANDEVICE_H_
