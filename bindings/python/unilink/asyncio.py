import asyncio
from typing import List, Union
import unilink_py

def _get_loop():
    try:
        return asyncio.get_running_loop()
    except RuntimeError:
        return asyncio.get_event_loop()

class AsyncChannelBase:
    """Base class for all asynchronous client transports."""
    def __init__(self, client_instance):
        self._raw_client = client_instance
        self._loop = _get_loop()
        self._data_queue = asyncio.Queue()
        self._message_queue = asyncio.Queue()
        
        # Setup internal handlers
        self._raw_client.on_data(self._on_data_bridge)
        self._raw_client.on_message(self._on_message_bridge)

    def _on_data_bridge(self, ctx):
        self._loop.call_soon_threadsafe(self._data_queue.put_nowait, ctx)

    def _on_message_bridge(self, ctx):
        self._loop.call_soon_threadsafe(self._message_queue.put_nowait, ctx)

    async def connect(self) -> bool:
        """Starts the channel and waits for the connection to be established."""
        return await self._loop.run_in_executor(None, self._raw_client.start)

    def stop(self):
        """Stops the channel operations."""
        self._raw_client.stop()

    def connected(self) -> bool:
        """Returns whether the underlying channel is currently connected."""
        return self._raw_client.connected()

    def send(self, data: Union[str, bytes]) -> bool:
        """Sends data through the channel and returns whether it was accepted."""
        return self._raw_client.send(data)

    async def read(self) -> unilink_py.MessageContext:
        """Waits for next raw data packet."""
        return await self._data_queue.get()

    async def read_message(self) -> unilink_py.MessageContext:
        """Waits for next framed message context."""
        return await self._message_queue.get()

    def use_line_framer(self, delimiter: str = "\n", include_delimiter: bool = False, max_length: int = 65536):
        self._raw_client.use_line_framer(delimiter, include_delimiter, max_length)
        return self

    def use_packet_framer(self, start_pattern: List[int], end_pattern: List[int], max_length: int):
        self._raw_client.use_packet_framer(start_pattern, end_pattern, max_length)
        return self

class AsyncTcpClient(AsyncChannelBase):
    def __init__(self, host: str, port: int):
        super().__init__(unilink_py.TcpClient(host, port))

class AsyncUdsClient(AsyncChannelBase):
    def __init__(self, socket_path: str):
        super().__init__(unilink_py.UdsClient(socket_path))

class AsyncSerial(AsyncChannelBase):
    def __init__(self, device: str, baud_rate: int):
        super().__init__(unilink_py.Serial(device, baud_rate))
    
    def baud_rate(self, baud: int):
        self._raw_client.baud_rate(baud)

class AsyncUdp(AsyncChannelBase):
    def __init__(self, config: unilink_py.UdpConfig):
        super().__init__(unilink_py.Udp(config))

class AsyncServerSession:
    """Represents a client session on an asynchronous server."""
    def __init__(self, server, client_id: int, info: str, loop):
        self._server = server
        self.id = client_id
        self.info = info
        self._loop = loop
        self._data_queue = asyncio.Queue()
        self._message_queue = asyncio.Queue()
        self._closed = asyncio.Event()

    def _push_data(self, data):
        self._loop.call_soon_threadsafe(self._data_queue.put_nowait, data)

    def _push_message(self, data):
        self._loop.call_soon_threadsafe(self._message_queue.put_nowait, data)

    def _notify_disconnect(self):
        self._loop.call_soon_threadsafe(self._closed.set)

    async def send(self, data: Union[str, bytes]) -> bool:
        return self._server.send_to(self.id, data)

    async def read(self) -> unilink_py.MessageContext:
        return await self._data_queue.get()

    async def read_message(self) -> unilink_py.MessageContext:
        return await self._message_queue.get()

    def __aiter__(self):
        return self

    async def __anext__(self):
        if self._closed.is_set() and self._message_queue.empty():
            raise StopAsyncIteration
        return await self.read_message()

class AsyncServerBase:
    """Base class for all asynchronous servers."""
    def __init__(self, server_instance):
        self._raw_server = server_instance
        self._loop = _get_loop()
        self._sessions = {}
        self._session_queue = asyncio.Queue()
        
        # Setup bridges
        self._raw_server.on_connect(self._on_connect_bridge)
        self._raw_server.on_disconnect(self._on_disconnect_bridge)
        self._raw_server.on_data(self._on_data_bridge)
        self._raw_server.on_message(self._on_message_bridge)

    def _on_connect_bridge(self, ctx):
        session = AsyncServerSession(self._raw_server, ctx.client_id, ctx.client_info, self._loop)
        self._sessions[ctx.client_id] = session
        self._loop.call_soon_threadsafe(self._session_queue.put_nowait, session)

    def _on_disconnect_bridge(self, ctx):
        session = self._sessions.pop(ctx.client_id, None)
        if session:
            session._notify_disconnect()

    def _on_data_bridge(self, ctx):
        session = self._sessions.get(ctx.client_id)
        if session:
            session._push_data(ctx)

    def _on_message_bridge(self, ctx):
        session = self._sessions.get(ctx.client_id)
        if session:
            session._push_message(ctx)

    async def start(self) -> bool:
        return await self._loop.run_in_executor(None, self._raw_server.start)

    def stop(self):
        self._raw_server.stop()

    def listening(self) -> bool:
        return self._raw_server.listening()

    def broadcast(self, data: Union[str, bytes]) -> bool:
        return self._raw_server.broadcast(data)

    def use_line_framer(self, delimiter: str = "\n", include_delimiter: bool = False, max_length: int = 65536):
        self._raw_server.use_line_framer(delimiter, include_delimiter, max_length)
        return self

    def use_packet_framer(self, start_pattern: List[int], end_pattern: List[int], max_length: int):
        self._raw_server.use_packet_framer(start_pattern, end_pattern, max_length)
        return self

    def __aiter__(self):
        return self

    async def __anext__(self) -> AsyncServerSession:
        return await self._session_queue.get()

class AsyncTcpServer(AsyncServerBase):
    def __init__(self, port: int):
        super().__init__(unilink_py.TcpServer(port))

class AsyncUdsServer(AsyncServerBase):
    def __init__(self, socket_path: str):
        super().__init__(unilink_py.UdsServer(socket_path))

class AsyncUdpServer(AsyncServerBase):
    def __init__(self, port_or_config: Union[int, unilink_py.UdpConfig]):
        if isinstance(port_or_config, int):
            super().__init__(unilink_py.UdpServer(port_or_config))
        else:
            super().__init__(unilink_py.UdpServer(port_or_config))
