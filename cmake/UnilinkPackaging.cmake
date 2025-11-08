# Unilink packaging configuration
# This file handles all packaging-related settings

# Basic package information
set(CPACK_PACKAGE_NAME "unilink")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unified async communication library")
set(CPACK_PACKAGE_DESCRIPTION "A modern C++ library for unified async communication across TCP, Serial, and other protocols")
set(CPACK_PACKAGE_VENDOR "Jinwoo Sung")
set(CPACK_PACKAGE_CONTACT "jwsung91@example.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/jwsung91/unilink")

# License and documentation
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Package file naming
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-source")

# Platform-specific settings
if(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS;WIX")
  set(CPACK_NSIS_DISPLAY_NAME "Unilink ${PROJECT_VERSION}")
  set(CPACK_NSIS_PACKAGE_NAME "Unilink")
  set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
  set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_NSIS_MODIFY_PATH ON)
  
  # WIX settings
  set(CPACK_WIX_PRODUCT_GUID "12345678-1234-1234-1234-123456789012")
  set(CPACK_WIX_UPGRADE_GUID "87654321-4321-4321-4321-210987654321")
  
elseif(APPLE)
  set(CPACK_GENERATOR "TGZ;DragNDrop")
  set(CPACK_DMG_VOLUME_NAME "Unilink ${PROJECT_VERSION}")
  set(CPACK_DMG_FORMAT "UDZO")
  
elseif(UNIX)
  set(CPACK_GENERATOR "TGZ;DEB;RPM")
  
  # DEB package settings
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}>")
  set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-system1.70.0 | libboost-system1.74.0 | libboost-system1.83.0")
  set(CPACK_DEBIAN_PACKAGE_SUGGESTS "cmake, pkg-config")
  set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libc6-dev")
  
  # RPM package settings
  set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
  set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
  set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_RPM_PACKAGE_REQUIRES "boost-system >= 1.70.0")
  set(CPACK_RPM_PACKAGE_SUGGESTS "cmake, pkg-config")
  set(CPACK_RPM_PACKAGE_RECOMMENDS "glibc-devel")
  
  # Set architecture
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "i386")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
  endif()
endif()

# Source package settings
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/.git/"
  "/.gitignore"
  "/.vscode/"
  "/.idea/"
  "*.swp"
  "*.swo"
  "*~"
  "/docs/html/"
  "/Testing/"
  "/test/"
  "/examples/"
  "CMakeCache.txt"
  "CMakeFiles/"
  "Makefile"
  "cmake_install.cmake"
  "install_manifest.txt"
  "CTestTestfile.cmake"
  "_CPack_Packages/"
  "CPackConfig.cmake"
  "CPackSourceConfig.cmake"
)

# Component-based packaging
set(CPACK_COMPONENTS_ALL
  libraries
  headers
  cmake
  pkgconfig
  documentation
  examples
)

# Component descriptions
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Unilink shared and static libraries")
set(CPACK_COMPONENT_LIBRARIES_REQUIRED TRUE)

set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Header Files")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "Unilink C++ header files")
set(CPACK_COMPONENT_HEADERS_REQUIRED TRUE)

set(CPACK_COMPONENT_CMAKE_DISPLAY_NAME "CMake Files")
set(CPACK_COMPONENT_CMAKE_DESCRIPTION "CMake configuration files for find_package")
set(CPACK_COMPONENT_CMAKE_REQUIRED TRUE)

set(CPACK_COMPONENT_PKGCONFIG_DISPLAY_NAME "pkg-config Files")
set(CPACK_COMPONENT_PKGCONFIG_DESCRIPTION "pkg-config configuration files")
set(CPACK_COMPONENT_PKGCONFIG_DEPENDS libraries)

set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "API documentation and user guides")
set(CPACK_COMPONENT_DOCUMENTATION_DEPENDS headers)

set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Examples")
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Example programs and tutorials")
set(CPACK_COMPONENT_EXAMPLES_DEPENDS libraries headers)

# Install rules for components
if(TARGET unilink_shared)
  install(TARGETS unilink_shared
    COMPONENT libraries
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()

if(TARGET unilink_static)
  install(TARGETS unilink_static
    COMPONENT libraries
    ARCHIVE DESTINATION lib
  )
endif()

if(TARGET unilink AND NOT TARGET unilink_shared AND NOT TARGET unilink_static)
  install(TARGETS unilink
    COMPONENT libraries
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unilink/
  COMPONENT headers
  DESTINATION include/unilink
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/unilinkConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/unilinkConfigVersion.cmake
  COMPONENT cmake
  DESTINATION lib/cmake/unilink
)

install(EXPORT unilinkTargets
  COMPONENT cmake
  NAMESPACE unilink::
  DESTINATION lib/cmake/unilink
)

if(UNILINK_ENABLE_PKGCONFIG)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unilink.pc
    COMPONENT pkgconfig
    DESTINATION lib/pkgconfig
  )
endif()

if(UNILINK_BUILD_DOCS AND UNILINK_DOXYGEN_AVAILABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/html/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/html/
    COMPONENT documentation
    DESTINATION share/doc/unilink/html
  )
endif()

if(UNILINK_BUILD_EXAMPLES)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/
    COMPONENT examples
    DESTINATION share/unilink/examples
    FILES_MATCHING PATTERN "*.cc" PATTERN "*.cpp" PATTERN "*.hpp" PATTERN "*.h"
  )
endif()

# Package versioning
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Include CPack
include(CPack)
