from pathlib import Path
import sys

try:
    from .unilink_py import *
except ImportError:
    # Fallback for when running from build directory or direct import
    try:
        from unilink_py import *
    except ImportError as exc:
        for entry in list(sys.path):
            package_dir = Path(entry) / "unilink"
            if not package_dir.is_dir():
                continue
            if not any(package_dir.glob("unilink_py.*")):
                continue

            sys.path.insert(0, str(package_dir))
            try:
                from unilink_py import *
                break
            except ImportError:
                sys.path.pop(0)
        else:
            raise ImportError(
                "Failed to import the `unilink_py` extension module. "
                "Build and install the Python bindings first, or ensure the built module is on PYTHONPATH."
            ) from exc
