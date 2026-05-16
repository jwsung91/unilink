# unilink tests

The test tree is organized by test scope first, then by component.

```text
test/
├── unit/          # Isolated component, wrapper, transport, config, and framer tests
├── integration/   # Cross-component tests and real loopback I/O paths
├── e2e/           # Scenario and stress tests
├── mocks/         # Mock socket, acceptor, and helper types
└── utils/         # Shared test helpers and constants
```

## Running

```bash
cmake -S . -B build -DUNILINK_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Useful focused runs:

```bash
ctest --test-dir build -L unit
ctest --test-dir build -L integration
ctest --test-dir build -L e2e
ctest --test-dir build -L tcp
ctest --test-dir build -L builder
ctest --test-dir build -L security
ctest --test-dir build -L contract
```

Labels use structured tokens such as `unit_transport_tcp_fast`. For example,
TCP transport unit tests can be selected with a regex such as:

```bash
ctest --test-dir build -L "unit.*transport.*tcp"
```

Label names follow this shape:

```text
<scope>_<component>[_subcomponent]_<kind>[_io]
```

Common examples:

```text
unit_common_fast
unit_transport_tcp_fast
unit_transport_tcp_fast_loopback
unit_wrapper_udp_loopback
integration_tcp_medium
e2e_scenario_slow
docs_snippets
```

Use `_loopback` when a nominal unit test still opens localhost sockets or
allocates real ports. Prefer placing new real I/O coverage under
`integration/`; the loopback suffix is for existing transitional tests or
cases that must stay near a narrow unit fixture.

## Naming

Prefer file names that describe the component and behavior under test:

```text
test_<component>_<behavior>.cc
```

Avoid naming new tests after why they were added, such as `coverage`,
`advanced`, `branches`, or `improvements`, unless that word is the actual
behavior being verified.
