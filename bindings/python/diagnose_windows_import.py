import ctypes
import importlib.util
import os
from pathlib import Path
import sys
import traceback

_dll_dir_handles = []


def _print_section(title: str) -> None:
    print(f"\n== {title} ==")


def _print_path_list(label: str, value: str | None, limit: int = 12) -> None:
    print(f"{label}:")
    if not value:
        print("  <empty>")
        return

    entries = [entry for entry in value.split(";") if entry]
    for index, entry in enumerate(entries[:limit], start=1):
        print(f"  {index}. {entry}")
    if len(entries) > limit:
        print(f"  ... ({len(entries) - limit} more)")


def _list_directory(path: Path, pattern: str = "*", limit: int = 40) -> None:
    print(f"{path}:")
    if not path.exists():
        print("  <missing>")
        return
    if not path.is_dir():
        print("  <not a directory>")
        return

    matches = sorted(path.glob(pattern))
    if not matches:
        print("  <no matches>")
        return

    for item in matches[:limit]:
        suffix = "/" if item.is_dir() else ""
        print(f"  {item.name}{suffix}")
    if len(matches) > limit:
        print(f"  ... ({len(matches) - limit} more)")


def _load_binary(path: Path) -> None:
    print(f"Trying to load: {path}")
    try:
        ctypes.WinDLL(str(path))
    except OSError as exc:
        print(f"  load failed: {exc}")
    else:
        print("  load succeeded")


def _sanitize_import_path(install_path: Path) -> None:
    script_dir = Path(__file__).resolve().parent
    source_root = script_dir.parent
    sanitized = []

    for entry in sys.path:
        resolved = Path(entry or ".").resolve()
        if resolved in {script_dir, source_root}:
            continue
        sanitized.append(entry)

    sys.path[:] = sanitized

    if install_path.is_dir():
        sys.path.insert(0, str(install_path))


def main() -> int:
    if os.name != "nt":
        print("This diagnostic helper is intended for Windows only.")
        return 0

    install_path = Path(os.environ.get("UNILINK_INSTALL_PATH", ""))
    package_path = Path(os.environ.get("UNILINK_PACKAGE_PATH", ""))

    _print_section("Interpreter")
    print(f"sys.executable: {sys.executable}")
    print(f"sys.version: {sys.version}")

    _print_section("Environment")
    print(f"UNILINK_INSTALL_PATH: {install_path}")
    print(f"UNILINK_PACKAGE_PATH: {package_path}")
    _print_path_list("PYTHONPATH", os.environ.get("PYTHONPATH"))
    _print_path_list("PATH", os.environ.get("PATH"))

    _print_section("Install Contents")
    _list_directory(install_path)
    _list_directory(package_path)
    _list_directory(package_path, "*.dll")
    _list_directory(package_path, "*.pyd")

    if hasattr(os, "add_dll_directory"):
        _print_section("DLL Directories")
        if package_path.is_dir():
            _dll_dir_handles.append(os.add_dll_directory(str(package_path)))
            print(f"added: {package_path}")

    _sanitize_import_path(install_path)

    _print_section("Sanitized sys.path")
    for index, entry in enumerate(sys.path[:12], start=1):
        print(f"  {index}. {entry}")
    if len(sys.path) > 12:
        print(f"  ... ({len(sys.path) - 12} more)")

    _print_section("Import Specs")
    print(f"spec(unilink): {importlib.util.find_spec('unilink')}")
    print(f"spec(unilink.unilink_py): {importlib.util.find_spec('unilink.unilink_py')}")

    extension_candidates = sorted(package_path.glob("unilink_py*.pyd"))
    runtime_candidates = sorted(package_path.glob("unilink*.dll"))

    _print_section("Binary Load Checks")
    if not extension_candidates:
        print("No unilink_py*.pyd found in package directory.")
    for candidate in extension_candidates:
        _load_binary(candidate)

    if not runtime_candidates:
        print("No unilink*.dll found in package directory.")
    for candidate in runtime_candidates:
        _load_binary(candidate)

    _print_section("Python Import")
    try:
        import unilink
    except Exception:
        traceback.print_exc()
        return 1

    print("Successfully imported unilink")
    print(f"unilink module file: {getattr(unilink, '__file__', '<unknown>')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
