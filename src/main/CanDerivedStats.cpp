#include "CanDerivedStats.h"

#include <deque>

#include "CanLogIt.h"

/**
 * @brief Updates the derived statistics for CAN diagnostics.
 *
 * This function calculates the rate of received (Rx) and transmitted (Tx)
 * messages per second and updates the diagnostics object with these values.
 * It initializes the statistics if they haven't been initialized yet.
 *
 * @param diagnostics A reference to a CanDiagnostics object that contains
 *                    the current Rx and Tx values and will be updated with
 *                    the calculated rolling average rates.
 */
void CanDerivedStats::update(CanDiagnostics &diagnostics) noexcept {
  if (!m_init) {
    if (!diagnostics.rx.has_value() || !diagnostics.tx.has_value()) {
      m_enabled = false;
      m_init = true;
      LOG(Log::INF, CanLogIt::h()) << "No Rx or Tx values present, disabling "
                                      "support for derived statistics";
      return;
    }

    m_rx = diagnostics.rx.value();
    m_tx = diagnostics.tx.value();
    m_last_update = std::chrono::system_clock::now();
    m_init = true;
    m_enabled = true;
    LOG(Log::INF, CanLogIt::h())
        << "Rx and Tx values present, enabled support for derived statistics";
    LOG(Log::DBG, CanLogIt::h()) << "Initial Rx: " << m_rx;
    LOG(Log::DBG, CanLogIt::h()) << "Initial Tx: " << m_tx;
    return;
  }

  if (!m_init || !m_enabled || !diagnostics.rx.has_value() ||
      !diagnostics.tx.has_value()) {
    return;
  }

  auto current_time = std::chrono::system_clock::now();
  auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
                       current_time - m_last_update)
                       .count();

  if (time_diff > 0) {
    const auto update_rate = [&](std::deque<double> &window,
                                 std::optional<uint32_t> &value,
                                 uint32_t &last_value, int64_t time_diff) {
      double rate = static_cast<double>(value.value() - last_value) / time_diff;
      window.push_front(rate);
      if (window.size() > window_size) {
        window.pop_back();
      }
      last_value = value.value();
    };

    update_rate(m_rx_per_second_window, diagnostics.rx, m_rx, time_diff);
    update_rate(m_tx_per_second_window, diagnostics.tx, m_tx, time_diff);

    m_last_update = current_time;

    const auto update_per_second = [&](const std::deque<double> &window,
                                       std::optional<double> &per_second) {
      if (!window.empty()) {
        per_second =
            std::accumulate(window.begin(), window.end(), 0.0) / window.size();
      }
    };

    update_per_second(m_rx_per_second_window, diagnostics.rx_per_second);
    update_per_second(m_tx_per_second_window, diagnostics.tx_per_second);
  }
}
