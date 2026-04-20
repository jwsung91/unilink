include(GNUInstallDirs)

if(DEFINED Python3_EXECUTABLE AND NOT DEFINED Python3_ROOT_DIR)
  get_filename_component(Python3_ROOT_DIR "${Python3_EXECUTABLE}" DIRECTORY)
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

find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_include())"
    OUTPUT_VARIABLE _unilink_pybind11_include_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _unilink_pybind11_include_result
  )

  if(NOT _unilink_pybind11_include_result EQUAL 0 OR NOT _unilink_pybind11_include_dir)
    message(
      FATAL_ERROR
        "pybind11 was not found via CMake package config, and the selected Python "
        "interpreter could not provide a pybind11 include directory."
    )
  endif()

  add_library(pybind11::headers INTERFACE IMPORTED)
  set_target_properties(
    pybind11::headers
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_unilink_pybind11_include_dir}"
  )
  message(STATUS "[unilink-python] pybind11 include : ${_unilink_pybind11_include_dir}")
endif()

message(STATUS "[unilink-python] interpreter      : ${Python3_EXECUTABLE}")
message(STATUS "[unilink-python] root dir         : ${Python3_ROOT_DIR}")
message(STATUS "[unilink-python] include dirs     : ${Python3_INCLUDE_DIRS}")
message(STATUS "[unilink-python] libraries        : ${Python3_LIBRARIES}")
message(STATUS "[unilink-python] library dirs     : ${Python3_LIBRARY_DIRS}")

# Inject version from CMake
set(UNILINK_VERSION ${PROJECT_VERSION})
configure_file(module.cpp.in module.cpp @ONLY)

Python3_add_library(unilink_py MODULE WITH_SOABI ${CMAKE_CURRENT_BINARY_DIR}/module.cpp)

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

# Python3_add_library can leave the Windows module output in a generator-specific
# location even when the project globally normalizes bin/lib directories. Force
# the extension and any companion import library into the shared build layout so
# the generated install script resolves the same path that the build produced.
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

# Link against the main library
target_link_libraries(unilink_py PRIVATE pybind11::headers unilink::unilink Python3::Module)

# Ensure we use C++17
target_compile_features(unilink_py PUBLIC cxx_std_17)

get_target_property(_unilink_py_suffix unilink_py SUFFIX)
get_target_property(_unilink_py_debug_postfix unilink_py DEBUG_POSTFIX)
message(STATUS "[unilink-python] module suffix    : ${_unilink_py_suffix}")
message(STATUS "[unilink-python] debug postfix   : ${_unilink_py_debug_postfix}")

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c
          "import sysconfig; p = sysconfig.get_path('platlib', vars={'base':'', 'platbase':''}) or sysconfig.get_path('purelib', vars={'base':'', 'platbase':''}) or ''; print(p.lstrip('/\\\\'))"
  OUTPUT_VARIABLE UNILINK_PYTHON_SITE_PACKAGES
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT UNILINK_PYTHON_SITE_PACKAGES)
  set(
    UNILINK_PYTHON_SITE_PACKAGES
    "${CMAKE_INSTALL_LIBDIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages"
  )
endif()

# Normalize to forward slashes so the path is safe inside install(CODE "...") strings.
# On Windows, sysconfig returns paths with backslashes (e.g. Lib\site-packages), and
# embedding \s in a CMake string literal is an invalid escape sequence that silently
# corrupts the variable, causing the WIN32 GET_RUNTIME_DEPENDENCIES block to GLOB nothing.
string(REPLACE "\\" "/" UNILINK_PYTHON_SITE_PACKAGES "${UNILINK_PYTHON_SITE_PACKAGES}")

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
      message(STATUS "[unilink-python] Staging vcpkg runtime DLLs into Python package")
      install(
        DIRECTORY "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/"
        DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
        COMPONENT python
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "python3*.dll" EXCLUDE
      )
    endif()

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
      install(FILES $<TARGET_RUNTIME_DLLS:unilink_py>
              DESTINATION ${UNILINK_PYTHON_PACKAGE_DIR}
              COMPONENT python)
    endif()
  endif()
endif()
