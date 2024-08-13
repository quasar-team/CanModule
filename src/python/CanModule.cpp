#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "CanDevice.h"
#include "CanDeviceConfig.h"
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
      .def("is_valid", &CanFrame::is_valid)
      .def("id", &CanFrame::id)
      .def("message", &CanFrame::message)
      .def("flags", &CanFrame::flags)
      .def("is_error_frame", &CanFrame::is_error_frame)
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
      .def("send", py::overload_cast<const CanFrame&>(&CanDevice::send))
      .def("send",
           py::overload_cast<const std::vector<CanFrame>&>(&CanDevice::send))
      .def("vendor_name", &CanDevice::vendor_name)
      .def("configuration", &CanDevice::configuration)
      .def_static("create", &CanDevice::create);

  py::class_<CanDeviceConfig>(m, "CanDeviceConfig")
      .def(py::init<const std::string&,
                    const std::function<void(const CanFrame&)>&>())
      .def_readonly("vendor_config", &CanDeviceConfig::vendor_config)
      .def("set_receiver",
           [](CanDeviceConfig& self,
              const std::function<void(const CanFrame&)>& recv) {
             const_cast<std::function<void(const CanFrame&)>&>(self.receiver) =
                 recv;
           });
}
