#include "CanVendorSocketCan.h"

#include <libsocketcan.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <thread>  // NOLINT
#include <vector>

#include "CanFrame.h"
#include "CanLogIt.h"
// Constant defining how long the epoll wait cycle should be in milliseconds.
constexpr auto EPOLL_WAIT_CYCLE_MS = 1000;
constexpr auto LIBSOCKETCAN_ERROR = -1;
constexpr auto LIBSOCKETCAN_SUCCESS = 0;

/**
 * @brief Constructor for the CanVendorSocketCan class.
 *
 * Initializes the CanVendorSocketCan object with the provided arguments.
 *
 * @param args A constant reference to the CanDeviceArguments object,
 * which contains the arguments for the CAN device.
 */
CanVendorSocketCan::CanVendorSocketCan(const CanDeviceArguments& args)
    : CanDevice("socketcan", args) {
  if (!args.config.bus_name.has_value()) {
    throw std::invalid_argument("Missing required configuration parameters");
  }
}
/**
 * @brief Opens the SocketCAN device and sets up the necessary configurations.
 *
 * This function initializes the SocketCAN device by opening a socket, setting
 * up the CAN interface, binding the socket, creating an epoll instance, and
 * starting a subscriber thread to handle incoming CAN frames.
 *
 * @return int Returns 0 on success, or -1 on failure.
 */
CanReturnCode CanVendorSocketCan::vendor_open() noexcept {
  if (args().config.bitrate.has_value()) {
    LOG(Log::INF, CanLogIt::h()) << "Configuring SocketCAN device";

    if (can_do_stop(args().config.bus_name.value().c_str()) ==
        LIBSOCKETCAN_ERROR) {
      LOG(Log::WRN, CanLogIt::h()) << "Failed to stop CAN bus";
    }

    if (!args().config.vcan.value_or(false) &&
        can_set_bitrate(args().config.bus_name.value().c_str(),
                        args().config.bitrate.value()) == LIBSOCKETCAN_ERROR) {
      LOG(Log::ERR, CanLogIt::h()) << "Failed to set bitrate";
      return CanReturnCode::socket_error;
    }

    if (!args().config.vcan.value_or(false) &&
        can_set_restart_ms(args().config.bus_name.value().c_str(),
                           args().config.timeout.value_or(0)) ==
            LIBSOCKETCAN_ERROR) {
      LOG(Log::ERR, CanLogIt::h()) << "Failed to set restart delay";
      return CanReturnCode::socket_error;
    }

    if (can_do_start(args().config.bus_name.value().c_str()) ==
        LIBSOCKETCAN_ERROR) {
      LOG(Log::ERR, CanLogIt::h()) << "Failed to start CAN bus";
      return CanReturnCode::socket_error;
    }
  } else {
    LOG(Log::INF, CanLogIt::h()) << "Not configuring SocketCAN device";
  }

  // Open the SocketCAN device
  m_socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (m_socket_fd < 0) {
    LOG(Log::ERR, CanLogIt::h()) << "Failed to open socket";

    return CanReturnCode::socket_error;
  }

  // Set up the CAN interface
  struct ifreq ifr;
  strcpy(ifr.ifr_name,  // NOLINT: Recipe from Offical SocketCan documentation
         args().config.bus_name.value().c_str());
  if (ioctl(m_socket_fd, SIOCGIFINDEX, &ifr) < 0) {
    ::close(m_socket_fd);
    m_socket_fd = -1;
    LOG(Log::ERR, CanLogIt::h()) << "Failed to get interface index";
    return CanReturnCode::internal_api_error;
  }

  struct sockaddr_can addr;
  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(m_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    ::close(m_socket_fd);
    m_socket_fd = -1;
    LOG(Log::ERR, CanLogIt::h()) << "Failed to bind socket";
    return CanReturnCode::internal_api_error;
  }

  if (args().receiver != nullptr) {
    LOG(Log::DBG, CanLogIt::h()) << "Starting CAN subscriber thread";
    // Create epoll instance
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd < 0) {
      ::close(m_socket_fd);
      m_socket_fd = -1;
      LOG(Log::ERR, CanLogIt::h()) << "Failed to create epoll instance";

      return CanReturnCode::internal_api_error;
    }

    // Add the socket to the epoll instance
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = m_socket_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_socket_fd, &ev) < 0) {
      ::close(m_epoll_fd);
      ::close(m_socket_fd);
      m_epoll_fd = -1;
      m_socket_fd = -1;
      LOG(Log::ERR, CanLogIt::h()) << "Failed to add socket to epoll";

      return CanReturnCode::internal_api_error;
    }

    // Start the subscriber thread
    m_subcriber_run = true;
    m_subscriber_thread = std::thread(&CanVendorSocketCan::subscriber, this);
  } else {
    LOG(Log::INF, CanLogIt::h())
        << "Receiver callback not provided, not starting subscriber thread";
  }

  LOG(Log::DBG, CanLogIt::h()) << "SocketCan connection opened";

  return CanReturnCode::success;
}

/**
 * @brief Closes the SocketCAN device and cleans up resources.
 *
 * This function stops the subscriber thread, closes the epoll instance, and
 * closes the socket.
 *
 * @return int Returns 0 on success.
 */
CanReturnCode CanVendorSocketCan::vendor_close() noexcept {
  m_subcriber_run = false;

  // Stop the subscriber thread
  if (m_subscriber_thread.joinable()) {
    m_subscriber_thread.join();
  }

  if (m_epoll_fd >= 0 && ::close(m_epoll_fd) < 0) {
    LOG(Log::ERR, CanLogIt::h())
        << "Error while closing epoll: " << strerror(errno);
    return CanReturnCode::unknown_close_error;
  }
  m_epoll_fd = -1;
  if (m_socket_fd >= 0 && ::close(m_socket_fd) < 0) {
    LOG(Log::ERR, CanLogIt::h())
        << "Error while closing socket: " << strerror(errno);
    return CanReturnCode::unknown_close_error;
  }
  m_socket_fd = -1;
  LOG(Log::DBG, CanLogIt::h()) << "SocketCan connection closed";

  return CanReturnCode::success;
}

/**
 * @brief Sends a CAN frame over the SocketCAN interface.
 *
 * This function translates a CanFrame object to a struct can_frame and sends it
 * over the SocketCAN interface.
 *
 * @param frame The CanFrame object to be sent.
 *
 * @return int Returns 0 on success, or -1 if the entire frame could not be
 * sent.
 */
CanReturnCode CanVendorSocketCan::vendor_send(const CanFrame& frame) noexcept {
  // Translate CanFrame to struct can_frame
  struct can_frame canFrame = translate(frame);

  // Send the CAN frame
  int bytes_sent = ::send(m_socket_fd, &canFrame, sizeof(canFrame), 0);
  if (bytes_sent != sizeof(canFrame)) {
    LOG(Log::DBG, CanLogIt::h())
        << "SocketCan sent " << bytes_sent << " bytes, but expected "
        << sizeof(canFrame) << " bytes";

    return CanReturnCode::unknown_send_error;
  }

  return CanReturnCode::success;
}

/**
 * @brief Retrieves diagnostic information from the CAN device associated with
 * the current instance.
 *
 * This function retrieves diagnostic information from the CAN device associated
 * with the current instance. The diagnostic information is returned as a
 * CanDiagnostics object, which contains various parameters such as error
 * counters, bus status, and other relevant diagnostic data.
 *
 * @return A CanDiagnostics object containing the diagnostic information.
 *
 * @note The specific implementation of this function may vary depending on the
 * capabilities of the associated SocketCan device.
 */
CanDiagnostics CanVendorSocketCan::vendor_diagnostics() noexcept {
  CanDiagnostics diagnostics;

  diagnostics.name = args().config.bus_name.value();

  // Retrieve CAN interface state
  int state;
  if (can_get_state(args().config.bus_name.value().c_str(), &state) ==
      LIBSOCKETCAN_SUCCESS) {
    LOG(Log::DBG, CanLogIt::h()) << "CAN interface state enum: " << state;
    switch (state) {
      case CAN_STATE_ERROR_ACTIVE:
        diagnostics.state = "ERROR_ACTIVE";
        break;
      case CAN_STATE_ERROR_WARNING:
        diagnostics.state = "ERROR_WARNING";
        break;
      case CAN_STATE_ERROR_PASSIVE:
        diagnostics.state = "ERROR_PASSIVE";
        break;
      case CAN_STATE_BUS_OFF:
        diagnostics.state = "BUS_OFF";
        break;
      case CAN_STATE_STOPPED:
        diagnostics.state = "STOPPED";
        break;
      case CAN_STATE_SLEEPING:
        diagnostics.state = "SLEEPING";
        break;
      default:
        diagnostics.state = "UNKNOWN";
        break;
    }
  } else {
    LOG(Log::ERR, CanLogIt::h()) << "Failed to retrieve CAN interface state";
    diagnostics.state = "UNKNOWN";
  }

  // Retrieve link statistics
  struct rtnl_link_stats64 stats;
  if (can_get_link_stats(args().config.bus_name.value().c_str(), &stats) ==
      LIBSOCKETCAN_SUCCESS) {
    LOG(Log::DBG, CanLogIt::h()) << "CAN link statistics fetched";
    diagnostics.rx = stats.rx_packets;
    diagnostics.tx = stats.tx_packets;
    diagnostics.rx_error = stats.rx_errors;
    diagnostics.tx_error = stats.tx_errors;
    diagnostics.rx_drop = stats.rx_dropped;
    diagnostics.tx_drop = stats.tx_dropped;
  } else {
    LOG(Log::ERR, CanLogIt::h()) << "Failed to retrieve CAN link statistics";
  }

  // Retrieve bit timing
  struct can_bittiming bittiming;
  if (can_get_bittiming(args().config.bus_name.value().c_str(), &bittiming) ==
      LIBSOCKETCAN_SUCCESS) {
    diagnostics.bitrate = bittiming.bitrate;
  }

  // Retrieve device statistics
  struct can_device_stats device_stats;
  if (can_get_device_stats(args().config.bus_name.value().c_str(),
                           &device_stats) == LIBSOCKETCAN_SUCCESS) {
    diagnostics.bus_error = device_stats.bus_error;
    diagnostics.error_warning = device_stats.error_warning;
    diagnostics.error_passive = device_stats.error_passive;
    diagnostics.bus_off = device_stats.bus_off;
    diagnostics.arbitration_lost = device_stats.arbitration_lost;
    diagnostics.restarts = device_stats.restarts;
  }

  return diagnostics;
}

/**
 * @brief Translates a CanFrame object to a struct can_frame.
 *
 * This function converts a CanFrame object into a struct can_frame, which is
 * used for sending CAN frames over the SocketCAN interface.
 *
 * @param frame The CanFrame object to be translated.
 *
 * @return struct can_frame The translated CAN frame.
 */
struct can_frame CanVendorSocketCan::translate(const CanFrame& frame) noexcept {
  struct can_frame canFrame;
  canFrame.can_id = frame.id();
  canFrame.len = frame.length();

  if (frame.is_remote_request() == false) {
    for (auto i = 0u; i < frame.length(); ++i) {
      canFrame.data[i] = frame.message()[i];
    }
  }

  if (frame.is_remote_request()) {
    canFrame.can_id |= CAN_RTR_FLAG;
  }
  if (frame.is_extended_id()) {
    canFrame.can_id |= CAN_EFF_FLAG;
  }
  if (frame.is_error()) {
    canFrame.can_id |= CAN_ERR_FLAG;
  }

  return canFrame;
}

/**
 * @brief Translates a struct can_frame to a CanFrame object.
 *
 * This function converts a struct can_frame, which is received from the
 * SocketCAN interface, into a CanFrame object that can be used within the
 * application.
 *
 * @param canFrame The struct can_frame to be translated.
 *
 * @return CanFrame The translated CanFrame object.
 */
const CanFrame CanVendorSocketCan::translate(
    const struct can_frame& canFrame) noexcept {
  const auto id = canFrame.can_id & CAN_EFF_MASK;
  const auto length = static_cast<uint32_t>(canFrame.len);
  const auto rtr = (canFrame.can_id & CAN_RTR_FLAG);
  const auto error = (canFrame.can_id & CAN_ERR_FLAG);
  const auto eff = (canFrame.can_id & CAN_EFF_FLAG);
  const auto data = std::vector<char>(canFrame.data, canFrame.data + length);

  uint32_t flags{0};
  flags |= rtr ? can_flags::remote_request : 0;
  flags |= error ? can_flags::error_frame : 0;
  flags |= eff ? can_flags::extended_id : 0;

  if (rtr) {
    return CanFrame{id, length, flags};
  }
  return CanFrame{id, data, flags};
}

/**
 * @brief Handles incoming CAN frames using epoll for event-driven I/O.
 *
 * This function runs in a loop, waiting for events on the CAN socket using
 * epoll. When a CAN frame is received, it reads the frame, translates it to a
 * CanFrame object, and calls the received method with the translated frame.
 *
 * @return int Returns 0 on success, or -1 if an error occurs during epoll_wait
 * or reading from the socket.
 */
int CanVendorSocketCan::subscriber() noexcept {
  struct epoll_event events[1];
  while (m_subcriber_run) {
    int nfds = epoll_wait(m_epoll_fd, events, 1, EPOLL_WAIT_CYCLE_MS);
    if (nfds < 0) {
      if (errno == EINTR) {
        LOG(Log::DBG, CanLogIt::h())
            << "Interrupted by signal, continue waiting";
        continue;
      } else {
        LOG(Log::ERR, CanLogIt::h())
            << "Error occurred during epoll_wait: " << strerror(errno);
        return -1;  // Error occurred
      }
    }

    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == m_socket_fd) {
        struct can_frame canFrame;
        int nbytes = ::read(m_socket_fd, &canFrame, sizeof(struct can_frame));
        if (nbytes < 0) {
          LOG(Log::ERR, CanLogIt::h())
              << "Unexpected error reading from socket, exiting";
          return -1;
        }

        if (nbytes == sizeof(canFrame)) {
          LOG(Log::DBG, CanLogIt::h()) << "Received CAN frame via SocketCAN";

          CanFrame frame = translate(canFrame);
          received(
              frame);  // Call the received method with the translated frame
        } else {
          LOG(Log::ERR, CanLogIt::h()) << "Corrupted CanFrame received";
        }
      }
    }
  }
  LOG(Log::DBG, CanLogIt::h()) << "Exiting subscriber loop";
  return 0;  // Success
}
