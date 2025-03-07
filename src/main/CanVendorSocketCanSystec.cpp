#include "CanVendorSocketCanSystec.h"

#include <CanLogIt.h>
#include <LogIt.h>
#include <linux/can/error.h>

/**
 * @brief Constructor for the CanVendorSocketCanSystec class.
 *
 * This constructor initializes the CanVendorSocketCanSystec object with the
 * provided arguments and sets up a callback function to handle bus off events.
 *
 * @param args A reference to a CanDeviceArguments object containing
 * configuration parameters for the CAN device.
 *
 * @note The constructor creates a new instance of the CanDevice class with the
 * specified vendor name ("socketcan_systec") and arguments. It also creates a
 * callback function that checks for bus off events and performs the necessary
 * actions (closing and reopening the CAN device). The callback function is then
 * used to initialize the m_can_vendor_socketcan member variable with a new
 * instance of the CanDevice class, passing the "socketcan" vendor name and a
 * new CanDeviceArguments object containing the configuration parameters and the
 * callback function.
 */
CanVendorSocketCanSystec::CanVendorSocketCanSystec(
    const CanDeviceArguments &args)
    : CanDevice("socketcan_systec", args) {
  const std::function<void(const CanFrame &)> filter_busoff_callback =
      [this](const CanFrame &frame) {
        if (frame.is_error()) {
          LOG(Log::WRN, CanLogIt::h()) << "Error frame received, restarting ";
          m_can_vendor_socketcan->close();
          std::this_thread::sleep_for(std::chrono::milliseconds(
              this->args().config.timeout.value_or(0)));
          m_can_vendor_socketcan->open();
        } else {
          received(frame);
        }
      };

  CanDeviceConfiguration config = args.config;
  config.timeout = 0;  // Systec causes a kernel panic if timeout/restart-ms is
                       // set to a non-zero value.

  m_can_vendor_socketcan = CanDevice::create(
      "socketcan", CanDeviceArguments{config, filter_busoff_callback});
}

/**
 * @brief Opens the CAN device for communication.
 *
 * This function attempts to open the CAN device associated with the
 * CanVendorSocketCanSystec instance. It checks if the device is initialized
 * and then calls the open method on the underlying CAN device.
 *
 * @return CanReturnCode indicating the result of the open operation.
 *         - If the device is successfully opened, the return code from the
 *           underlying CAN device's open method is returned.
 *         - If the device is not initialized, CanReturnCode::internal_api_error
 *           is returned.
 */
CanReturnCode CanVendorSocketCanSystec::vendor_open() noexcept {
  if (m_can_vendor_socketcan) return m_can_vendor_socketcan->open();
  return CanReturnCode::internal_api_error;
}

/**
 * @brief Closes the CAN device.
 *
 * This function attempts to close the CAN device associated with the
 * CanVendorSocketCanSystec instance. It checks if the device is initialized
 * and then calls the close method on the underlying CAN device.
 *
 * @return CanReturnCode indicating the result of the close operation.
 *         - If the device is successfully closed, the return code from the
 *           underlying CAN device's close method is returned.
 *         - If the device is not initialized, CanReturnCode::internal_api_error
 *           is returned.
 */
CanReturnCode CanVendorSocketCanSystec::vendor_close() noexcept {
  if (m_can_vendor_socketcan) return m_can_vendor_socketcan->close();
  return CanReturnCode::internal_api_error;
}

/**
 * @brief Sends a CAN frame through the CAN device.
 *
 * This function attempts to send a CAN frame using the CAN device associated
 * with the CanVendorSocketCanSystec instance. It checks if the device is
 * initialized and then calls the send method on the underlying CAN device.
 *
 * @param frame A reference to a CanFrame object representing the CAN frame to
 * be sent.
 *
 * @return CanReturnCode indicating the result of the send operation.
 *         - If the frame is successfully sent, the return code from the
 *           underlying CAN device's send method is returned.
 *         - If the device is not initialized, CanReturnCode::internal_api_error
 *           is returned.
 */
CanReturnCode CanVendorSocketCanSystec::vendor_send(
    const CanFrame &frame) noexcept {
  if (m_can_vendor_socketcan) return m_can_vendor_socketcan->send(frame);
  return CanReturnCode::internal_api_error;
}

/**
 * @brief Retrieves diagnostics information from the CAN device.
 *
 * This function attempts to obtain diagnostic data from the CAN device
 * associated with the CanVendorSocketCanSystec instance. It checks if the
 * device is initialized and then calls the diagnostics method on the
 * underlying CAN device.
 *
 * @return CanDiagnostics object containing diagnostic information.
 *         - If the device is initialized, the diagnostics data from the
 *           underlying CAN device is returned.
 *         - If the device is not initialized, an empty CanDiagnostics object
 *           is returned and an error is logged.
 */
CanDiagnostics CanVendorSocketCanSystec::vendor_diagnostics() noexcept {
  if (m_can_vendor_socketcan) {
    return m_can_vendor_socketcan->diagnostics();
  } else {
    LOG(Log::ERR, CanLogIt::h())
        << "SocketCAN device not initialized; I cannot retrieve diagnostics.";
    return CanDiagnostics();
  }
}
