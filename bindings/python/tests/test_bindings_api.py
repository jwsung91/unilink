import asyncio
import socket
import threading
import time
from types import SimpleNamespace

import pytest

import unilink
import unilink.asyncio as unilink_asyncio


def wait_until(predicate, timeout=5.0, interval=0.01):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(interval)
    return False


def reserve_tcp_port():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.bind(("127.0.0.1", 0))
            return sock.getsockname()[1]
    except PermissionError as exc:
        pytest.skip(f"socket creation is blocked in this environment: {exc}")


def test_standard_python_surface_uses_canonical_names():
    assert hasattr(unilink.TcpClient, "connected")
    assert not hasattr(unilink.TcpClient, "is_connected")
    assert hasattr(unilink.TcpClient, "use_line_framer")
    assert not hasattr(unilink.TcpClient, "line_framer")
    assert hasattr(unilink.TcpClient, "use_packet_framer")
    assert not hasattr(unilink.TcpClient, "packet_framer")

    assert hasattr(unilink.TcpServer, "listening")
    assert not hasattr(unilink.TcpServer, "is_listening")
    assert hasattr(unilink.TcpServer, "on_connect")
    assert not hasattr(unilink.TcpServer, "on_client_connect")
    assert hasattr(unilink.TcpServer, "on_disconnect")
    assert not hasattr(unilink.TcpServer, "on_client_disconnect")

    assert hasattr(unilink.MessageContext, "client_info")
    assert not hasattr(unilink.MessageContext, "remote_address")


def test_tcp_loopback_supports_standard_callbacks_and_framer():
    port = reserve_tcp_port()
    server_connected = threading.Event()
    server_message = threading.Event()
    received = []

    server = unilink.TcpServer(port)
    client = unilink.TcpClient("127.0.0.1", port)

    def on_connect(_ctx):
        server_connected.set()

    def on_message(ctx):
        received.append(ctx.data)
        server_message.set()

    try:
        server.use_line_framer("\n")
        server.on_connect(on_connect)
        server.on_message(on_message)

        assert server.start() is True
        assert wait_until(server.listening)

        assert client.start() is True
        assert wait_until(lambda: server_connected.is_set() and client.connected())

        assert client.send_line("hello")
        assert wait_until(server_message.is_set)
        assert received == [b"hello"]
    finally:
        client.stop()
        server.stop()


class FakeClient:
    def __init__(self):
        self._connected = True
        self._on_data = None
        self._on_message = None
        self.line_framer_args = None
        self.packet_framer_args = None

    def on_data(self, handler):
        self._on_data = handler

    def on_message(self, handler):
        self._on_message = handler

    def start(self):
        return True

    def stop(self):
        return None

    def connected(self):
        return self._connected

    def send(self, _data):
        return True

    def use_line_framer(self, *args):
        self.line_framer_args = args

    def use_packet_framer(self, *args):
        self.packet_framer_args = args


def test_asyncio_wrapper_uses_start_not_connect():
    async def run():
        wrapper = unilink_asyncio.AsyncChannelBase(FakeClient())
        assert hasattr(wrapper, "start")
        assert not hasattr(wrapper, "connect")
        assert wrapper.connected() is True
        assert wrapper.send(b"payload") is True

        wrapper.use_line_framer("\n", False, 1024)
        wrapper._raw_client._on_data(SimpleNamespace(client_id=0, client_info="", data=b"raw"))
        raw_ctx = await asyncio.wait_for(wrapper.read(), timeout=1.0)
        assert raw_ctx.data == b"raw"

        wrapper._raw_client._on_message(SimpleNamespace(client_id=0, client_info="", data=b"framed"))
        msg_ctx = await asyncio.wait_for(wrapper.read_message(), timeout=1.0)
        assert msg_ctx.data == b"framed"

    asyncio.run(run())
