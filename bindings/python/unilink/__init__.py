import os
from pathlib import Path
import sys

_dll_dir_handles = []
_registered_dll_dirs = set()


def _register_windows_dll_directory(directory: Path) -> None:
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return
    
    try:
        resolved_dir = str(directory.resolve())
        if not os.path.isdir(resolved_dir):
            return
            
        if resolved_dir in _registered_dll_dirs:
            return

        # Keep handle to ensure the directory remains in the search path
        handle = os.add_dll_directory(resolved_dir)
        _dll_dir_handles.append(handle)
        _registered_dll_dirs.add(resolved_dir)
    except Exception:
        # Best effort - skip invalid paths or permission issues
        pass


def _add_windows_dll_directory(package_dir: Path) -> None:
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return

    # 1. Add the package directory itself (contains unilink.dll and unilink_py.pyd)
    if package_dir.is_dir():
        _register_windows_dll_directory(package_dir)

    # 2. Add directories from PATH as fallback. Python 3.8+ ignores PATH for DLL 
    # loading, but many users and CI environments rely on it. Scoping to 
    # directories that actually exist to minimize overhead.
    for path_str in os.environ.get("PATH", "").split(os.pathsep):
        if not path_str:
            continue
        path = Path(path_str)
        # Only add if it looks like it might contain dependencies (e.g., contains 'vcpkg' or 'bin')
        # or if it's the directory we're currently looking at.
        path_lower = path_str.lower()
        if "vcpkg" in path_lower or "bin" in path_lower or "unilink" in path_lower:
            if path.is_dir():
                _register_windows_dll_directory(path)


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
