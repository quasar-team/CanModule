#include "CanDevice.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "CanLogIt.h"
#include "CanVendorAnagate.h"
#include "CanVendorLoopback.h"

#ifndef _WIN32
#include "CanVendorSocketCan.h"
#endif

/**
 * @brief Opens the CAN device for communication.
 *
 * This function initializes and opens the CAN device using the vendor-specific
 * implementation.
 *
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
CanReturnCode CanDevice::open() {
  LOG(Log::INF, CanLogIt::h()) << "Opening CAN device " << m_vendor;
  LOG(Log::INF, CanLogIt::h()) << "Configuration: " << m_args.config;

  CanReturnCode result = vendor_open();

  if (result != CanReturnCode::success) {
    LOG(Log::ERR, CanLogIt::h())
        << "Failed to open CAN device: error code " << result;
  } else {
    LOG(Log::DBG, CanLogIt::h()) << "Successfully opened CAN device";
  }

  return result;
}
/**
 * @brief Closes the CAN device.
 *
 * This function finalizes and closes the CAN device using the vendor-specific
 * implementation.
 *
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
CanReturnCode CanDevice::close() {
  LOG(Log::INF, CanLogIt::h()) << "Closing CAN device " << m_vendor;

  CanReturnCode result = vendor_close();

  if (result != CanReturnCode::success) {
    LOG(Log::ERR, CanLogIt::h())
        << "Failed to close CAN device: error code " << result;
  } else {
    LOG(Log::DBG, CanLogIt::h()) << "Successfully closed CAN device";
  }

  return result;
}

/**
 * @brief Sends a CAN frame.
 *
 * This function sends a single CAN frame using the vendor-specific
 * implementation.
 *
 * @param frame The CAN frame to be sent. It must be a valid frame.
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
CanReturnCode CanDevice::send(const CanFrame &frame) {
  LOG(Log::DBG, CanLogIt::h()) << "Sending CAN frame: " << frame;

  CanReturnCode result = vendor_send(frame);

  if (result != CanReturnCode::success) {
    LOG(Log::ERR, CanLogIt::h())
        << "Failed to send CAN frame: error code " << result;
  } else {
    LOG(Log::TRC, CanLogIt::h()) << "Successfully sent CAN frame: " << frame;
  }

  return result;
}

/**
 * @brief Sends multiple CAN frames.
 *
 * This function sends a list of CAN frames using the vendor-specific
 * implementation. It returns a vector of integers where each integer represents
 * the result of sending the corresponding CAN frame.
 *
 * @param frames A vector of CAN frames to be sent. Each frame must be a valid
 * frame.
 * @return std::vector<int> A vector of integers where each integer is 0 on
 * success or a non-zero error code on failure for the corresponding frame.
 */
std::vector<CanReturnCode> CanDevice::send(
    const std::vector<CanFrame> &frames) {
  std::vector<CanReturnCode> result(frames.size());
  std::transform(frames.begin(), frames.end(), result.begin(),
                 [this](const CanFrame &frame) { return send(frame); });
  return result;
}

/**
 * @brief Retrieves diagnostic information from the CAN device.
 *
 * This function retrieves and returns diagnostic information from the CAN
 * device using the vendor-specific implementation. The returned object contains
 * various parameters such as error counters, bus status, and other relevant
 * diagnostic data.
 *
 * @return CanDiagnostics A structure containing diagnostic information.
 * The structure is defined in the CanDiagnostics.h header file. Each field is
 * optional and depends on the vendor-specific implementation.
 */
CanDiagnostics CanDevice::diagnostics() { return vendor_diagnostics(); }

/**
 * @brief Creates a CAN device instance based on the specified vendor.
 *
 * This function creates and returns a unique pointer to a CAN device object
 * that corresponds to the specified vendor. The created device is configured
 * using the provided configuration.
 *
 * @param vendor A string view representing the vendor name. Supported values
 * are "loopback", "socketcan" (only Linux platforms or Windows with WSL), and
 * "anagate".
 * @param configuration A reference to a CanDeviceArguments object that contains
 * the configuration settings for the CAN device.
 * @return std::unique_ptr<CanDevice> A unique pointer to the created CAN device
 * object. Returns nullptr if the vendor is not recognized.
 */
std::unique_ptr<CanDevice> CanDevice::create(
    std::string_view vendor, const CanDeviceArguments &configuration) {
  LOG(Log::INF, CanLogIt::h()) << "Creating CAN device for vendor: " << vendor;
  LOG(Log::INF, CanLogIt::h()) << "Configuration: " << configuration.config;

  if (vendor == "loopback") {
    LOG(Log::DBG, CanLogIt::h()) << "Creating Loopback CAN device";
    return std::make_unique<CanVendorLoopback>(configuration);
  }

#ifndef _WIN32
  if (vendor == "socketcan") {
    LOG(Log::DBG, CanLogIt::h()) << "Creating SocketCAN CAN device";
    return std::make_unique<CanVendorSocketCan>(configuration);
  }
#endif

  if (vendor == "anagate") {
    LOG(Log::DBG, CanLogIt::h()) << "Creating Anagate CAN device";
    return std::make_unique<CanVendorAnagate>(configuration);
  }

  LOG(Log::ERR, CanLogIt::h()) << "Unrecognized CAN device vendor: " << vendor;
  return nullptr;
}

std::ostream &operator<<(std::ostream &os, CanReturnCode code) {
  switch (code) {
    case CanReturnCode::success:
      return os << "SUCCESS";
    case CanReturnCode::unknown_open_error:
      return os << "UNKNOWN_OPEN_ERROR";
    case CanReturnCode::socket_error:
      return os << "SOCKET_ERROR";
    case CanReturnCode::too_many_connections:
      return os << "TOO_MANY_CONNECTIONS";
    case CanReturnCode::timeout:
      return os << "TIMEOUT";
    case CanReturnCode::disconnected:
      return os << "NOT_CONNECTED";
    case CanReturnCode::internal_api_error:
      return os << "INTERNAL_API_ERROR";
    case CanReturnCode::unknown_send_error:
      return os << "UNKNOWN_SEND_ERROR";
    case CanReturnCode::not_ack:
      return os << "CAN_NACK";
    case CanReturnCode::tx_error:
      return os << "CAN_TX_ERROR";
    case CanReturnCode::tx_buffer_overflow:
      return os << "CAN_TX_BUFFER_OVERFLOW";
    case CanReturnCode::lost_arbitration:
      return os << "CAN_LOST_ARBITRATION";
    case CanReturnCode::invalid_bitrate:
      return os << "CAN_INVALID_BITRATE";
    case CanReturnCode::unknown_close_error:
      return os << "UNKNOWN_CLOSE_ERROR";
    default:
      return os << "Unknown";
  }
}
