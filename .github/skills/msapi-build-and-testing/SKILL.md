---
name: msapi-build-and-testing
description: Build, test, CI, and contributor-checklist guidance for MSAPI. Use when validating changes, updating tests, or investigating repository workflows.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI build and testing guide

## Building & Testing

### Environment Setup

```bash
export MSAPI_PATH=/path/to/MSAPI
export BUILD_PROFILE=Debug   # or Release (default CI)
```

### Helper Scripts (Preferred)

Located in `bash/`:

```bash
bash ${MSAPI_PATH}/bash/buildLib.sh
bash ${MSAPI_PATH}/bash/buildApps.sh
bash ${MSAPI_PATH}/bash/executeTests.sh
bash ${MSAPI_PATH}/bash/test.sh
```

### Direct CMake

```bash
cd ${MSAPI_PATH}/library/build
cmake .
cmake --build . -j "$(nproc)"
```

### Testing Structure

- Test executables are built in `tests/*/build/`.
- Unit tests live under `tests/units/`.
- Integration tests live under `tests/integration/`.
- MSAPI uses its own test framework from `library/source/test/test.h`.
- Functions covered by tests should document that coverage with a Doxygen `@test` tag.

### CI Workflows

- `.github/workflows/build_and_test.yml` covers the C++ library and application build/test path.
- `.github/workflows/test_frontend.yml` covers frontend JavaScript tests.
- `.github/workflows/clang_format_check.yml` covers formatting checks for C++, headers, `.inl`, and JS files.

## Common Patterns

### Testing (C++)

- Use `MSAPI::Test` and `Assert()` to compare expected and actual values.
- Prefer `RETURN_IF_FALSE` for early exits after failed assertions.
- Keep tests fast, deterministic, and focused on observable behavior.

## Contributor Checklist

0. Look at documentation near definitions to understand the existing implementation ideas and capabilities.
1. Add the license header to new source files.
2. Follow `.clang-format` and formatting checks before commit.
3. Add or update tests for new public APIs.
4. Document enums and new protocol fields, including related `meta.hpp` mappings.
5. Run `buildLib.sh` and `executeTests.sh` locally and ensure no failures.
6. Avoid new third-party runtime dependencies.
7. Contributions follow the terms in `CONTRIBUTING.md`.
