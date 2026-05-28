# Release Checklist

This checklist is maintained in the core repository because release packaging,
CI, CPack, and consumer smoke workflows live here.

## Version

- [ ] `CMakeLists.txt` project version is updated.
- [ ] Release tag matches the project version.
- [ ] README version-sensitive examples are still valid.

## Build and test

- [ ] Full CI passed.
- [ ] Unit tests passed.
- [ ] Integration tests passed.
- [ ] Installed consumer smoke passed for shared mode.
- [ ] Installed consumer smoke passed for static mode.

## Packaging

- [ ] Release workflow dry-run completed.
- [ ] CPack package was generated.
- [ ] Package contains headers.
- [ ] Package contains library artifacts.
- [ ] Package contains `unilinkConfig.cmake`.
- [ ] Package contains `unilinkTargets.cmake`.
- [ ] Package contains `README.md`.
- [ ] Package contains `LICENSE`.
- [ ] Package contains `NOTICE`.

## Documentation

- [ ] Core README links to `unilink-docs`.
- [ ] Core minimal docs are present:
  - [ ] `docs/installation.md`
  - [ ] `docs/quickstart.md`
  - [ ] `docs/api_stability.md`
  - [ ] `docs/release_checklist.md`
- [ ] `unilink-docs` is updated when public API or behavior changes.
- [ ] Doxygen workflow in `unilink-docs` passes.

## Benchmark / validation

- [ ] Latest relevant benchmark result is preserved in `unilink-benchmarks`.
- [ ] Known benchmark/environment limitations are documented.
- [ ] UDP large-payload classification is current.
- [ ] Orin/WSL2 validation notes are current when relevant.

## External repositories

- [ ] `unilink-python` compatibility impact checked.
- [ ] `unilink-examples` compatibility impact checked, if examples depend on changed API.
- [ ] `unilink-benchmarks` compatibility impact checked, if benchmark APIs changed.

## Release notes

- [ ] Release notes summarize user-facing changes.
- [ ] Breaking changes are explicitly marked.
- [ ] Known limitations are included.
- [ ] ABI stability disclaimer is included if pre-1.0.
