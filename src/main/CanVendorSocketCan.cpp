#include "CanVendorSocketCan.h"

#include <libsocketcan.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <thread>  // NOLINT

#include "CanFrame.h"

// Constant defining how long the epoll wait cycle should be in milliseconds.
constexpr auto EPOLL_WAIT_CYCLE_MS = 1000;
constexpr auto LIBSOCKETCAN_ERROR = -1;
constexpr auto LIBSOCKETCAN_SUCCESS = 0;

/**
 * @brief Opens the SocketCAN device and sets up the necessary configurations.
 *
 * This function initializes the SocketCAN device by opening a socket, setting
 * up the CAN interface, binding the socket, creating an epoll instance, and
 * starting a subscriber thread to handle incoming CAN frames.
 *
 * @return int Returns 0 on success, or -1 on failure.
 */
int CanVendorSocketCan::vendor_open() {
  if (args().config.bitrate.has_value()) {
    if (can_do_stop(args().config.bus_name.value().c_str()) ==
        LIBSOCKETCAN_ERROR) {
      return -1;  // Failed to stop CAN bus
    }
    if (can_set_bitrate(args().config.bus_name.value().c_str(),
                        args().config.bitrate.value()) == LIBSOCKETCAN_ERROR) {
      return -1;  // Failed to set bitrate
    }
    if (can_set_restart_ms(args().config.bus_name.value().c_str(),
                           args().config.timeout.value_or(0)) ==
        LIBSOCKETCAN_ERROR) {
      // Failed to set restart delay
    }
    if (can_do_start(args().config.bus_name.value().c_str()) ==
        LIBSOCKETCAN_ERROR) {
      return -1;  // Failed to start CAN bus
    }
  } else {
    // Not reconfigure socketcan device
  }

  // Open the SocketCAN device
  socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd < 0) {
    return -1;  // Failed to open socket
  }

  // Set up the CAN interface
  struct ifreq ifr;
  strcpy(ifr.ifr_name,  // NOLINT: Recipe from Offical SocketCan documentation
         args().config.bus_name.value().c_str());
  if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) < 0) {
    ::close(socket_fd);
    return -1;  // Failed to get interface index
  }

  struct sockaddr_can addr;
  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    ::close(socket_fd);
    return -1;  // Failed to bind socket
  }

  // Create epoll instance
  epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    ::close(socket_fd);
    return -1;  // Failed to create epoll instance
  }

  // Add the socket to the epoll instance
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = socket_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) < 0) {
    ::close(epoll_fd);
    ::close(socket_fd);
    return -1;  // Failed to add socket to epoll
  }

  // Start the subscriber thread
  m_subcriber_run = true;
  subscriber_thread = std::thread(&CanVendorSocketCan::subscriber, this);

  return 0;  // Success
}

/**
 * @brief Closes the SocketCAN device and cleans up resources.
 *
 * This function stops the subscriber thread, closes the epoll instance, and
 * closes the socket.
 *
 * @return int Returns 0 on success.
 */
int CanVendorSocketCan::vendor_close() {
  m_subcriber_run = false;

  // Stop the subscriber thread
  if (subscriber_thread.joinable()) {
    subscriber_thread.join();
  }

  ::close(epoll_fd);
  if (socket_fd >= 0) {
    ::close(socket_fd);
  }

  return 0;  // Success
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
int CanVendorSocketCan::vendor_send(const CanFrame &frame) {
  // Translate CanFrame to struct can_frame
  struct can_frame canFrame = translate(frame);

  // Send the CAN frame
  int bytes_sent = ::send(socket_fd, &canFrame, sizeof(canFrame), 0);
  if (bytes_sent != sizeof(canFrame)) {
    return -1;  // Failed to send the entire frame
  }

  return 0;  // Success
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
struct can_frame CanVendorSocketCan::translate(const CanFrame &frame) {
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
const CanFrame CanVendorSocketCan::translate(const struct can_frame &canFrame) {
  const auto id = canFrame.can_id & CAN_EFF_MASK;
  const auto length = static_cast<uint32_t>(canFrame.len);
  const auto rtr = (canFrame.can_id & CAN_RTR_FLAG);
  const auto error = (canFrame.can_id & CAN_ERR_FLAG);
  const auto eff = (canFrame.can_id & CAN_EFF_FLAG);
  const auto data = std::vector<char>(canFrame.data, canFrame.data + length);

  uint32_t flags{0};
  flags |= rtr ? CanFlags::REMOTE_REQUEST : 0;
  flags |= error ? CanFlags::ERROR_FRAME : 0;
  flags |= eff ? CanFlags::EXTENDED_ID : 0;

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
int CanVendorSocketCan::subscriber() {
  struct epoll_event events[1];
  while (m_subcriber_run) {
    int nfds = epoll_wait(epoll_fd, events, 1, EPOLL_WAIT_CYCLE_MS);
    if (nfds < 0) {
      if (errno == EINTR) {
        continue;  // Interrupted by signal, continue waiting
      } else {
        return -1;  // Error occurred
      }
    }

    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == socket_fd) {
        struct can_frame canFrame;
        int nbytes = ::read(socket_fd, &canFrame, sizeof(struct can_frame));
        if (nbytes < 0) {
          return -1;  // Error reading from socket
        }

        if (nbytes == sizeof(canFrame)) {
          CanFrame frame = translate(canFrame);
          received(
              frame);  // Call the received method with the translated frame
        }
      }
    }
  }

  return 0;  // Success
}
