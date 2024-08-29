#ifndef SRC_INCLUDE_CANLOGIT_H_
#define SRC_INCLUDE_CANLOGIT_H_

#include "CanVersion.h"
#include "LogIt.h"

struct CanLogIt {
  inline static Log::LogComponentHandle h() {
    static Log::LogComponentHandle handle = [] {
      Log::initializeLogging();
      Log::LogComponentHandle h = Log::registerLoggingComponent("CanModule");
      LOG(Log::INF, h) << "CAN Module SHA: " << CanModule::Version::value
                       << " Date: " << CanModule::Version::date;
      return h;
    }();

    return handle;
  }
};

#endif  // SRC_INCLUDE_CANLOGIT_H_
