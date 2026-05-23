# Release Checklist {#contrib_release_checklist}

Use this checklist before publishing a unilink release.

## 1. Versioning

- [ ] Update `project(VERSION ...)` in `CMakeLists.txt`
- [ ] Confirm CPack package version matches the project version
- [ ] Confirm Git tag matches the release version
- [ ] Confirm release notes mention breaking changes, if any
- [ ] Confirm `unilink-python` compatibility status, if applicable

## 2. Build And Test

- [ ] Full CI passes
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] E2E tests pass
- [ ] Runtime statistics tests pass
- [ ] BestEffort drop accounting tests pass
- [ ] UDP large-payload delivery tests pass
- [ ] Memory safety / sanitizer tests pass
- [ ] Documentation snippet compile checks pass
- [ ] Installed consumer smoke test passes

## 3. Packaging

- [ ] CPack artifacts are generated successfully
- [ ] Package names match `docs/user/installation.md`
- [ ] Package contents include public headers
- [ ] Package contents include CMake package files
- [ ] Package contents include `LICENSE`
- [ ] Package contents include `NOTICE`
- [ ] Package contents include `README.md`
- [ ] External CMake consumer can use `find_package(unilink CONFIG REQUIRED)`
- [ ] External CMake consumer can link against `unilink::unilink`

## 4. Documentation

- [ ] README is current
- [ ] Installation Guide is current
- [ ] Requirements Guide is current
- [ ] API Guide matches actual builder/wrapper behavior
- [ ] API Stability Policy is current
- [ ] Transport Feature Matrix is current
- [ ] Send/backpressure semantics are current
- [ ] SendResult design status is current
- [ ] Known limitations are documented
- [ ] Python bindings link points to `unilink-python`
- [ ] Examples link points to the external examples repository

## 5. Release Assets

Check that expected release assets exist.

- [ ] Linux x64 package
- [ ] Linux ARM64 package
- [ ] macOS package
- [ ] Windows x64 package
- [ ] Windows ARM64 package, if supported
- [ ] Source archive
- [ ] Draft GitHub Release created
- [ ] Release notes include known limitations

## 6. Compatibility Checks

- [ ] vcpkg package status checked
- [ ] External examples repository checked against this release
- [ ] Benchmark repository release/results are linked but not copied into core docs
- [ ] `unilink-python` compatibility checked, if applicable
- [ ] Container image compatibility checked, if applicable

## 7. Local Release Validation Commands

```bash
cmake -S . -B build-v075 \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_BUILD_TESTS=ON \
  -DUNILINK_BUILD_DOCS=OFF \
  -DUNILINK_ENABLE_INSTALL=ON \
  -DUNILINK_ENABLE_PKGCONFIG=ON \
  -DUNILINK_ENABLE_EXPORT_HEADER=ON

cmake --build build-v075 --parallel
ctest --test-dir build-v075 --output-on-failure

cmake --build build-v075 --target doc_snippets_smoke --parallel
ctest --test-dir build-v075 --output-on-failure -L docs_snippets

scripts/verify_installed_consumer.sh --library-mode shared
scripts/verify_installed_consumer.sh --library-mode static
```

For resource-constrained environments:

```bash
ctest --test-dir build-v075 --output-on-failure -j1
```

## 8. Post-Release Checks

- [ ] GitHub Release download links work
- [ ] Documentation site is updated
- [ ] Coverage badge still resolves
- [ ] Installation Guide commands work with the released artifacts
- [ ] Any follow-up issues are created for known limitations
