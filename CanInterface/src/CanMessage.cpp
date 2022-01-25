#include <iomanip>
#include <sstream>

#include <CanMessage.h>

std::string CanMessage::toString() const
{
    std::stringstream ss;
    ss << "<id=" << std::hex << c_id << " ";
    if (c_rtr)
        ss << "RTR ";
    for (unsigned int i=0; i < c_dlc; i++)
        ss << " " << std::hex << std::setw(2) << (unsigned int)c_data[i];
    ss << ">";
    return ss.str();
}