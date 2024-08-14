#ifndef SRC_INCLUDE_CANDIAGNOSTIC_H_
#define SRC_INCLUDE_CANDIAGNOSTIC_H_

#include <string>
#include <vector>

#include "CanVersion.h"

struct CanDiagnostic {
  std::vector<std::string> log_entries;
  std::string last_error;
  std::vector<std::string> connected_clients;
  constexpr std::string_view canmodule_version{CanModule::Version::value};
  constexpr std::string_view canmodule_date{CanModule::Version::date};
};

#endif  // SRC_INCLUDE_CANDIAGNOSTIC_H_
