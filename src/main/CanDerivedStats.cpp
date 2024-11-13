#include "CanDerivedStats.h"

#include "CanLogIt.h"

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
    double rx_rate =
        static_cast<double>(diagnostics.rx.value() - m_rx) / time_diff;

    double tx_rate =
        static_cast<double>(diagnostics.tx.value() - m_tx) / time_diff;

    m_rx_per_second_window.push_front(rx_rate);
    m_tx_per_second_window.push_front(tx_rate);

    if (m_rx_per_second_window.size() > window_size) {
      m_rx_per_second_window.pop_back();
    }
    if (m_tx_per_second_window.size() > window_size) {
      m_tx_per_second_window.pop_back();
    }

    m_rx = diagnostics.rx.value();
    m_tx = diagnostics.tx.value();
    m_last_update = current_time;

    if (m_rx_per_second_window.size() > 0) {
      diagnostics.rx_per_second =
          std::accumulate(m_rx_per_second_window.begin(),
                          m_rx_per_second_window.end(), 0.0) /
          m_rx_per_second_window.size();
    }
    if (m_tx_per_second_window.size() > 0) {
      diagnostics.tx_per_second =
          std::accumulate(m_tx_per_second_window.begin(),
                          m_tx_per_second_window.end(), 0.0) /
          m_tx_per_second_window.size();
    }
  }
}
