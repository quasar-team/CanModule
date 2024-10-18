#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

#include "CanDevice.h"
#include "CanDeviceArguments.h"
#include "CanFrame.h"
#include "CanLogIt.h"

namespace py = pybind11;

PYBIND11_MODULE(canmodule, m) {
  py::class_<CanFrame>(m, "CanFrame")
      .def(py::init<const uint32_t, const uint32_t, const uint32_t>(),
           py::arg("id"), py::arg("requested_length"), py::arg("flags"))
      .def(py::init<const uint32_t, const uint32_t>(), py::arg("id"),
           py::arg("requested_length"))
      .def(py::init<const uint32_t>(), py::arg("id"))
      .def(py::init<const uint32_t, const std::vector<char>&>(), py::arg("id"),
           py::arg("message"))
      .def(py::init<const uint32_t, const std::vector<char>&, const uint32_t>(),
           py::arg("id"), py::arg("message"), py::arg("flags"))
      .def("id", &CanFrame::id)
      .def("message", &CanFrame::message)
      .def("flags", &CanFrame::flags)
      .def("is_error", &CanFrame::is_error)
      .def("is_remote_request", &CanFrame::is_remote_request)
      .def("is_standard_id", &CanFrame::is_standard_id)
      .def("is_extended_id", &CanFrame::is_extended_id)
      .def("length", &CanFrame::length)
      .def("__str__", &CanFrame::to_string);

  py::module_ can_flags =
      m.def_submodule("can_flags", "Namespace for CAN frame flags");
  can_flags.attr("standard_id") = can_flags::standard_id;
  can_flags.attr("extended_id") = can_flags::extended_id;
  can_flags.attr("error_frame") = can_flags::error_frame;
  can_flags.attr("remote_request") = can_flags::remote_request;

  py::enum_<CanReturnCode>(m, "CanReturnCode")
      .value("success", CanReturnCode::success)
      .value("unknown_open_error", CanReturnCode::unknown_open_error)
      .value("socket_error", CanReturnCode::socket_error)
      .value("too_many_connections", CanReturnCode::too_many_connections)
      .value("timeout", CanReturnCode::timeout)
      .value("disconnected", CanReturnCode::disconnected)
      .value("internal_api_error", CanReturnCode::internal_api_error)
      .value("unknown_send_error", CanReturnCode::unknown_send_error)
      .value("not_ack", CanReturnCode::not_ack)
      .value("tx_error", CanReturnCode::tx_error)
      .value("rx_error", CanReturnCode::rx_error)
      .value("tx_buffer_overflow", CanReturnCode::tx_buffer_overflow)
      .value("lost_arbitration", CanReturnCode::lost_arbitration)
      .value("invalid_bitrate", CanReturnCode::invalid_bitrate)
      .value("unknown_close_error", CanReturnCode::unknown_close_error)
      .export_values();

  py::class_<CanDevice>(m, "CanDevice")
      .def("open", &CanDevice::open)
      .def("close", &CanDevice::close)
      .def("diagnostics", &CanDevice::diagnostics)
      .def("send", py::overload_cast<const CanFrame&>(&CanDevice::send))
      .def("send",
           py::overload_cast<const std::vector<CanFrame>&>(&CanDevice::send))
      .def("args", &CanDevice::args)
      .def_static("create", &CanDevice::create);

  py::class_<CanDeviceArguments>(m, "CanDeviceArguments")
      .def(py::init<const CanDeviceConfiguration&,
                    const std::function<void(const CanFrame&)>,
                    const std::function<void(const CanReturnCode,
                                             std::string_view)>&>(),
           py::arg("config"),
           py::arg("receiver") = std::function<void(const CanFrame&)>(),
           py::arg("on_error") =
               std::function<void(const CanReturnCode, std::string_view)>())
      .def_readonly("config", &CanDeviceArguments::config);

  py::class_<CanDeviceConfiguration>(m, "CanDeviceConfiguration")
      .def(py::init<>())
      .def_readwrite("bus_name", &CanDeviceConfiguration::bus_name)
      .def_readwrite("bus_number", &CanDeviceConfiguration::bus_number)
      .def_readwrite("host", &CanDeviceConfiguration::host)
      .def_readwrite("bitrate", &CanDeviceConfiguration::bitrate)
      .def_readwrite("enable_termination",
                     &CanDeviceConfiguration::enable_termination)
      .def_readwrite("vcan", &CanDeviceConfiguration::vcan)
      .def_readwrite("timeout", &CanDeviceConfiguration::timeout)
      .def_readwrite("sent_acknowledgement",
                     &CanDeviceConfiguration::sent_acknowledgement)
      .def("__str__", &CanDeviceConfiguration::to_string);

  py::class_<CanDiagnostics>(m, "CanDiagnostics")
      .def(py::init<>())
      .def_readonly("log_entries", &CanDiagnostics::log_entries)
      .def_readonly("name", &CanDiagnostics::name)
      .def_readonly("handle", &CanDiagnostics::handle)
      .def_readonly("mode", &CanDiagnostics::mode)
      .def_readonly("state", &CanDiagnostics::state)
      .def_readonly("bitrate", &CanDiagnostics::bitrate)
      .def_readonly("connected_clients_addresses",
                    &CanDiagnostics::connected_clients_addresses)
      .def_readonly("connected_clients_timestamps",
                    &CanDiagnostics::connected_clients_timestamps)
      .def_readonly("connected_clients_ports",
                    &CanDiagnostics::connected_clients_ports)
      .def_readonly("number_connected_clients",
                    &CanDiagnostics::number_connected_clients)
      .def_readonly("temperature", &CanDiagnostics::temperature)
      .def_readonly("uptime", &CanDiagnostics::uptime)
      .def_readonly("tcp_rx", &CanDiagnostics::tcp_rx)
      .def_readonly("tcp_tx", &CanDiagnostics::tcp_tx)
      .def_readonly("rx", &CanDiagnostics::rx)
      .def_readonly("tx", &CanDiagnostics::tx)
      .def_readonly("rx_error", &CanDiagnostics::rx_error)
      .def_readonly("tx_error", &CanDiagnostics::tx_error)
      .def_readonly("rx_drop", &CanDiagnostics::rx_drop)
      .def_readonly("tx_drop", &CanDiagnostics::tx_drop)
      .def_readonly("tx_timeout", &CanDiagnostics::tx_timeout)
      .def_readonly("bus_error", &CanDiagnostics::bus_error)
      .def_readonly("error_warning", &CanDiagnostics::error_warning)
      .def_readonly("error_passive", &CanDiagnostics::error_passive)
      .def_readonly("bus_off", &CanDiagnostics::bus_off)
      .def_readonly("arbitration_lost", &CanDiagnostics::arbitration_lost)
      .def_readonly("restarts", &CanDiagnostics::restarts)
      .def("__str__", &CanDiagnostics::to_string);
}
