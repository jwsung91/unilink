# unilink-test

This directory contains unit tests to verify the correctness and stability of the `unilink` library. The tests are written using the GoogleTest framework.

## How to Run Tests

From the project's root directory, you can build and run the tests using the following commands.

```bash
# 1. Configure the project with tests enabled
cmake -S . -B build -DBUILD_TESTING=ON

# 2. Build the targets
cmake --build build -j

# 3. Run the tests
cd build
ctest --output-on-failure
```
