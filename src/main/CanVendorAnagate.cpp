#include "CanVendorAnagate.h"

#include <AnaGateDllCan.h>

#include <iostream>
#include <memory>
#include <mutex>  // NOLINT

std::mutex CanVendorAnagate::m_handles_lock;
std::map<int, CanVendorAnagate *> CanVendorAnagate::m_handles;

/**
 * @brief Callback function to handle incoming CAN frames from the AnaGate DLL.
 *
 * This function is called by the AnaGate DLL whenever a CAN frame is received.
 * It extracts the necessary information from the parameters and constructs a
 * CanFrame object, which is then passed to the appropriate CanVendorAnagate
 * instance.
 *
 * @param nIdentifier The identifier of the received CAN frame.
 * @param pcBuffer A pointer to the data buffer containing the received CAN
 * frame.
 * @param nBufferLen The length of the received CAN frame data.
 * @param nFlags The flags associated with the received CAN frame.
 * @param hHandle The handle of the CAN device associated with the received CAN
 * frame.
 */
void anagate_receive(AnaInt32 nIdentifier, const char *pcBuffer,
                     AnaInt32 nBufferLen, AnaInt32 nFlags, AnaInt32 hHandle) {
  uint32_t flags{0};
  (nFlags & AnagateConstants::EXTENDED_ID) ? flags |= CanFlags::EXTENDED_ID : 0;
  (nFlags & AnagateConstants::REMOTE_REQUEST)
      ? flags |= CanFlags::REMOTE_REQUEST
      : 0;
  auto o = CanVendorAnagate::m_handles[hHandle];
  if (o != nullptr) {
    if (flags & CanFlags::REMOTE_REQUEST) {
      o->received(CanFrame{static_cast<uint32_t>(nIdentifier),
                           static_cast<uint32_t>(nBufferLen), flags});
    } else {
      o->received(CanFrame{static_cast<uint32_t>(nIdentifier),
                           std::vector<char>(pcBuffer, pcBuffer + nBufferLen),
                           flags});
    }
  }
}

/**
 * @brief Opens a connection to the CAN device associated with the current
 * instance.
 *
 * This function opens a connection to the CAN device associated with the
 * current instance using the provided configuration parameters. It calls the
 * CANOpenDevice function from the AnaGate DLL to establish the connection and
 * sets a callback function to handle incoming CAN frames. The opened device
 * handle is stored in the m_handle member variable.
 *
 * @return An integer representing the status of the operation. A value of 0
 * indicates success, while a non-zero value indicates an error. The specific
 * meaning of the error code can be obtained by calling CANGetLastError.
 */
int CanVendorAnagate::vendor_open() {
  const AnaInt32 result = CANOpenDevice(
      &m_handle, args().config.sent_acknowledgement.value_or(false), true,
      args().config.bus_address.value(), args().config.host.value().c_str(),
      args().config.timeout.value_or(AnagateConstants::default_timeout));
  CANSetCallback(m_handle, reinterpret_cast<CAN_PF_CALLBACK>(anagate_receive));
  std::lock_guard<std::mutex> guard(CanVendorAnagate::m_handles_lock);
  CanVendorAnagate::m_handles[m_handle] = this;
  return static_cast<int>(result);
}

/**
 * @brief Sends a CAN frame to the associated device.
 *
 * This function sends a CAN frame to the CAN device associated with the current
 * instance. It prepares the necessary flags based on the properties of the CAN
 * frame and then calls the appropriate CANWrite function from the AnaGate DLL.
 *
 * @param frame A const reference to the CanFrame object to be sent.
 *
 * @return An integer representing the status of the operation. A value of 0
 * indicates success, while a non-zero value indicates an error. The specific
 * meaning of the error code can be obtained by calling CANGetLastError.
 */
int CanVendorAnagate::vendor_send(const CanFrame &frame) {
  int anagate_flags{0};
  anagate_flags |= frame.is_extended_id() ? AnagateConstants::EXTENDED_ID : 0;
  anagate_flags |=
      frame.is_remote_request() ? AnagateConstants::REMOTE_REQUEST : 0;

  const int sent_status =
      frame.is_remote_request()
          ? CANWrite(m_handle, frame.id(), AnagateConstants::empty_message,
                     frame.length(), anagate_flags)
          : CANWrite(m_handle, frame.id(), frame.message().data(),
                     frame.length(), anagate_flags);
  return sent_status;
}

/**
 * @brief Closes the CAN device associated with the current instance.
 *
 * This function closes the CAN device associated with the current instance by
 * calling the CANCloseDevice function from the AnaGate DLL. It also removes the
 * instance from the m_handles map to prevent any further use of the closed
 * device.
 *
 * @return An integer representing the status of the operation. A value of 0
 * indicates success, while a non-zero value indicates an error. The specific
 * meaning of the error code can be obtained by calling CANGetLastError.
 */
int CanVendorAnagate::vendor_close() {
  const int r = CANCloseDevice(m_handle);
  std::lock_guard<std::mutex> guard(CanVendorAnagate::m_handles_lock);
  CanVendorAnagate::m_handles.erase(m_handle);
  return r;
}
