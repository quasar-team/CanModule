#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "CanDevice.h"
#include "CanDeviceArguments.h"
#include "CanFrame.h"

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
      .def("length", &CanFrame::length);

  py::module_ can_flags =
      m.def_submodule("CanFlags", "Namespace for CAN frame flags");
  can_flags.attr("STANDARD_ID") = CanFlags::STANDARD_ID;
  can_flags.attr("EXTENDED_ID") = CanFlags::EXTENDED_ID;
  can_flags.attr("ERROR_FRAME") = CanFlags::ERROR_FRAME;
  can_flags.attr("REMOTE_REQUEST") = CanFlags::REMOTE_REQUEST;

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
                    const std::function<void(const CanFrame&)>&>(),
           py::arg("config"), py::arg("receiver"))
      .def_readwrite("config", &CanDeviceArguments::config)
      .def("set_receiver",
           [](CanDeviceArguments& self,
              const std::function<void(const CanFrame&)>& recv) {
             const_cast<std::function<void(const CanFrame&)>&>(self.receiver) =
                 recv;
           });

  py::class_<CanDeviceConfiguration>(m, "CanDeviceConfiguration")
      .def(py::init<>())
      .def_readwrite("bus_name", &CanDeviceConfiguration::bus_name)
      .def_readwrite("bus_address", &CanDeviceConfiguration::bus_address)
      .def_readwrite("host", &CanDeviceConfiguration::host)
      .def_readwrite("bitrate", &CanDeviceConfiguration::bitrate)
      .def_readwrite("enable_termination",
                     &CanDeviceConfiguration::enable_termination)
      .def_readwrite("timeout", &CanDeviceConfiguration::timeout)
      .def_readwrite("sent_acknowledgement",
                     &CanDeviceConfiguration::sent_acknowledgement);

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
      .def_readonly("restarts", &CanDiagnostics::restarts);
}
