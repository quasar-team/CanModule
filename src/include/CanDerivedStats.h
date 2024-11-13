#ifndef SRC_INCLUDE_CANDERIVEDSTATS_H_
#define SRC_INCLUDE_CANDERIVEDSTATS_H_

#include <chrono>
#include <deque>
#include <numeric>

#include "CanDiagnostics.h"

constexpr uint32_t window_size = 20;

struct CanDerivedStats {
  CanDerivedStats() {}
  void init(const CanDiagnostics& diagnostics);
  void update(CanDiagnostics& diagnostics) noexcept;  // NOLINT

 private:
  uint32_t m_rx{0}, m_tx{0};
  bool m_enabled{true};
  std::deque<double> m_rx_per_second_window, m_tx_per_second_window;
  std::chrono::time_point<std::chrono::system_clock> m_last_update;
}

#endif  // SRC_INCLUDE_CANDERIVEDSTATS_H_
