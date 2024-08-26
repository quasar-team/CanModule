#ifndef SRC_INCLUDE_CANLOGIT_H_
#define SRC_INCLUDE_CANLOGIT_H_

#include "CanVersion.h"
#include "LogIt.h"

namespace CanLogIt {
static const bool InitLogIt = Log::initializeLogging();
static const Log::LogComponentHandle h =
    Log::registerLoggingComponent("CanModule");
bool print_version();
static const bool print_version_init = print_version();
}  // namespace CanLogIt

#endif  // SRC_INCLUDE_CANLOGIT_H_
