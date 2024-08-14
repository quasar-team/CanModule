#include "CanVendorSocketCan.h"

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
int CanVendorSocketCan::vendor_open() {
  // Open the SocketCAN device
  socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd < 0) {
    return -1;  // Failed to open socket
  }

  // Set up the CAN interface
  struct ifreq ifr;
  strcpy(ifr.ifr_name,  // NOLINT: Recipe from Offical SocketCan documentation
         configuration().vendor_config.c_str());
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

int CanVendorSocketCan::subscriber() {
  struct epoll_event events[10];  // Array to hold epoll events
  while (m_subcriber_run) {
    int nfds =
        epoll_wait(epoll_fd, events, 10, 1000);  // Wait indefinitely for events
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
