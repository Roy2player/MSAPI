# Repository custom instructions

These instructions apply to almost every task in `Roy2player/MSAPI`.

## Project baseline

- MSAPI is a dependency-free C++ library for Linux microservices plus a static manager frontend in `apps/manager/web/`.
- Prefer the smallest safe change that fits the current architecture. Do not rewrite subsystems or introduce new frameworks unless the user explicitly asks for that.
- Preserve current public behavior and ABI expectations unless the task clearly requires a breaking change.

## Repository-wide expectations

- Respect module boundaries between `library/`, `apps/`, `tests/`, and `bash/`.
- Library code must remain exception-free.
- Avoid new third-party runtime dependencies.
- Follow `.clang-format`: tabs for indentation, PascalCase types/functions, `m_camelCase` members, and Doxygen-style public API documentation.
- Add the standard project copyright header to new source files.
- Prefer fast, focused validation. Use the existing scripts instead of inventing new workflows.

## Validation commands

- C++ library/app changes: `bash ${MSAPI_PATH}/bash/buildLib.sh`, `bash ${MSAPI_PATH}/bash/buildApps.sh`, `bash ${MSAPI_PATH}/bash/executeTests.sh`
- Frontend JS changes: `bash ${MSAPI_PATH}/apps/manager/web/js/tests/runJsTests.sh ${MSAPI_PATH}/apps/manager/web/js/tests`

## Use skills for task-specific guidance

Load the relevant skill when the task needs deeper repository context:

- `msapi-architecture` for repository structure, module boundaries, and ownership expectations
- `msapi-server-and-protocols` for server flow, application lifecycle, and protocol handling
- `msapi-cpp-conventions` for C++ style, headers, enum patterns, and API design rules
- `msapi-build-and-testing` for build scripts, test layout, and CI expectations
- `msapi-manager-frontend` for manager web UI modules, JS conventions, and frontend security/testing
- `msapi-performance-and-reliability` for hot-path, allocation, logging, and exception-free performance guidance
