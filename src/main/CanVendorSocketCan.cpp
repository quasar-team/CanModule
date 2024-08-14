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
int CanVendorSocketCan::vendor_open() {
  // Open the SocketCAN device
  socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd < 0) {
    return -1;  // Failed to open socket
  }

  // Set up the CAN interface
  struct ifreq ifr;
  strcpy(ifr.ifr_name,  // NOLINT: Recipe from Offical SocketCan documentation
         "can0");
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
  subscriber_thread = std::thread(&CanVendorSocketCan::subscriber, this);

  return 0;  // Success
}

int CanVendorSocketCan::vendor_close() {
  // Stop the subscriber thread
  if (subscriber_thread.joinable()) {
    // Signal the subscriber thread to stop
    // This can be done by closing the epoll file descriptor
    // which will cause the epoll_wait to return and the thread to exit
    ::close(epoll_fd);
    subscriber_thread.join();
  }

  // Close the SocketCAN device
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
  canFrame.can_dlc = frame.message().size();
  std::copy(frame.message().begin(), frame.message().end(), canFrame.data);
  return canFrame;
}

CanFrame CanVendorSocketCan::translate(const struct can_frame &canFrame) {
  CanFrame frame(
      canFrame.can_id,
      std::vector<char>(canFrame.data, canFrame.data + canFrame.can_dlc));
  return frame;
}

int CanVendorSocketCan::subscriber() {
  struct epoll_event events[10];  // Array to hold epoll events
  while (true) {
    int nfds =
        epoll_wait(epoll_fd, events, 10, -1);  // Wait indefinitely for events
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
        int nbytes = ::read(socket_fd, &canFrame, sizeof(canFrame));
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
