#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "unilink/unilink.hpp"

namespace py = pybind11;
using namespace unilink;
using namespace unilink::wrapper;

PYBIND11_MODULE(unilink_py, m) {
  m.doc() = "unilink python bindings";

  // ErrorCode enum (Matching actual definitions in error_codes.hpp)
  py::enum_<ErrorCode>(m, "ErrorCode")
      .value("Success", ErrorCode::Success)
      .value("Unknown", ErrorCode::Unknown)
      .value("InvalidConfiguration", ErrorCode::InvalidConfiguration)
      .value("InternalError", ErrorCode::InternalError)
      .value("IoError", ErrorCode::IoError)
      .value("ConnectionRefused", ErrorCode::ConnectionRefused)
      .value("ConnectionReset", ErrorCode::ConnectionReset)
      .value("ConnectionAborted", ErrorCode::ConnectionAborted)
      .value("TimedOut", ErrorCode::TimedOut)
      .value("NotConnected", ErrorCode::NotConnected)
      .value("AlreadyConnected", ErrorCode::AlreadyConnected)
      .value("PortInUse", ErrorCode::PortInUse)
      .value("AccessDenied", ErrorCode::AccessDenied)
      .value("Stopped", ErrorCode::Stopped)
      .value("StartFailed", ErrorCode::StartFailed)
      .export_values();

  // Context objects
  py::class_<MessageContext>(m, "MessageContext")
      .def_property_readonly("client_id", &MessageContext::client_id)
      .def_property_readonly("data", [](const MessageContext& self) { return std::string(self.data()); })
      .def_property_readonly("client_info", &MessageContext::client_info);

  py::class_<ConnectionContext>(m, "ConnectionContext")
      .def_property_readonly("client_id", &ConnectionContext::client_id)
      .def_property_readonly("client_info", &ConnectionContext::client_info);

  py::class_<ErrorContext>(m, "ErrorContext")
      .def_property_readonly("code", &ErrorContext::code)
      .def_property_readonly("message", [](const ErrorContext& self) { return std::string(self.message()); })
      .def_property_readonly("client_id", &ErrorContext::client_id);

  // TcpClient
  py::class_<TcpClient, std::shared_ptr<TcpClient>>(m, "TcpClient")
      .def(py::init<const std::string&, uint16_t>())
      .def("start",
           [](TcpClient& self) {
             py::gil_scoped_release release;
             return self.start().get();
           })
      .def("stop", &TcpClient::stop)
      .def("send", &TcpClient::send)
      .def("send_line", &TcpClient::send_line)
      .def("is_connected", &TcpClient::is_connected)
      .def("auto_manage", &TcpClient::auto_manage, py::arg("manage") = true)
      .def("on_data",
           [](TcpClient& self, std::function<void(const MessageContext&)> handler) {
             self.on_data([handler](const MessageContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_connect",
           [](TcpClient& self, std::function<void(const ConnectionContext&)> handler) {
             self.on_connect([handler](const ConnectionContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_disconnect",
           [](TcpClient& self, std::function<void(const ConnectionContext&)> handler) {
             self.on_disconnect([handler](const ConnectionContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_error", [](TcpClient& self, std::function<void(const ErrorContext&)> handler) {
        self.on_error([handler](const ErrorContext& ctx) {
          py::gil_scoped_acquire gil;
          handler(ctx);
        });
        return &self;
      });

  // TcpServer
  py::class_<TcpServer, std::shared_ptr<TcpServer>>(m, "TcpServer")
      .def(py::init<uint16_t>())
      .def("start",
           [](TcpServer& self) {
             py::gil_scoped_release release;
             return self.start().get();
           })
      .def("stop", &TcpServer::stop)
      .def("broadcast", &TcpServer::broadcast)
      .def("send_to", &TcpServer::send_to)
      .def("get_client_count", &TcpServer::get_client_count)
      .def("on_data",
           [](TcpServer& self, std::function<void(const MessageContext&)> handler) {
             self.on_data([handler](const MessageContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_client_connect",
           [](TcpServer& self, std::function<void(const ConnectionContext&)> handler) {
             self.on_client_connect([handler](const ConnectionContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_client_disconnect",
           [](TcpServer& self, std::function<void(const ConnectionContext&)> handler) {
             self.on_client_disconnect([handler](const ConnectionContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_error", [](TcpServer& self, std::function<void(const ErrorContext&)> handler) {
        self.on_error([handler](const ErrorContext& ctx) {
          py::gil_scoped_acquire gil;
          handler(ctx);
        });
        return &self;
      });

  // Serial
  py::class_<Serial, std::shared_ptr<Serial>>(m, "Serial")
      .def(py::init<const std::string&, uint32_t>())
      .def("start",
           [](Serial& self) {
             py::gil_scoped_release release;
             return self.start().get();
           })
      .def("stop", &Serial::stop)
      .def("send", &Serial::send)
      .def("send_line", &Serial::send_line)
      .def("is_connected", &Serial::is_connected)
      .def("set_baud_rate", &Serial::set_baud_rate)
      .def("set_data_bits", &Serial::set_data_bits)
      .def("set_stop_bits", &Serial::set_stop_bits)
      .def("set_parity", &Serial::set_parity)
      .def("set_flow_control", &Serial::set_flow_control)
      .def("on_data",
           [](Serial& self, std::function<void(const MessageContext&)> handler) {
             self.on_data([handler](const MessageContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_connect",
           [](Serial& self, std::function<void(const ConnectionContext&)> handler) {
             self.on_connect([handler](const ConnectionContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_disconnect",
           [](Serial& self, std::function<void(const ConnectionContext&)> handler) {
             self.on_disconnect([handler](const ConnectionContext& ctx) {
               py::gil_scoped_acquire gil;
               handler(ctx);
             });
             return &self;
           })
      .def("on_error", [](Serial& self, std::function<void(const ErrorContext&)> handler) {
        self.on_error([handler](const ErrorContext& ctx) {
          py::gil_scoped_acquire gil;
          handler(ctx);
        });
        return &self;
      });

  // UdpConfig
  py::class_<config::UdpConfig>(m, "UdpConfig")
      .def(py::init<>())
      .def_readwrite("local_address", &config::UdpConfig::local_address)
      .def_readwrite("local_port", &config::UdpConfig::local_port)
      .def_readwrite("remote_address", &config::UdpConfig::remote_address)
      .def_readwrite("remote_port", &config::UdpConfig::remote_port)
      .def_readwrite("backpressure_threshold", &config::UdpConfig::backpressure_threshold)
      .def_readwrite("enable_memory_pool", &config::UdpConfig::enable_memory_pool)
      .def_readwrite("stop_on_callback_exception", &config::UdpConfig::stop_on_callback_exception);

  // Udp
  py::class_<Udp, std::shared_ptr<Udp>>(m, "Udp")
      .def(py::init<const config::UdpConfig&>())
      .def("start",
           [](Udp& self) {
             py::gil_scoped_release release;
             return self.start().get();
           })
      .def("stop", &Udp::stop)
      .def("send", &Udp::send)
      .def("send_line", &Udp::send_line)
      .def("is_connected", &Udp::is_connected)
      .def("on_data", [](Udp& self, std::function<void(const MessageContext&)> handler) {
        self.on_data([handler](const MessageContext& ctx) {
          py::gil_scoped_acquire gil;
          handler(ctx);
        });
        return &self;
      });
}
