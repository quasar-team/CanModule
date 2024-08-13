#ifndef SRC_INCLUDE_CANDEVICE_H_
#define SRC_INCLUDE_CANDEVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CanDeviceConfig.h"
#include "CanFrame.h"

struct CanDevice {
  int open();
  int close();
  int send(const CanFrame& frame);
  std::vector<int> send(const std::vector<CanFrame>& frames);

  inline std::string vendor_name() const { return m_vendor; }
  inline const CanDeviceConfig& configuration() const {
    return m_configuration;
  }

  static std::unique_ptr<CanDevice> create(
      std::string_view vendor, const CanDeviceConfig& configuration);

 protected:
  CanDevice(std::string_view vendor_name, const CanDeviceConfig& configuration)
      : m_vendor{vendor_name}, m_configuration{configuration} {}

  virtual int vendor_open() = 0;
  virtual int vendor_close() = 0;
  virtual int vendor_send(const CanFrame& frame) = 0;
  void received(const CanFrame& frame) { m_configuration.receiver(frame); }

 private:
  const std::string m_vendor;
  const CanDeviceConfig m_configuration;
};

#endif  // SRC_INCLUDE_CANDEVICE_H_
