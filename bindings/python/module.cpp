#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>

#include "unilink/unilink.hpp"

namespace py = pybind11;
using namespace unilink;
using namespace unilink::wrapper;

PYBIND11_MODULE(unilink_py, m) {
    m.doc() = "unilink python bindings";

    // TcpClient
    py::class_<TcpClient, std::shared_ptr<TcpClient>>(m, "TcpClient")
        .def(py::init<const std::string&, uint16_t>())
        .def("start", &TcpClient::start)
        .def("stop", &TcpClient::stop)
        .def("send", &TcpClient::send)
        .def("send_line", &TcpClient::send_line)
        .def("is_connected", &TcpClient::is_connected)
        .def("auto_manage", &TcpClient::auto_manage, py::arg("manage") = true)
        .def("on_data", &TcpClient::on_data)
        .def("on_bytes", [](TcpClient& self, std::function<void(const py::bytes&)> handler) {
             self.on_bytes([handler](const uint8_t* data, size_t size) {
                 py::gil_scoped_acquire gil;
                 handler(py::bytes((const char*)data, size));
             });
             return &self;
        })
        .def("on_connect", &TcpClient::on_connect)
        .def("on_disconnect", &TcpClient::on_disconnect)
        .def("on_error", &TcpClient::on_error);

    // TcpServer
    py::class_<TcpServer, std::shared_ptr<TcpServer>>(m, "TcpServer")
        .def(py::init<uint16_t>())
        .def("start", &TcpServer::start)
        .def("stop", &TcpServer::stop)
        .def("send", &TcpServer::send)
        .def("send_line", &TcpServer::send_line)
        .def("is_connected", &TcpServer::is_connected)
        .def("broadcast", &TcpServer::broadcast)
        .def("send_to_client", &TcpServer::send_to_client)
        .def("get_client_count", &TcpServer::get_client_count)
        .def("on_data", &TcpServer::on_data)
        .def("on_bytes", [](TcpServer& self, std::function<void(const py::bytes&)> handler) {
             self.on_bytes([handler](const uint8_t* data, size_t size) {
                 py::gil_scoped_acquire gil;
                 handler(py::bytes((const char*)data, size));
             });
             return &self;
        })
        .def("on_connect", &TcpServer::on_connect)
        .def("on_disconnect", &TcpServer::on_disconnect)
        .def("on_error", &TcpServer::on_error)
        .def("on_multi_connect", &TcpServer::on_multi_connect)
        .def("on_multi_data", &TcpServer::on_multi_data)
        .def("on_multi_disconnect", &TcpServer::on_multi_disconnect);

    // Serial
    py::class_<Serial, std::shared_ptr<Serial>>(m, "Serial")
        .def(py::init<const std::string&, uint32_t>())
        .def("start", &Serial::start)
        .def("stop", &Serial::stop)
        .def("send", &Serial::send)
        .def("send_line", &Serial::send_line)
        .def("is_connected", &Serial::is_connected)
        .def("set_baud_rate", &Serial::set_baud_rate)
        .def("set_data_bits", &Serial::set_data_bits)
        .def("set_stop_bits", &Serial::set_stop_bits)
        .def("set_parity", &Serial::set_parity)
        .def("set_flow_control", &Serial::set_flow_control)
        .def("on_data", &Serial::on_data)
        .def("on_bytes", [](Serial& self, std::function<void(const py::bytes&)> handler) {
             self.on_bytes([handler](const uint8_t* data, size_t size) {
                 py::gil_scoped_acquire gil;
                 handler(py::bytes((const char*)data, size));
             });
             return &self;
        })
        .def("on_connect", &Serial::on_connect)
        .def("on_disconnect", &Serial::on_disconnect)
        .def("on_error", &Serial::on_error);

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
        .def("start", &Udp::start)
        .def("stop", &Udp::stop)
        .def("send", &Udp::send)
        .def("send_line", &Udp::send_line)
        .def("is_connected", &Udp::is_connected)
        .def("on_data", &Udp::on_data)
        .def("on_bytes", [](Udp& self, std::function<void(const py::bytes&)> handler) {
             self.on_bytes([handler](const uint8_t* data, size_t size) {
                 py::gil_scoped_acquire gil;
                 handler(py::bytes((const char*)data, size));
             });
             return &self;
        })
        .def("on_connect", &Udp::on_connect)
        .def("on_disconnect", &Udp::on_disconnect)
        .def("on_error", &Udp::on_error);
}
