#include "CanVendorLoopback.h"

int CanVendorLoopback::vendor_send(const CanFrame &frame) {
  received(frame);
  return 0;
}
