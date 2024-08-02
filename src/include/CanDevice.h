#ifndef SRC_INCLUDE_CANDEVICE_H_
#define SRC_INCLUDE_CANDEVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CanFrame.h"

struct CanDevice {
  inline int open(std::function<void(const CanFrame&)> receiver = nullptr) {
    m_receiver = receiver;
    return vendor_open();
  }
  inline int close() {
    m_receiver = nullptr;
    return vendor_close();
  }
  inline int send(const CanFrame& frame);
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

  const std::string m_vendor;
  const std::string m_configuration;

 private:
  std::function<void(const CanFrame&)> m_receiver;
};

#endif /* SRC_INCLUDE_CANDEVICE_H_ */
