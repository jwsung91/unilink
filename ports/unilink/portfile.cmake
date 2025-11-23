vcpkg_minimum_required(VERSION 2023-11-15)

# Use local sources when consuming this overlay.
set(SOURCE_PATH "${CURRENT_PORT_DIR}/../..")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DUNILINK_BUILD_TESTS=OFF
        -DUNILINK_ENABLE_PERFORMANCE_TESTS=OFF
        -DUNILINK_BUILD_EXAMPLES=OFF
        -DUNILINK_BUILD_DOCS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME unilink CONFIG_PATH lib/cmake/unilink)
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()

# Clean debug-only share dir
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
