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
endif()
