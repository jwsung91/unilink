find_package(pybind11 CONFIG REQUIRED)

# Inject version from CMake
set(UNILINK_VERSION ${PROJECT_VERSION})
configure_file(module.cpp.in module.cpp @ONLY)

pybind11_add_module(unilink_py ${CMAKE_CURRENT_BINARY_DIR}/module.cpp)

# Link against the main library
target_link_libraries(unilink_py PRIVATE unilink::unilink)

# Ensure we use C++17
target_compile_features(unilink_py PUBLIC cxx_std_17)
