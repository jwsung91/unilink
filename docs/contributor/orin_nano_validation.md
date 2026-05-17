# Orin Nano Validation {#contrib_orin_nano_validation}

Step-by-step validation runbook for `unilink` on NVIDIA Jetson Orin Nano and
similar Ubuntu `aarch64` systems.

---

## Scope

Use this guide when you want to answer one of these questions:

- Does the C++ core library build from source on Ubuntu ARM64?
- Do the unit and integration tests pass on Jetson Orin Nano?
- Do Linux serial integration tests work with `socat` or loopback hardware?
- Does the installed CMake package work with `find_package(unilink)`?

Python bindings are validated in the separate unilink-python repository:

https://github.com/unilink-lab/unilink-python

This guide assumes:

- Ubuntu 22.04 ARM64 is the primary validation baseline
- Ubuntu 24.04 ARM64 is a secondary validation target
- You are building from a local checkout of this repository

## Latest Validation Snapshot

Most recent reported Jetson Orin Nano result:

- Platform: Ubuntu 22.04 on `aarch64`
- Result: `100% tests passed, 0 tests failed out of 481`
- Real elapsed test time: `25.52 sec`
- ARM64 release artifact package generation passed
- Installed-package consumer smoke passed with `find_package(unilink)` and
  `unilink::unilink`
- Serial integration labels passed as part of the full sweep
- One test was listed as not run because it is disabled by design:
  `UdsErrorTest.ServerStopWithActiveSessions`

Interpretation:

- This is strong evidence for Ubuntu 22.04 ARM64 support on Orin Nano across
  C++ core and installed-package consumption paths
- It does not by itself prove every other Linux ARM64 distribution or userspace
  combination

---

## Prerequisites

Install the baseline packages:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ccache \
  pkg-config \
  socat
```

Install C++ dependencies through vcpkg so Boost 1.83.0+ is used consistently:

```bash
vcpkg install boost-asio boost-system spdlog --triplet arm64-linux
```

Check the expected environment:

```bash
uname -m
lsb_release -ds
cmake --version
```

Expected baseline:

- `uname -m` prints `aarch64`
- Ubuntu 22.04 is preferred on Orin Nano

---

## Configure And Build

From the repository root:

```bash
cmake -S . -B build-orin \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_BUILD_DOCS=OFF \
  -DUNILINK_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=arm64-linux
```

```bash
cmake --build build-orin -j"$(nproc)"
```

---

## Run C++ Tests

Run the fast unit-style suite first:

```bash
ctest --test-dir build-orin \
  --output-on-failure \
  --parallel "$(nproc)" \
  --label-regex "unit|core|memory|config"
```

Run the integration suite next:

```bash
ctest --test-dir build-orin \
  --output-on-failure \
  --parallel 2 \
  --label-regex "integration|mock|stable" \
  --label-exclude "slow"
```

Run the end-to-end suite if you want broader confidence:

```bash
ctest --test-dir build-orin \
  --output-on-failure \
  --parallel 2 \
  -L e2e
```

Run everything except documentation snippets:

```bash
ctest --test-dir build-orin \
  --output-on-failure \
  --parallel 2 \
  -LE docs
```

---

## Serial Validation

### Automated Integration Coverage

The ARM64 integration command above already includes serial integration tests.

`test_serial_timeout.cc` creates a virtual serial pair with `socat` when it is
available, so `socat` should be installed before running the integration suite.

### Manual Virtual Serial Pair

If you want to do manual bring-up without hardware:

```bash
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
```

That creates two connected serial endpoints:

- `/tmp/ttyA`
- `/tmp/ttyB`

You can then use the serial example or your own local smoke test against those
endpoints.

### Physical Loopback

If your Orin Nano testbed includes UART hardware, you can validate a real
loopback path:

1. Connect TX/RX appropriately for your adapter or board header.
2. Confirm the device node, for example `/dev/ttyTHS0`, `/dev/ttyUSB0`, or
   `/dev/ttyACM0`.
3. Run your smoke test or serial example with that path.

---

## Pass Criteria

For a practical "supported on Orin Nano" claim, use this minimum bar:

1. `cmake` configure succeeds on Ubuntu 22.04 ARM64.
2. `cmake --build` succeeds with tests enabled.
3. Unit and integration `ctest` commands pass.
4. Installed-package consumer smoke passes with `find_package(unilink)`.

The current Orin Nano report satisfies that bar and also includes:

1. ARM64 `TGZ` package generation.
2. Installed-package consumer smoke using the canonical `unilink::unilink`
   target.

For a stronger "generic Ubuntu ARM64" claim, add:

1. The same validation on Ubuntu 24.04 ARM64.
2. At least one serial validation path, either `socat` or physical loopback.
3. unilink-python ARM64 validation if the Python package is part of your release
   claim.

---

## Troubleshooting

### Boost Not Found

Check that vcpkg installed the ARM64 Boost ports:

```bash
vcpkg list | grep boost
```

If CMake still fails, clear the build directory and reconfigure:

```bash
rm -rf build-orin
cmake -S . -B build-orin ...
```

### Serial Tests Skip

If the serial integration tests skip on ARM64:

- confirm `socat` is installed
- confirm `/tmp` is writable
- check that no stale `socat` process is holding the test symlinks

### Port-Binding Failures

If TCP or UDS tests fail intermittently:

- rerun the failed tests once
- make sure no unrelated local services are occupying ephemeral ports
- reduce test parallelism from `2` to `1`

---

## Related Docs

- [Testing](testing.md)
- [Build Guide](build_guide.md)
- [Requirements](../user/requirements.md)
- [Serial Communication Tutorial](../user/tutorials/04_serial_communication.md)
- [unilink-python](https://github.com/unilink-lab/unilink-python)

[Back to Contributor Guide](index.md)
