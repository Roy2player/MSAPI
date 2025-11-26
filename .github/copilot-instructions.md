# MSAPI Copilot Instructions

## Project Overview

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
  - `log.h` / `log.cpp`: Logger with levels (`TRACE`..`ERROR`) and multi-output support.
  - `time.h` / `time.cpp`: Timers and scheduling helpers.
  - `html.h` / `html.cpp`: Lightweight HTML parser (subset for internal needs).
  - `json.h` / `json.cpp`: Custom JSON DOM + serializer (no external libs). Keep allocations minimal.
  - `table.h` / `table.cpp`: Tabular data structures (`Table`, `TableData`).
  - `bin.h` / `bin.cpp`: Filesystem and binary utilities.
  - `helper.h` / `helper.cpp`: Generic helpers (string ops, path resolution, env utilities).
  - `pthread.hpp`: Threading primitives (mutex/wrappers).
  - `meta.hpp`: Compile-time enum translation patterns (string tables + `static_assert`).
  - `continuousAllocator.hpp`: Region-style allocator for high-frequency temporary objects.
  - `circleContainer.hpp`: Circular buffer container.
  - `diagnostic.h` / `diagnostic.cpp`: Diagnostic counters / event tracking.
  - `identifier.h` / `identifier.cpp`: ID generation / mapping utilities.
  - `standardType.hpp`: Shared standard protocol type enumerations.

4. **Testing Framework** (`library/source/test/`)
  - `test.h` / `test.cpp`: Base test registration, output formatting.
  - `daemon.hpp`: Support for background test daemons.
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
- `Standard::Data` (`protocol/standard.h`): Dynamic message storing key–value pairs. Keys are typically small integral or short string identifiers (confirm usage pattern). Memory: prefers reuse of internal buffers—avoid creating transient `std::string` copies when passing around keys.

### Object Protocol
- `object.*`: Assumes trivially copyable struct layout. Use only with plain aggregates (no virtual functions, no internal owning pointers). For non-trivial resources, prefer Standard protocol.

### Tabular Data
- `Table` / `TableData` (`help/table.h`): `TableData` holds the underlying storage; `Table` may provide structural / access helpers.
- CURRENT OWNERSHIP MODEL (Needs confirmation): Parameters referencing tables inside `Application` are non-owning; the application must ensure lifetime ≥ any consumer callback / request handling.
  - Option A (Document Non-Owning): Keep as-is; instruct contributors to manage lifetime externally.
  - Option B (Owning via `std::shared_ptr<Table>`): API change; adds overhead but clarifies ownership and prevents dangling references.
  - Await maintainer decision—see Open Questions section.

### Parameters
- `Application` (`server/application.h`) maintains parameter registry. Parameters currently appear to store raw pointers / references (non-owning). When adding new parameter types:
  - Prefer value semantics if size small.
  - Use `std::optional<T>` for nullable semantic rather than pointer.
  - For shared large data (e.g., tables), consider `shared_ptr` pending ownership policy.

### Memory Strategy
- Use `continuousAllocator.hpp` for burst allocations that can be discarded wholesale (e.g., per message batch) to reduce fragmentation.
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
- Use modern C++ features (C++20): `[[maybe_unused]]`, `[[unlikely]]`, concepts, etc.
- Prefer `std::optional` over raw pointers where appropriate
- Use `namespace MSAPI` for all library code
- Document public APIs with Doxygen-style comments (`@brief`, `@param`, `@return`)

### File Headers

All source files must include this copyright header:
```cpp
/**************************
 * @file        filename.ext
 * @version     6.0
 * @date        YYYY-MM-DD
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 */
```

### Enum Guidance

- Use `enum class` with explicit underlying type (e.g., `enum class ProtocolType : uint8_t { ... };`) for new enums.
- Preserve sentinel values (e.g., `Unknown`, `COUNT`, `Invalid`) at end; document semantics.
- Follow `meta.hpp` approach: static compile-time array mapping enum values to strings + `static_assert` ensuring array size matches enum count.
- Conversion pattern example:
  ```cpp
  constexpr std::array<std::string_view, static_cast<size_t>(MyEnum::COUNT)> kMyEnumNames{ "A", "B" /* ... */ };
  constexpr std::string_view ToString(MyEnum v) {
    const auto idx = static_cast<size_t>(v);
    if (idx >= kMyEnumNames.size()) return "Unknown";
    return kMyEnumNames[idx];
  }
  static_assert(kMyEnumNames.size() == static_cast<size_t>(MyEnum::COUNT));
  ```
- Migration: Convert unscoped legacy enums to `enum class` incrementally; keep adapter functions for ABI stability.

## Building & Testing

### Environment Setup
```bash
export MSAPI_PATH=/path/to/MSAPI
export BUILD_PROFILE=Debug   # or Release (default CI)
```

### Helper Scripts (Preferred)
Located in `bash/`:
```bash
bash ${MSAPI_PATH}/bash/buildMsapiLib.sh      # Build library
bash ${MSAPI_PATH}/bash/buildMsapiApps.sh     # Build applications
bash ${MSAPI_PATH}/bash/executeMsapiTests.sh  # Execute tests
bash ${MSAPI_PATH}/bash/testMSAPI.sh          # Run full test suite
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
- Integration tests: `tests/server/`, `tests/http/`, `tests/objectProtocol/`, `tests/applicationHandlers/`
- Custom MSAPI test framework (`library/source/test/test.h`)
- Tests output colored results (GREEN for passed, RED for failed)

### Code Formatting
```bash
find . -type f \( -iname "*.cpp" -o -iname "*.h" -o -iname "*.hpp" -o -iname "*.js" \) \
  -not -path "*/CMakeFiles/*" -print0 \
  | xargs -0 -r clang-format-18 --dry-run --Werror --style=file:.clang-format --verbose
```

### CI Workflows
- `build_and_test.yml`: Library + apps build, executes tests
- `clang_format_check.yml`: Enforces formatting
- `test_frontend.yml`: Frontend asset validation

All workflows run on `ubuntu-latest`.

## Common Patterns

### Server Implementation
Servers inherit from `MSAPI::Server` and override `HandleBuffer` for custom data handling.

### Application State Management
Applications use state-based lifecycle management via `HandleRunRequest`, `HandlePauseRequest`, etc.

### Logging
Use the global `MSAPI::logger` object with levels: `TRACE`, `DEBUG`, `INFO`, `WARNING`, `ERROR`.

### Testing (C++)
Tests inherit from `MSAPI::Test` and use `AddTest()` to register test cases with pass/fail reporting.

## Frontend Workflow (Manager App)

### Current State
- Static assets only under `apps/manager/web/` (no `package.json`).
- Keep JS modular; avoid global pollution—wrap logic in IIFEs or ES modules.
- Optimize images (lossless or WebP) before commit.

### JavaScript Architecture
Core scripts in `apps/manager/web/js/`:
- `dispatcher.js`: Central event/message dispatch
- `grid.js` / `table.js`: Tabular & grid-based data views
- `select.js`: Custom select/dropdown logic
- `duration.js` / `timer.js`: Time/nanosecond input helpers
- `helper.js`: General utilities (UID generation, numeric parsing, deep equality)
- `iframeBridge.js`: Cross-frame communication
- `metadataCollector.js`: Metadata for dynamic forms
- `views/*.js`: Individual view controllers

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

### Server Loop
- Minimize per-connection dynamic allocations; reuse buffers
- Batch reads when possible; avoid per-byte processing

### Protocol Encode/Decode
- Prefer contiguous layouts; validate `DataHeader` early
- Precompute total size before writing to avoid multiple resizes

### Allocators
- Use `continuousAllocator.hpp` for short-lived bursts; reset instead of individual frees
- Do NOT store long-lived objects from continuous allocator in global structures

### Logging
- Avoid logging in hot loops at `TRACE` unless behind runtime guard
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

## Dependencies

- **Build system**: CMake 3.2+
- **Compiler**: C++20 compatible (GCC/Clang)
- **Runtime**: Linux (POSIX threads, Linux sockets)
- **No third-party libraries**: Pure C++ standard library

## Contributor Checklist

1. Add license header to new source files
2. Follow `.clang-format` (run formatting check before commit)
3. Add/update tests for all new public APIs
4. Document enums & new protocol fields; update `meta.hpp` mappings
5. Run `buildMsapiLib.sh` + `executeMsapiTests.sh` locally; ensure no failures
6. Avoid new third-party runtime dependencies
7. By contributing, you agree to the terms in `CONTRIBUTING.md`

## Open Questions (Require Maintainer Input)

1. **Table Ownership**: Should `Application` parameters referencing tables remain non-owning or migrate to owning smart pointers?
2. **Enum Migration**: Approve incremental `enum class` conversion plan?
3. **Frontend Dependencies**: Is introducing Node-based tooling acceptable (dev-only, not runtime)?
4. **Performance Baseline**: Are there target latency/throughput numbers for guidance?
5. **Serialization Strategy**: Any planned evolution (e.g., schema versioning) to pre-document?