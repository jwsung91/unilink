try:
    from .unilink_py import *
except ImportError:
    # Fallback for when running from build directory or direct import
    try:
        from unilink_py import *
    except ImportError as exc:
        raise ImportError(
            "Failed to import the `unilink_py` extension module. "
            "Build and install the Python bindings first, or ensure the built module is on PYTHONPATH."
        ) from exc
