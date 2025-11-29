# Copilot Instructions

## MSAPI Project Overview

MSAPI is a modular, high-performance C++ library for building Linux-based microservice and macrosystem architectures. It provides internal protocols, utilities, and server logic for efficient, scalable system development.

**Key characteristics:**
- No third-party dependencies
- Thoroughly documented with Doxygen-style comments
- Well covered by unit and end-to-end tests
- Licensed under Polyform Noncommercial License 1.0.0

## Architecture

### Core Components (Concrete File References)

1. **Server Framework** (`library/source/server/`)
  - `server.h` / `server.cpp`: Socket accept loop, connection read/write, buffer dispatch via `HandleBuffer`. Avoid blocking operations inside `HandleBuffer`.
  - `application.h` / `application.cpp`: Application lifecycle (`HandleRunRequest`, `HandlePauseRequest`, `HandleStopRequest`), parameter registration and retrieval. Parameters are currently non-owning references (see Data Models & Ownership).

2. **Protocol Modules** (`library/source/protocol/`)
  - `dataHeader.h` / `dataHeader.cpp`: Common framing header (size, type identifiers). Always validate length before decoding payload.
  - `standard.h` / `standard.cpp`: Key–value dynamic message container (`Standard::Data`). Encoding focuses on minimal copies; keep buffer reuse strategies.
  - `object.h` / `object.cpp`: Lightweight object protocol for POD-like copyable structs; serialization assumes trivially copyable layout.
  - `http.h` / `http.cpp`: Minimal HTTP parsing (request line, headers). Not a full RFC implementation—avoid relying on unsupported features.

3. **Utility Modules** (`library/source/help/`)
  - `log.h` / `log.cpp`: Logger with logging levels and console/file output support.
  - `time.h` / `time.cpp`: Timer, Date, Duration and Event scheduling helpers.
  - `html.h` / `html.cpp`: Lightweight HTML parser.
  - `json.h` / `json.cpp`: Custom JSON DOM + serializer (no external libs). Keep allocations minimal.
  - `table.h` / `table.cpp`: Tabular data structures (`Table`, `TableData`).
  - `bin.h` / `bin.cpp`: Filesystem and binary utilities.
  - `helper.h` / `helper.cpp`: Generic helpers (string ops, path resolution, env utilities, FP comparison).
  - `pthread.hpp`: Threading primitives (mutex/atomic locks/RAII wrappers).
  - `meta.hpp`: Compile-time patterns.
  - `diagnostic.h` / `diagnostic.cpp`: Diagnostic counters / event tracking.
  - `identifier.h` / `identifier.cpp`: ID generation / mapping utilities.
  - `standardType.hpp`: Shared standard protocol type enumerations.

4. **Testing Framework** (`library/source/test/`)
  - `test.h` / `test.cpp`: Base test registration, output formatting.
  - `daemon.hpp`: Support for running servers in the background with direct access.
  - `actionsCounter.*`: Helper for counting test actions.
  - No external test frameworks; everything is custom.

5. **Manager Application Frontend** (`apps/manager/web/`)
  - `html/`, `css/`, `js/`, `images/`: Static assets. (See Frontend Workflow section for enhancement recommendations.)

### Interaction Overview
`Server` receives raw bytes → constructs / validates `DataHeader` → dispatches to protocol decoder (`standard` / `object` / `http`) → application logic consumes decoded structures. Utilities (`log`, `json`, `table`, allocators) support the flow with minimal allocations and deterministic resource handling.

### Extension Points
- Add new protocol: mirror existing pattern (`*.h` + `*.cpp`, integrate with header parsing, update enum/type maps in `meta.hpp`).
- Add new application: subclass or configure `Application`, register parameters, provide handlers for lifecycle hooks.
- Add utility: place under `help/`, keep public API documented, avoid third-party dependencies.

## Data Models & Ownership

### Framing & Headers
- `DataHeader` (`protocol/dataHeader.h`): Contains size, type, and possibly flags. ALWAYS bounds-check against received buffer length before further decode.

### Standard Protocol
- `Standard::Data` (`protocol/standard.h`): Dynamic message storing key–value pairs. Keys are integral identifiers. Memory: prefers reuse of internal buffers—avoid creating transient `std::string` copies when passing around keys.

### Object Protocol
- `object.*`: Assumes trivially copyable struct layout. Use only with plain aggregates (no virtual functions, no internal owning pointers). For non-trivial resources, prefer Standard protocol.

### Tabular Data
- `Table` / `TableData` (`help/table.h`): `TableData` holds the underlying storage; `Table` may provide structural / access helpers.

### Parameters
- `Application` (`server/application.h`) maintains parameter registry. Parameters store non-owning references (observer pattern).
- When adding new parameter types:
  - Prefer value semantics if size small.
  - Use `std::optional<T>` for values that cannot be empty by default.
  - For shared large data (e.g., tables), the field owns the memory; parameters only observe.

### Memory Strategy
- Minimize dynamic allocations in hot paths; prefer stack allocation or buffer reuse.
- Avoid mixing allocator domains for the same object graph.

## Coding Standards

### C++ Style

- **Formatting**: Follow `.clang-format` configuration (WebKit-based style)
- **Indentation**: Use TABS (not spaces), TabWidth=4, IndentWidth=4
- **Line length**: Maximum 120 columns
- **Braces**: Custom brace wrapping style
  - After function: newline
  - After class/control statement/namespace: same line
  - Before else/catch: newline
- **Naming conventions**:
  - Classes: PascalCase (e.g., `Server`, `Application`)
  - Member variables: m_camelCase prefix (e.g., `m_counter`, `m_passedTests`)
  - Functions: PascalCase (e.g., `HandleBuffer`, `GetExecutableDir`)
  - Namespaces: PascalCase (e.g., `MSAPI`, `Helper`)
- Use modern C++ features (C++20, C++23): `[[maybe_unused]]`, `[[unlikely]]`, concepts, etc.
- Prefer `std::optional` over raw pointers where appropriate
- Use `namespace MSAPI` for all library code
- Document public APIs with Doxygen-style comments (`@brief`, `@param`, `@return`)

### File Headers

All source files must include this copyright header:
```cpp
/**************************
 * @file        filename.ext
 * @version     CURRENT_VERSION
 * @date        YYYY-MM-DD
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–YYYY Maksim Andreevich Leonov
 *
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: maks.angels@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–YYYY Maksim Andreevich Leonov, maks.angels@mail.ru
 */
```
- Replacing `filename.ext`, `CURRENT_VERSION`, and `YYYY` as appropriate. Current version can be found in another files, e.g., `library/source/help/log.cpp`.

### Enum Guidance

- Use `enum class` with explicit underlying type sized appropriately (e.g., `enum class ProtocolType : int8_t { ... };`). Prefer integer types, as they can be compiled in more efficient code.
- Preserve sentinel values (e.g., `Undefined`, `Max`) at end; document semantics.
- Use switch statements with `static_assert` to ensure all enum values are covered, including sentinel values. Add a default case with `LOG_ERROR` for unknown values. This matches the established pattern in the codebase (see `standardType.hpp`, `object.cpp`, `log.cpp`, etc.).
- Conversion pattern example:
  ```cpp
static FORCE_INLINE std::string_view EnumToString(const InternalAction value)
{
  static_assert(U(InternalAction::Max) == 4, "Need to specify a new internal action");

  switch (value) {
  case InternalAction::Undefined:
    return "Undefined";
  case InternalAction::Usual:
    return "Usual";
  case InternalAction::Fill:
    return "Fill";
  case InternalAction::Cancel:
    return "Cancel";
  case InternalAction::Max:
    return "Max";
  default:
    LOG_ERROR("Unknown internal action: " + _S(U(value)));
    return "Unknown";
  }
}
```

## Building & Testing

### Environment Setup
```bash
export MSAPI_PATH=/path/to/MSAPI
export BUILD_PROFILE=Debug   # or Release (default CI)
```

### Helper Scripts (Preferred)
Located in `bash/`:
```bash
bash ${MSAPI_PATH}/bash/buildLib.sh      # Build library
bash ${MSAPI_PATH}/bash/buildApps.sh     # Build library and applications
bash ${MSAPI_PATH}/bash/executeTests.sh  # Execute tests
bash ${MSAPI_PATH}/bash/test.sh          # Run full test suite
```

### Direct CMake
```bash
cd ${MSAPI_PATH}/library/build
cmake .
cmake --build . -j "$(nproc)"
```

### Testing Structure
- Test executables: `tests/*/build/`
- Unit tests: `tests/units/`
- Integration tests: `tests/integration/server/`
- MSAPI test framework (`library/source/test/test.h`)
- Each function covered by tests must have a Doxygen `@test` tag in its documentation. For example: `@test Has unit tests`.

### CI Workflows
- `.github/workflows`

## Common Patterns

### Server Implementation
Servers inherit from `MSAPI::Server` and override `HandleBuffer` for custom data handling.

### Application State Management
Applications use state-based lifecycle management via `HandleRunRequest`, `HandlePauseRequest`, etc.

### Logging
Use macros from `log.h`, prefer NEW versions with formatted strings. Format floating-point (FP) values using printf-style format specifiers with the specified precision: `%.9f` for float, `%.17f` for double, and `%.21Lf` for long double. For example: `LOG_INFO_NEW("Value: {:.9f}", floatValue);`

### Testing (C++)
Tests use the `MSAPI::Test` class and call `Assert()` to compare expected/actual values with pass/fail reporting.
Prefer RETURN_IF_FALSE for early exits on failed assertions.

## Frontend Workflow (Manager App)

### Current State
- Static assets only under `apps/manager/web/` (no `package.json`)
- Keep JS modular; avoid global pollution—wrap logic in IIFEs or ES modules
- Optimize images (lossless or WebP) before commit

### JS Core
- **View:** (apps/manager/web/js/view.js) Abstraction for UI views, supporting creation, movement, resizing, snapping, maximizing, hiding, closing, and error handling
- **Table:** (apps/manager/web/js/table.js) Dynamic table creation and management, supporting mutable and immutable tables, validation, and custom column types
- **Grid:** (apps/manager/web/js/grid.js) Flexible grid component for displaying and managing tabular data with sorting, filtering, and column/row operations
- **Timer:** (apps/manager/web/js/timer.js) Timestamp input handling, normalization, and validation with timezone support
- **Duration:** (apps/manager/web/js/duration.js) Duration input parsing, normalization, and validation for multiple time units
- **Select:** (apps/manager/web/js/select.js) Custom select input with searchable options, validation, and dynamic metadata integration
- **Helper:** (apps/manager/web/js/helper.js) Utility functions for type limits, validation, formatting, deep equality, and more

### Default views
- **InstalledApps:** (apps/manager/web/js/views/installedApps.js) Displays a grid of installed MSAPI applications
- **CreatedApps:** (apps/manager/web/js/views/createdApps.js) Shows a grid of created/running apps with parameters and action buttons
- **NewApp:** (apps/manager/web/js/views/newApp.js) Presents a form for creating new applications
- **ModifyApp:** (apps/manager/web/js/views/modifyApp.js) Allows modification of application parameters
- **AppView:** (apps/manager/web/js/views/appView.js) Embeds the application’s custom UI in an iframe
- **TableView:** (apps/manager/web/js/views/tableView.js) Displays a read-only MSAPI table for a given parameter
- **SelectView:** (apps/manager/web/js/views/selectView.js) Provides a searchable, case-sensitive select dialog for choosing parameter values
- **GridSettingsView:** (apps/manager/web/js/views/gridSettingsView.js) Offers a settings panel for grid columns, including alignment, sorting, and filtering options

### JS Style & Conventions
- File header: Use same Polyform license block as existing JS files
- Classes/constructors: PascalCase (`TestRunner`, `TableChecker`)
- Functions/variables: camelCase (`generateUid`, `addRow`)
- Constants: UPPER_SNAKE or PascalCase (`TYPES_LIMITS`)
- Avoid implicit globals; prefer pure functions in `helper.js`

### Frontend Testing (Node-Based)
Location: `apps/manager/web/js/tests/`
```bash
cd apps/manager/web/js/tests
bash runJsTests.sh
```
- Harness: `testRunner.js` provides `TestRunner`, `TableChecker`
- Assertions: `test.Assert(actual, expected, message)`
- Async: `await TestRunner.WaitFor(() => condition, 'description', timeoutMs)`

### Security Considerations
- Sanitize dynamic HTML insertion—avoid `innerHTML` with untrusted data
- Restrict iframe bridge messages to whitelist of actions; validate payload shapes
- Validate response structure before DOM injection

## Performance Recommendations
- Prefer to use continuous memory allocators for frequently accessed objects
- MSAPI library must never throw exceptions
- Use .inl files pattern, where the first part of the file is declarations, followed by implementations
- Use `FORCE_INLINE`

### Server Loop
- Minimize per-connection dynamic allocations; reuse buffers
- Batch reads when possible; avoid per-byte processing

### Protocol Encode/Decode
- Prefer contiguous layouts; validate `DataHeader` early
- Precompute total size before writing to avoid multiple resizes

### Logging
- Avoid logging in hot loops unless behind runtime guard
- Batch expensive formatting outside critical path

## Important Directories

- `library/source/`: Core library source code
  - `server/`: Server and application framework
  - `protocol/`: Protocol implementations
  - `help/`: Utility modules
  - `test/`: Testing framework
- `library/build/`: CMake build configuration
- `tests/`: Test suites
- `apps/manager/`: Manager app with web interface
- `bash/`: Build and test helper scripts
- `.github/workflows/`: CI/CD configuration

## Requirements
- **Build system**: CMake 3.2+
- **Compiler**: C++20 compatible (GCC/Clang)
- **Runtime**: Linux (POSIX threads, Linux sockets)
- **No third-party libraries**: Pure C++ standard library

## Contributor Checklist

1. Add license header to new source files
2. Follow `.clang-format` (run formatting check before commit)
3. Add/update tests for all new public APIs
4. Document enums & new protocol fields; update `meta.hpp` mappings
5. Run `buildLib.sh` + `executeTests.sh` locally; ensure no failures
6. Avoid new third-party runtime dependencies
7. By contributing, you agree to the terms in `CONTRIBUTING.md`