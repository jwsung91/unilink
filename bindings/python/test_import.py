import sys


try:
    import unilink
    print("Successfully imported unilink")
except ImportError as exc:
    print(f"Failed to import unilink: {exc}")
    sys.exit(1)


def test_python_surface():
    print("Testing Python surface...")
    assert hasattr(unilink.TcpClient, "connected")
    assert hasattr(unilink.TcpClient, "use_line_framer")
    assert hasattr(unilink.TcpServer, "on_connect")
    assert hasattr(unilink.TcpServer, "on_disconnect")
    assert hasattr(unilink.MessageContext, "client_info")
    print("Python surface initialized.")


if __name__ == "__main__":
    test_python_surface()
    print("All tests passed!")
