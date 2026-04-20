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
find_package(pybind11 CONFIG REQUIRED)

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
# Requires double-escaping: \\\" survives the CMakeLists.txt string literal and
# becomes \" in the generated cmake_install.cmake string literal, which in turn
# becomes a literal " when cmake_install.cmake is parsed at install time.
set(_unilink_vcpkg_dirs_clause "")
if(WIN32 AND VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
  set(_unilink_vcpkg_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  string(REPLACE "\\" "/" _unilink_vcpkg_bin_dir "${_unilink_vcpkg_bin_dir}")
  set(_unilink_vcpkg_dirs_clause "DIRECTORIES \\\"${_unilink_vcpkg_bin_dir}\\\"")
  message(STATUS "[unilink-python] vcpkg bin dir   : ${_unilink_vcpkg_bin_dir}")
  message(STATUS "[unilink-python] dirs clause      : ${_unilink_vcpkg_dirs_clause}")
else()
  message(STATUS "[unilink-python] vcpkg DIRECTORIES skipped"
    " (WIN32=${WIN32} VCPKG_INSTALLED_DIR='${VCPKG_INSTALLED_DIR}'"
    " VCPKG_TARGET_TRIPLET='${VCPKG_TARGET_TRIPLET}')")
endif()

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
    install(
      CODE
        "
set(_unilink_package_dir \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${UNILINK_PYTHON_PACKAGE_DIR}\")
file(TO_CMAKE_PATH \"\${_unilink_package_dir}\" _unilink_package_dir)
message(STATUS \"[unilink-install] package dir: \${_unilink_package_dir}\")

file(GLOB _unilink_extension_candidates \"\${_unilink_package_dir}/unilink_py*.pyd\")
file(GLOB _unilink_runtime_candidates \"\${_unilink_package_dir}/unilink*.dll\")
message(STATUS \"[unilink-install] pyd candidates : \${_unilink_extension_candidates}\")
message(STATUS \"[unilink-install] dll candidates : \${_unilink_runtime_candidates}\")

set(_unilink_runtime_targets \${_unilink_extension_candidates} \${_unilink_runtime_candidates})
list(FILTER _unilink_runtime_targets EXCLUDE REGEX [[[/\\\\]python3\\.dll$]])
list(FILTER _unilink_runtime_targets EXCLUDE REGEX [[[/\\\\]python312\\.dll$]])
message(STATUS \"[unilink-install] runtime targets: \${_unilink_runtime_targets}\")

if(_unilink_runtime_targets)
  file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES \${_unilink_runtime_targets}
    RESOLVED_DEPENDENCIES_VAR _unilink_resolved_deps
    UNRESOLVED_DEPENDENCIES_VAR _unilink_unresolved_deps
    ${_unilink_vcpkg_dirs_clause}
    PRE_EXCLUDE_REGEXES
      [[api-ms-win-.*]]
      [[ext-ms-.*]]
    POST_EXCLUDE_REGEXES
      [[.*[/\\\\]Windows[/\\\\].*]]
      [[.*[/\\\\]system32[/\\\\].*]]
      [[.*[/\\\\]python3\\.dll$]]
      [[.*[/\\\\]python312\\.dll$]]
  )
  message(STATUS \"[unilink-install] resolved deps  : \${_unilink_resolved_deps}\")
  message(STATUS \"[unilink-install] unresolved deps: \${_unilink_unresolved_deps}\")

  foreach(_unilink_dependency IN LISTS _unilink_resolved_deps)
    get_filename_component(_unilink_dep_name \"\${_unilink_dependency}\" NAME)
    string(TOLOWER \"\${_unilink_dep_name}\" _unilink_dep_name_lower)
    if(_unilink_dep_name_lower MATCHES [[^python3\\.dll$]] OR _unilink_dep_name_lower MATCHES [[^python312\\.dll$]])
      continue()
    endif()

    message(STATUS \"[unilink-install] staging: \${_unilink_dep_name}\")
    execute_process(
      COMMAND \"${CMAKE_COMMAND}\" -E copy_if_different
              \"\${_unilink_dependency}\"
              \"\${_unilink_package_dir}/\${_unilink_dep_name}\"
      COMMAND_ERROR_IS_FATAL ANY
    )
  endforeach()

  foreach(_unilink_unresolved IN LISTS _unilink_unresolved_deps)
    string(TOLOWER \"\${_unilink_unresolved}\" _unilink_unresolved_lower)
    if(
      NOT _unilink_unresolved_lower MATCHES [[^api-ms-win-]]
      AND NOT _unilink_unresolved_lower MATCHES [[^ext-ms-]]
      AND NOT _unilink_unresolved_lower MATCHES [[^python3\\.dll$]]
      AND NOT _unilink_unresolved_lower MATCHES [[^python312\\.dll$]]
    )
      message(WARNING \"Unresolved runtime dependency for Python package install: \${_unilink_unresolved}\")
    endif()
  endforeach()
endif()
"
      COMPONENT python
    )
  endif()
endif()
