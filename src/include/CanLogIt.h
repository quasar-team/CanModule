#ifndef SRC_INCLUDE_CANLOGIT_H_
#define SRC_INCLUDE_CANLOGIT_H_

#include "CanVersion.h"
#include "LogIt.h"

/**
 * @brief A utility class for logging within the CAN Module.
 *
 * This class provides a static function to initialize and register a logging
 * component for the CAN Module. The function ensures that the logging
 * initialization and registration only happen once using a static local
 * variable. It initializes the logging system using Log::initializeLogging()
 * and then registers a logging component named "CanModule" using
 * Log::registerLoggingComponent(). Afterwards, it logs the SHA and date of the
 * CAN Module using the registered logging component.
 */
struct CanLogIt {
  /**
   * @brief This function initializes and registers a logging component for the
   * CAN Module.
   *
   * @return A handle to the registered logging component.
   */
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
