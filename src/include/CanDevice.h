#ifndef SRC_INCLUDE_CANDEVICE_H_
#define SRC_INCLUDE_CANDEVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CanFrame.h"

struct CanDevice {
  int open() { return vendor_open(); }
  int close() { return vendor_close(); }
  int send(const CanFrame& frame);
  std::vector<int> send(const std::vector<CanFrame>& frames);

  inline std::string vendor_name() const { return m_vendor; }
  inline std::string configuration() const { return m_configuration; }

  static std::unique_ptr<CanDevice> create(std::string_view vendor,
                                           std::string_view configuration);

 protected:
  CanDevice(std::string_view vendor_name, std::string_view configuration)
      : m_vendor{vendor_name}, m_configuration{configuration} {}

  virtual int vendor_open() = 0;
  virtual int vendor_close() = 0;
  virtual int vendor_send(const CanFrame& frame) = 0;

  const std::string m_configuration;
  const std::string m_vendor;
};

#endif /* SRC_INCLUDE_CANDEVICE_H_ */
