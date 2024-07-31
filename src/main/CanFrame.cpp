#include "CanFrame.h"

CanFrame::CanFrame(uint32_t id, uint32_t requested_length, uint32_t flags) {
  invalid_frame(id, requested_length, message, flags);
}

CanFrame::CanFrame(uint32_t id, const std::vector<char> &message,
                   uint32_t flags) {}

bool CanFrame::is_id_standard(uint32_t id) { return (id & 0xFFFFF800) == 0; }

bool CanFrame::is_id_extended(uint32_t id) { return (id & 0xE0000000) == 0; }
