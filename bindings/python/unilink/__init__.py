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

    dll_candidates = [package_dir, package_dir.parent]
    for candidate in dll_candidates:
        if not candidate.is_dir():
            continue
        if any(candidate.glob("unilink*.dll")):
            _register_windows_dll_directory(candidate)


def _add_windows_dependency_directories() -> None:
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return

    vcpkg_installed_dir = os.environ.get("VCPKG_INSTALLED_DIR")
    vcpkg_triplet = os.environ.get("VCPKG_TARGET_TRIPLET")
    vcpkg_root = os.environ.get("VCPKG_ROOT")

    if vcpkg_installed_dir and vcpkg_triplet:
        _register_windows_dll_directory(
            Path(vcpkg_installed_dir) / vcpkg_triplet / "bin"
        )

    if vcpkg_root and vcpkg_triplet:
        _register_windows_dll_directory(
            Path(vcpkg_root) / "installed" / vcpkg_triplet / "bin"
        )


def _candidate_package_dirs():
    package_dir = Path(__file__).resolve().parent
    yield package_dir

    for entry in list(sys.path):
        candidate = Path(entry) / "unilink"
        if candidate.is_dir():
            yield candidate


_seen_package_dirs = set()

try:
    _add_windows_dependency_directories()
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

            _add_windows_dependency_directories()
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
