#ifndef SRC_INCLUDE_CANLOGIT_H_
#define SRC_INCLUDE_CANLOGIT_H_

#include "LogIt.h"

namespace CanModule::LogIt {
static const bool InitLogIt = Log::initializeLogging();
static const Log::LogComponentHandle log_handle =
    Log::registerLoggingComponent("CanModule", Log::INF);
LOG(CanModule::LogIt::log_handle, Log::DBG)
    << "CanModule initialized successfully";

}  // namespace CanModule::LogIt

#endif  // SRC_INCLUDE_CANLOGIT_H_
