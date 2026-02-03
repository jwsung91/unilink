find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(unilink_py module.cpp)

# Link against the main library
target_link_libraries(unilink_py PRIVATE unilink::unilink)

# Ensure we use C++17
target_compile_features(unilink_py PUBLIC cxx_std_17)

# Output the module to the root binary directory (e.g. build/ or build/Release)
# This ensures it sits alongside the main library (for DLL resolution on Windows)
# and matches the PYTHONPATH expectations in the CI script.
set_target_properties(unilink_py PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
