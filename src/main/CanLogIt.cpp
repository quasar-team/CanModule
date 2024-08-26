#include "CanLogIt.h"

bool CanLogIt::print_version() {
  LOG(Log::INF, h) << "CAN Module SHA: " << CanModule::Version::value
                   << " Date: " << CanModule::Version::date;
  return true;
}
