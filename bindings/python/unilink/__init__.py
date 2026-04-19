import os
from pathlib import Path
import sys

_dll_dir_handles = []
_registered_dll_dirs = set()


def _register_windows_dll_directory(directory: Path) -> None:
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return
    if not directory.is_dir():
        return

    resolved_dir = str(directory.resolve())
    if resolved_dir in _registered_dll_dirs:
        return

    _dll_dir_handles.append(os.add_dll_directory(resolved_dir))
    _registered_dll_dirs.add(resolved_dir)


def _add_windows_dll_directory(package_dir: Path) -> None:
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return

    if not package_dir.is_dir():
        return

    if any(package_dir.glob("unilink*.dll")) or any(package_dir.glob("unilink_py*.pyd")):
        _register_windows_dll_directory(package_dir)


def _candidate_package_dirs():
    package_dir = Path(__file__).resolve().parent
    yield package_dir

    for entry in list(sys.path):
        candidate = Path(entry) / "unilink"
        if candidate.is_dir():
            yield candidate


_seen_package_dirs = set()

try:
    _add_windows_dll_directory(Path(__file__).resolve().parent)
    from .unilink_py import *
except ImportError:
    # Fallback for when running from build directory or direct import
    try:
        from unilink_py import *
    except ImportError as exc:
        for package_dir in _candidate_package_dirs():
            resolved_dir = package_dir.resolve()
            if resolved_dir in _seen_package_dirs:
                continue
            _seen_package_dirs.add(resolved_dir)

            if not any(package_dir.glob("unilink_py.*")):
                continue

            _add_windows_dll_directory(package_dir)
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
