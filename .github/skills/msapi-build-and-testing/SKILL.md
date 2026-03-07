---
name: msapi-build-and-testing
description: Build, test, and CI guidance for MSAPI. Use when validating changes, updating tests, or investigating repository workflows.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI build and testing guide

## Environment

```bash
export MSAPI_PATH=/path/to/MSAPI
export BUILD_PROFILE=Debug   # Release is the common CI profile
```

## Preferred scripts

```bash
bash ${MSAPI_PATH}/bash/buildLib.sh
bash ${MSAPI_PATH}/bash/buildApps.sh
bash ${MSAPI_PATH}/bash/executeTests.sh
bash ${MSAPI_PATH}/bash/test.sh
```

## Direct CMake fallback

```bash
cd ${MSAPI_PATH}/library/build
cmake .
cmake --build . -j "$(nproc)"
```

## Test layout

- Unit tests live under `tests/units/`
- Integration tests live under `tests/integration/`
- Test executables are built into `tests/*/build/`
- MSAPI uses its own framework in `library/source/test/`

## Frontend tests

```bash
bash ${MSAPI_PATH}/apps/manager/web/js/tests/runJsTests.sh ${MSAPI_PATH}/apps/manager/web/js/tests
```

- The frontend tests use the custom JS harness in `apps/manager/web/js/tests/`.

## CI expectations

- `.github/workflows/build_and_test.yml` covers the C++ build/test path.
- `.github/workflows/test_frontend.yml` covers frontend JS tests.
- `.github/workflows/clang_format_check.yml` covers formatting checks for C++ and JS sources.

## Testing guidance

- Prefer fast, deterministic tests focused on observable behavior.
- Add or update tests when public behavior changes.
- Keep new validation aligned with the existing scripts and harnesses instead of introducing new tooling.
