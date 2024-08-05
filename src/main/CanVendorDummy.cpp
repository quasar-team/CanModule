#include "CanVendorDummy.h"

#include <iostream>

int CanVendorDummy::vendor_send(const CanFrame &frame) {
  std::cout << "Receive CAN frame: " << std::endl;
  m_configuration.receiver(frame);
  return 0;
}
