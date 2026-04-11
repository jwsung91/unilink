import asyncio
from typing import Optional, Union, List
import unilink_py

def _get_loop():
    try:
        return asyncio.get_running_loop()
    except RuntimeError:
        return asyncio.get_event_loop()

class AsyncChannelBase:
    """Base class for all asynchronous client transports."""
    def __init__(self, client_instance):
        self._client = client_instance
        self._loop = _get_loop()
        self._data_queue = asyncio.Queue()
        self._message_queue = asyncio.Queue()
        
        # Setup internal handlers
        self._client.on_data(self._on_data_bridge)
        self._client.on_message(self._on_message_bridge)

    def _on_data_bridge(self, ctx):
        self._loop.call_soon_threadsafe(self._data_queue.put_nowait, ctx)

    def _on_message_bridge(self, msg_bytes):
        self._loop.call_soon_threadsafe(self._message_queue.put_nowait, msg_bytes)

    async def connect(self) -> bool:
        """Starts the channel and waits for the connection to be established."""
        return await self._loop.run_in_executor(None, self._client.start)

    def stop(self):
        """Stops the channel operations."""
        self._client.stop()

    def send(self, data: Union[str, bytes]):
        """Sends data through the channel."""
        self._client.send(data)

    async def read(self) -> unilink_py.MessageContext:
        """Waits for next raw data packet."""
        return await self._data_queue.get()

    async def read_message(self) -> bytes:
        """Waits for next framed message payload."""
        return await self._message_queue.get()

    def use_line_framer(self, delimiter: str = "\n", include_delimiter: bool = False, max_length: int = 65536):
        self._client.use_line_framer(delimiter, include_delimiter, max_length)
        return self

    def use_packet_framer(self, start_pattern: List[int], end_pattern: List[int], max_length: int):
        self._client.use_packet_framer(start_pattern, end_pattern, max_length)
        return self

class AsyncTcpClient(AsyncChannelBase):
    def __init__(self, host: str, port: int):
        self._raw_client = unilink_py.TcpClient(host, port)
        super().__init__(self._raw_client)

class AsyncUdsClient(AsyncChannelBase):
    def __init__(self, socket_path: str):
        self._raw_client = unilink_py.UdsClient(socket_path)
        super().__init__(self._raw_client)

class AsyncSerial(AsyncChannelBase):
    def __init__(self, device: str, baud_rate: int):
        self._raw_client = unilink_py.Serial(device, baud_rate)
        super().__init__(self._raw_client)
    
    def set_baud_rate(self, baud: int):
        self._raw_client.set_baud_rate(baud)

class AsyncUdp(AsyncChannelBase):
    def __init__(self, config: unilink_py.UdpConfig):
        self._raw_client = unilink_py.Udp(config)
        super().__init__(self._raw_client)
