include(GNUInstallDirs)

if(DEFINED Python3_EXECUTABLE AND NOT DEFINED Python3_ROOT_DIR)
  get_filename_component(
    _unilink_python_executable_dir "${Python3_EXECUTABLE}" DIRECTORY
  )
  get_filename_component(
    _unilink_python_executable_dir_name "${_unilink_python_executable_dir}"
    NAME
  )
  if(_unilink_python_executable_dir_name MATCHES "^(bin|Scripts)$")
    get_filename_component(
      Python3_ROOT_DIR "${_unilink_python_executable_dir}" DIRECTORY
    )
  else()
    set(Python3_ROOT_DIR "${_unilink_python_executable_dir}")
  endif()
endif()

# On Windows CI, vcpkg also provides a Python distribution for pybind11's
# build-time dependency graph. Lock FindPython to the explicitly selected
# interpreter and a non-debug ABI so the extension does not link to
# python312_d.dll in Release jobs.
set(Python3_FIND_STRATEGY LOCATION)
set(PYBIND11_FINDPYTHON ON)
if(WIN32)
  set(Python3_FIND_REGISTRY NEVER)
  set(Python3_FIND_ABI OFF ANY ANY)
endif()

if((WIN32 OR APPLE) AND Python3_EXECUTABLE)
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}" -c
      "import pathlib, sys, sysconfig; exe_dir = pathlib.Path(sys.executable).resolve().parent; include = sysconfig.get_path('include') or str(exe_dir / 'Include'); libname = sysconfig.get_config_var('LDLIBRARY') or sysconfig.get_config_var('LIBRARY') or f'python{sys.version_info.major}{sys.version_info.minor}.lib'; candidates = [sysconfig.get_config_var('LIBDIR'), sysconfig.get_config_var('LIBPL'), str(pathlib.Path(sys.base_prefix) / 'libs'), str(pathlib.Path(sys.prefix) / 'libs'), str(exe_dir / 'libs')]; libs = [str(pathlib.Path(p) / libname) for p in candidates if p]; existing = next((p for p in libs if pathlib.Path(p).exists()), libs[0] if libs else ''); print(include); print(existing)"
    OUTPUT_VARIABLE _unilink_python_probe
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _unilink_python_probe_result
  )

  if(_unilink_python_probe_result EQUAL 0)
    string(REPLACE "\r\n" "\n" _unilink_python_probe "${_unilink_python_probe}")
    string(REPLACE "\n" ";" _unilink_python_probe_lines
                   "${_unilink_python_probe}"
    )
    list(LENGTH _unilink_python_probe_lines _unilink_python_probe_count)

    if(_unilink_python_probe_count GREATER_EQUAL 1)
      list(GET _unilink_python_probe_lines 0 _unilink_python_include_dir)

      if(EXISTS "${_unilink_python_include_dir}/Python.h")
        set(Python3_INCLUDE_DIR
            "${_unilink_python_include_dir}"
            CACHE PATH
                  "Python include directory selected by Python3_EXECUTABLE"
                  FORCE
        )
      endif()

      if(_unilink_python_probe_count GREATER_EQUAL 2)
        list(GET _unilink_python_probe_lines 1 _unilink_python_library)
        get_filename_component(
          _unilink_python_library_dir "${_unilink_python_library}" DIRECTORY
        )
      endif()

      if(WIN32 AND EXISTS "${_unilink_python_library}")
        set(Python3_LIBRARY
            "${_unilink_python_library}"
            CACHE FILEPATH
                  "Python import library selected by Python3_EXECUTABLE" FORCE
        )
        set(Python3_LIBRARY_RELEASE
            "${_unilink_python_library}"
            CACHE FILEPATH
                  "Python release import library selected by Python3_EXECUTABLE"
                  FORCE
        )
        set(_Python3_LIBRARY_RELEASE
            "${_unilink_python_library}"
            CACHE INTERNAL
                  "Python release import library selected by Python3_EXECUTABLE"
                  FORCE
        )
        set(_Python3_LIBRARY_DEBUG
            "Python3_LIBRARY_DEBUG-NOTFOUND"
            CACHE INTERNAL "Disable mismatched debug Python import library"
                  FORCE
        )
      endif()
    endif()
  endif()
endif()

find_package(
  Python3
  COMPONENTS Interpreter Development.Module
  REQUIRED
)

if(WIN32
   AND _unilink_python_library
   AND EXISTS "${_unilink_python_library}"
)
  set(Python3_LIBRARIES "${_unilink_python_library}")
  set(Python3_LIBRARY_DIRS "${_unilink_python_library_dir}")
endif()

find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c
            "import pybind11; print(pybind11.get_include())"
    OUTPUT_VARIABLE _unilink_pybind11_include_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _unilink_pybind11_include_result
  )

  if(NOT _unilink_pybind11_include_result EQUAL 0
     OR NOT _unilink_pybind11_include_dir
  )
    message(
      FATAL_ERROR
        "pybind11 was not found via CMake package config, and the selected Python "
        "interpreter could not provide a pybind11 include directory."
    )
  endif()

  add_library(pybind11::headers INTERFACE IMPORTED)
  set_target_properties(
    pybind11::headers PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                 "${_unilink_pybind11_include_dir}"
  )
  message(
    STATUS
      "[unilink-python] pybind11 include : ${_unilink_pybind11_include_dir}"
  )
endif()

message(STATUS "[unilink-python] interpreter      : ${Python3_EXECUTABLE}")
message(STATUS "[unilink-python] root dir         : ${Python3_ROOT_DIR}")
message(STATUS "[unilink-python] include dirs     : ${Python3_INCLUDE_DIRS}")
message(STATUS "[unilink-python] libraries        : ${Python3_LIBRARIES}")
message(STATUS "[unilink-python] library dirs     : ${Python3_LIBRARY_DIRS}")

# Inject version from CMake
set(UNILINK_VERSION ${PROJECT_VERSION})
configure_file(module.cpp.in module.cpp @ONLY)

python3_add_library(
  unilink_py MODULE WITH_SOABI ${CMAKE_CURRENT_BINARY_DIR}/module.cpp
)

# CMake 4.x can inherit Python debug postfix metadata on Windows and emit
# `unilink_py_d.cp312-win_amd64.pyd` even for Release jobs. Keep the importable
# module basename stable across configurations so CI and packaging always look
# for `unilink_py`.
set_target_properties(
  unilink_py
  PROPERTIES OUTPUT_NAME "unilink_py"
             DEBUG_POSTFIX ""
             RELEASE_POSTFIX ""
             RELWITHDEBINFO_POSTFIX ""
             MINSIZEREL_POSTFIX ""
)

# Python3_add_library can leave the Windows module output in a
# generator-specific location even when the project globally normalizes bin/lib
# directories. Force the extension and any companion import library into the
# shared build layout so the generated install script resolves the same path
# that the build produced.
set_target_properties(
  unilink_py
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
             ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
)

if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" cfg_upper)
    set_target_properties(
      unilink_py
      PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${cfg_upper}
                 "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
                 RUNTIME_OUTPUT_DIRECTORY_${cfg_upper}
                 "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
                 ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper}
                 "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    )
  endforeach()
endif()

# Link against the main library. On Windows, avoid the imported Python3::Module
# target when vcpkg has installed its own Python package; it can retain vcpkg's
# python312.lib even when the interpreter comes from actions/setup-python.
set(_unilink_python_module_link_target Python3::Module)
if(WIN32
   AND _unilink_python_library
   AND EXISTS "${_unilink_python_library}"
)
  get_target_property(_unilink_py_link_libraries unilink_py LINK_LIBRARIES)
  if(_unilink_py_link_libraries)
    list(REMOVE_ITEM _unilink_py_link_libraries Python3::Module)
    list(FILTER _unilink_py_link_libraries EXCLUDE REGEX
         "([Pp]ython[0-9]+(_d)?\\.lib|Python3::Module)"
    )
    set_target_properties(
      unilink_py PROPERTIES LINK_LIBRARIES "${_unilink_py_link_libraries}"
    )
  endif()

  add_library(unilink_python_module INTERFACE)
  target_link_libraries(
    unilink_python_module INTERFACE "${_unilink_python_library}"
  )
  if(EXISTS "${_unilink_python_include_dir}/Python.h")
    target_include_directories(
      unilink_python_module INTERFACE "${_unilink_python_include_dir}"
    )
  endif()
  set(_unilink_python_module_link_target unilink_python_module)
endif()

target_link_libraries(
  unilink_py PRIVATE pybind11::headers unilink::unilink
                     ${_unilink_python_module_link_target}
)

# Ensure extension builds use the same standard as the C++ library.
target_compile_features(unilink_py PUBLIC cxx_std_20)

get_target_property(_unilink_py_suffix unilink_py SUFFIX)
get_target_property(_unilink_py_debug_postfix unilink_py DEBUG_POSTFIX)
message(STATUS "[unilink-python] module suffix    : ${_unilink_py_suffix}")
message(
  STATUS "[unilink-python] debug postfix   : ${_unilink_py_debug_postfix}"
)

execute_process(
  COMMAND
    ${Python3_EXECUTABLE} -c
    "import sysconfig; p = sysconfig.get_path('platlib', vars={'base':'', 'platbase':''}) or sysconfig.get_path('purelib', vars={'base':'', 'platbase':''}) or ''; print(p.lstrip('/\\\\'))"
  OUTPUT_VARIABLE UNILINK_PYTHON_SITE_PACKAGES
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT UNILINK_PYTHON_SITE_PACKAGES)
  set(UNILINK_PYTHON_SITE_PACKAGES
      "${CMAKE_INSTALL_LIBDIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages"
  )
endif()

# Normalize to forward slashes so the path is safe inside install(CODE "...")
# strings. On Windows, sysconfig returns paths with backslashes (e.g.
# Lib\site-packages), and embedding \s in a CMake string literal is an invalid
# escape sequence that silently corrupts the variable, causing the WIN32
# GET_RUNTIME_DEPENDENCIES block to GLOB nothing.
string(REPLACE "\\" "/" UNILINK_PYTHON_SITE_PACKAGES
               "${UNILINK_PYTHON_SITE_PACKAGES}"
)

set(UNILINK_PYTHON_PACKAGE_DIR "${UNILINK_PYTHON_SITE_PACKAGES}/unilink")

# Build the DIRECTORIES clause for GET_RUNTIME_DEPENDENCIES so vcpkg-installed
# DLLs (e.g. Boost) are resolved and staged next to the Python extension.
# (Removed in favor of explicit directory install to fix CI flakiness)

if(APPLE)
  set_target_properties(unilink_py PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set_target_properties(unilink_py PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

if(UNILINK_ENABLE_INSTALL)
  install(
    TARGETS unilink_py
    COMPONENT python
    LIBRARY DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
    RUNTIME DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/unilink/__init__.py
          ${CMAKE_CURRENT_SOURCE_DIR}/unilink/asyncio.py
    COMPONENT python
    DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
  )

  if(UNILINK_SHARED_LIBRARY_TARGET)
    install(
      TARGETS ${UNILINK_SHARED_LIBRARY_TARGET}
      COMPONENT python
      LIBRARY DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
      RUNTIME DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
    )
  endif()

  if(WIN32)
    if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
      set(_unilink_vcpkg_bin_dir
          "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/"
      )
      if(EXISTS "${_unilink_vcpkg_bin_dir}")
        message(
          STATUS
            "[unilink-python] Staging vcpkg runtime DLLs into Python package from ${_unilink_vcpkg_bin_dir}"
        )
        install(
          DIRECTORY "${_unilink_vcpkg_bin_dir}"
          DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
          COMPONENT python
          FILES_MATCHING
          PATTERN "*.dll"
          PATTERN "python3*.dll" EXCLUDE
        )
      else()
        message(
          STATUS
            "[unilink-python] No vcpkg runtime DLL directory found at ${_unilink_vcpkg_bin_dir} (expected for static triplets)"
        )
      endif()
    endif()
  endif()
endif()
