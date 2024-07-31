#include "CanFrame.hpp"

/**
 * @brief Constructs a CanFrame object with a specified ID and requested length.
 * 
 * @param id The identifier for the CAN frame.
 * @param requested_length The length of the data to be transmitted in the CAN frame.
 */
CanFrame::CanFrame(uint32_t id, uint32_t requested_length)
{
}

CanFrame::CanFrame(uint32_t id, uint32_t requested_length, uint32_t flags)
{
}

CanFrame::CanFrame(uint32_t id, const std::vector<char> &message)
{
}

CanFrame::CanFrame(uint32_t id, const std::vector<char> &message, uint32_t flags)
{
}
