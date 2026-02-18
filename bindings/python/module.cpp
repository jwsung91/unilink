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

  // Context objects
  py::class_<MessageContext>(m, "MessageContext")
      .def("client_id", &MessageContext::client_id)
      .def("data", &MessageContext::data);

  py::class_<ConnectionContext>(m, "ConnectionContext")
      .def("client_id", &ConnectionContext::client_id)
      .def("info", &ConnectionContext::info);

  py::class_<ErrorContext>(m, "ErrorContext").def("code", &ErrorContext::code).def("message", &ErrorContext::message);

  // TcpClient
  py::class_<TcpClient, std::shared_ptr<TcpClient>>(m, "TcpClient")
      .def(py::init<const std::string&, uint16_t>())
      .def("start", [](TcpClient& self) { return self.start().get(); })
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
      .def("start", [](TcpServer& self) { return self.start().get(); })
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
      .def("start", [](Serial& self) { return self.start().get(); })
      .def("stop", &Serial::stop)
      .def("send", &Serial::send)
      .def("send_line", &Serial::send_line)
      .def("is_connected", &Serial::is_connected)
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

  // Udp
  py::class_<Udp, std::shared_ptr<Udp>>(m, "Udp")
      .def(py::init<const config::UdpConfig&>())
      .def("start", [](Udp& self) { return self.start().get(); })
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
