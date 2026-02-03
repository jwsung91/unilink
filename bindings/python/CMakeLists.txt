find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(unilink_py module.cpp)

# Link against the main library
target_link_libraries(unilink_py PRIVATE unilink::unilink)

# Ensure we use C++17
target_compile_features(unilink_py PUBLIC cxx_std_17)
