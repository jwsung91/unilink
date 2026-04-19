include(GNUInstallDirs)

find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

# Inject version from CMake
set(UNILINK_VERSION ${PROJECT_VERSION})
configure_file(module.cpp.in module.cpp @ONLY)

pybind11_add_module(unilink_py ${CMAKE_CURRENT_BINARY_DIR}/module.cpp)

# Link against the main library
target_link_libraries(unilink_py PRIVATE unilink::unilink)

# Ensure we use C++17
target_compile_features(unilink_py PUBLIC cxx_std_17)

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

file(GLOB _unilink_extension_candidates \"\${_unilink_package_dir}/unilink_py*.pyd\")
file(GLOB _unilink_runtime_candidates \"\${_unilink_package_dir}/unilink*.dll\")

set(_unilink_runtime_targets \${_unilink_extension_candidates} \${_unilink_runtime_candidates})
list(FILTER _unilink_runtime_targets EXCLUDE REGEX [[[/\\\\]python3\\.dll$]])
list(FILTER _unilink_runtime_targets EXCLUDE REGEX [[[/\\\\]python312\\.dll$]])

if(_unilink_runtime_targets)
  file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES \${_unilink_runtime_targets}
    RESOLVED_DEPENDENCIES_VAR _unilink_resolved_deps
    UNRESOLVED_DEPENDENCIES_VAR _unilink_unresolved_deps
    PRE_EXCLUDE_REGEXES
      [[api-ms-win-.*]]
      [[ext-ms-.*]]
    POST_EXCLUDE_REGEXES
      [[.*[/\\\\]Windows[/\\\\].*]]
      [[.*[/\\\\]system32[/\\\\].*]]
      [[.*[/\\\\]python3\\.dll$]]
      [[.*[/\\\\]python312\\.dll$]]
  )

  foreach(_unilink_dependency IN LISTS _unilink_resolved_deps)
    get_filename_component(_unilink_dep_name \"\${_unilink_dependency}\" NAME)
    string(TOLOWER \"\${_unilink_dep_name}\" _unilink_dep_name_lower)
    if(_unilink_dep_name_lower MATCHES [[^python3\\.dll$]] OR _unilink_dep_name_lower MATCHES [[^python312\\.dll$]])
      continue()
    endif()

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
