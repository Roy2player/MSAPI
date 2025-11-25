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

## Enum Guidance

### Preferred Form
- Use `enum class` with explicit underlying type (e.g., `enum class ProtocolType : uint8_t { ... };`) for new enums.
- Preserve sentinel values (e.g., `Unknown`, `COUNT`, `Invalid`) at end; document semantics.

### String Translation Pattern
- Follow `meta.hpp` approach: static compile-time array mapping enum values to strings + `static_assert` ensuring array size matches enum count.
- Conversion pattern example (pseudo):
  ```cpp
  constexpr std::array<std::string_view, static_cast<size_t>(MyEnum::COUNT)> kMyEnumNames{ "A", "B" /* ... */ };
  constexpr std::string_view ToString(MyEnum v) {
    const auto idx = static_cast<size_t>(v);
    if (idx >= kMyEnumNames.size()) return "Unknown";
    return kMyEnumNames[idx];
  }
  static_assert(kMyEnumNames.size() == static_cast<size_t>(MyEnum::COUNT));
  ```

### Migration Strategy
- Incremental: Convert unscoped legacy enums to `enum class` one at a time; keep adapter functions for ABI stability if exposing symbols.
- Avoid sweeping changes that alter binary layout without version bump.
- Document each migrated enum in a CHANGELOG (consider adding if absent).

## Building & Testing (Updated)

### Environment Setup
```bash
export MSAPI_PATH=/path/to/MSAPI
```

### Helper Scripts (Canonical)
Located in `bash/`:
- `buildLib.sh`: Builds core library.
- `buildApps.sh`: Builds applications (e.g., manager).
- `executeTests.sh`: Builds & runs tests (unit + integration) if configured.
- `test.sh`: Aggregated test runner (invokes underlying logic).
- `performanceAnalyzer.sh`: Optional performance / profiling entry.

Usage:
```bash
bash ${MSAPI_PATH}/bash/buildLib.sh
bash ${MSAPI_PATH}/bash/buildApps.sh
bash ${MSAPI_PATH}/bash/executeTests.sh   # or test.sh for full suite
```

### Direct CMake (If Needed)
```bash
cd ${MSAPI_PATH}/library/build
cmake .
cmake --build . -j "$(nproc)"
```

### Build Profiles
Set `BUILD_PROFILE` prior to build (see `library/build/CMakeListsCommonOptions.txt`):
```bash
export BUILD_PROFILE=Debug   # or Release (default CI)
```

### Formatting Check
```bash
find . -type f \( -iname "*.cpp" -o -iname "*.h" -o -iname "*.hpp" -o -iname "*.js" \) \
  -not -path "*/CMakeFiles/*" -print0 \
  | xargs -0 -r clang-format-18 --dry-run --Werror --style=file:.clang-format --verbose
```

### CI Workflows
- `build_and_test.yml`: Library + apps build, executes tests.
- `clang_format_check.yml`: Enforces formatting.
- `test_frontend.yml`: (Frontend asset validation; recommend adding JS tests—see Frontend Workflow.)

## Frontend Workflow (Manager App)

### Current State
- Static assets only under `apps/manager/web/` (no `package.json`).

### Static Asset Guidelines
- Keep JS modular; avoid global pollution—wrap logic in IIFEs or ES modules once `type="module"` adopted.
- Optimize images (lossless or WebP where useful) before commit.
 
### JavaScript Architecture
- Core scripts in `apps/manager/web/js/` provide UI primitives and application orchestration.
  - `dispatcher.js`: Central event/message dispatch; coordinates UI components.
  - `grid.js` / `table.js`: Render and manage tabular & grid-based data views; rely on wrapper elements with CSS classes (`.row`, `.new`, `.add`).
  - `select.js`: Custom select/dropdown logic with programmatic value setting (`Select.SetValue`).
  - `duration.js` / `timer.js`: Conversion & validation helpers for time/nanosecond inputs (`Duration.SetValue`, `Timer.SetValue`).
  - `helper.js`: General utilities (UID generation, numeric parsing, deep equality, JSON transforms).
  - `iframeBridge.js`: Cross-frame communication; isolate sandbox events (keep surface minimal for security).
  - `metadataCollector.js`: Aggregates metadata for dynamic forms (strategy parameters, etc.).
  - `views/*.js`: Individual view controllers (e.g., `appView.js`, `newApp.js`, `tableView.js`)—each should encapsulate DOM creation & event binding.

### Testing (Node-Based)
- Location: `apps/manager/web/js/tests/`.
  - Test files: `*.test.js` (e.g., `dispatcher.test.js`, `grid.test.js`).
  - Harness: `testRunner.js` provides a lightweight framework (`TestRunner`, `TableChecker`).
  - Runner script: `runJsTests.sh` executes all `*.test.js` with Node.
  - Dependencies: Uses `jsdom` & `mutation-observer`; currently implicit (no `package.json`).

#### Running Tests
```bash
cd apps/manager/web/js/tests
bash runJsTests.sh          # runs tests in current dir
bash runJsTests.sh ..       # runs tests one directory up (adjust argument as needed)
```

#### Writing Tests
- Require the harness: `const { TestRunner } = require('./testRunner.js');` (path adjusted per test file).
- Instantiate runner: `const test = new TestRunner();` then define tests with `test.Test(name, async () => { ... });`.
- Assertions: `test.Assert(actual, expected, message)`—keep messages concise; they are aggregated.
- Async conditions: use `await TestRunner.WaitFor(() => condition, 'description', timeoutMs);`.
- Table interactions: `TableChecker.AddRow(test, tableInstance, [values...]);` followed by `tableInstance.Save();` to persist.

### Style & Conventions (JS)
- File header: Use same Polyform license block as existing JS files.
- Naming:
  - Classes / constructors: PascalCase (`TestRunner`, `TableChecker`).
  - Functions & variables: camelCase (`generateUid`, `addRow`).
  - Constants: UPPER_SNAKE or PascalCase depending on usage (`TYPES_LIMITS`).
- Avoid implicit globals—access through explicit exported modules or attach to a controlled namespace object if needed.
- Prefer pure functions in `helper.js`; isolate DOM mutations inside view/controller modules.

### Module Migration Path
1. Add `type="module"` to `<script>` tags in `html/index.html` (staged change; verify load order).
2. Replace `require`/`module.exports` with ES module `import`/`export` syntax.
3. Introduce a build step (optional): `esbuild` or `rollup` for bundling if network request count becomes a concern.
4. Maintain backward compatibility by keeping a transitional UMD wrapper for critical files if needed.

### Performance (Frontend)
- Batch DOM updates: construct fragments off-DOM, then append once.
- Reuse selectors: cache frequently accessed nodes (`const viewsRoot = document.querySelector('.views');`).
- Debounce high-frequency events (scroll, resize) at ~16–50ms.
- Minimize layout thrashing: group `getBoundingClientRect()` calls before mutations.
- Prefer CSS transitions over JS-driven animations.
- Clean up event listeners using stored references (see patched `addEventListener` wrapper in `testRunner.js`).

### Security Considerations
- Sanitize any dynamic HTML insertion—avoid `innerHTML` with untrusted data.
- Restrict iframe bridge messages to a whitelist of actions; validate payload shapes.
- Keep fetch mocks in tests isolated; production fetch logic should validate response structure before DOM injection.

### Adding a New View
1. Create `views/<name>.js` exporting a factory or class.
2. Register with dispatcher (pattern: ensure unique ID or hash using `Helper.GenerateUid()` if needed).
3. Provide an initialization method that accepts root container & optional configuration.
4. Add tests covering rendering + event handling edge cases.

### Frontend Contribution Checklist (Supplemental)
1. License header on new JS files.
2. No implicit globals—attach only via controlled namespace or exports.
3. Provide a test for each new UI component (structure + interactions).
4. Avoid hard-coded timing constants; centralize in one config module if needed.
5. Document any added message types used by `dispatcher.js`.

## Performance Recommendations

### Server Loop (`server/server.cpp`)
- Minimize per-connection dynamic allocations; reuse buffers.
- Batch reads when possible; avoid per-byte processing.
- Offload heavy parsing to worker threads only if contention observed—baseline is synchronous, keep overhead low.

### Protocol Encode/Decode (`protocol/*.cpp`)
- Prefer contiguous layouts; validate `DataHeader` early and bail out on invalid sizes.
- For `Standard::Data`, precompute total size before writing to a buffer to avoid multiple resizes.
- For object protocol, confirm `sizeof(T)` stability; adding non-trivial members invalidates copy assumptions.

### Allocators (`help/continuousAllocator.hpp`)
- Use for short-lived bursts (e.g., message handling cycle). Reset instead of individual frees to reduce fragmentation.
- Do NOT store long-lived objects allocated from continuous allocator in global structures.

### Table Operations (`help/table.*`)
- Coalesce updates; avoid frequent row-level small allocations—prefer block updates.
- If adding indexing features, ensure O(1)/amortized operations do not introduce hidden copy costs.

### Logging (`help/log.*`)
- Avoid logging in hot inner loops at `TRACE` unless behind a runtime guard.
- Batch expensive formatting (JSON dumps) outside critical path.

### Scaling Strategies
- Identify protocol hotspots with `performanceAnalyzer.sh` before optimizing.
- Introduce lock-free structures only after profiling contention (current pthread wrappers suffice for moderate concurrency).
- Consider connection sharding by CPU core using epoll sets if scaling beyond baseline.

## Open Questions (Require Maintainer Input)
1. Table Ownership: Confirm whether `Application` parameters referencing tables should remain non-owning (documented Option A) or migrate to owning smart pointers (Option B).
2. Enum Migration: Approve incremental `enum class` conversion plan? Provide priority list of enums.
3. Frontend Dependencies: Is introducing Node-based tooling acceptable (adds dev-only dependencies, not runtime)?
4. Performance Baseline Metrics: Are there target latency / throughput numbers to include for guidance?
5. Serialization Strategy: Any planned evolution (e.g., schema versioning) that should be pre-documented?

## Contributor Checklist (Quick Reference)
1. Add license header to new source files.
2. Follow `.clang-format` (run formatting check before commit).
3. Add / update tests (unit or integration) for all new public APIs.
4. Document enums & new protocol fields; update `meta.hpp` mappings.
5. Run `buildLib.sh` + `executeTests.sh` locally; ensure no failures.
6. Avoid new third-party runtime dependencies (pending frontend decision).
7. Open a PR referencing Open Questions resolution if changing ownership or enum patterns.

## Common Patterns (Unchanged Core)

### Server Implementation
Servers inherit from `MSAPI::Server` and override `HandleBuffer` for custom data handling.

### Application State Management
Applications use state-based lifecycle management via `HandleRunRequest`, `HandlePauseRequest`, etc.

### Logging
Use the global `MSAPI::logger` object with levels: `TRACE`, `DEBUG`, `INFO`, `WARNING`, `ERROR`.

### Testing
Tests inherit from `MSAPI::Test` and use `AddTest()` to register test cases with pass/fail reporting.

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

### C++ Features

- Use modern C++ features (C++20): `[[maybe_unused]]`, `[[unlikely]]`, concepts, etc.
- Prefer `std::optional` over raw pointers where appropriate
- Use `namespace MSAPI` for all library code
- Document public APIs with Doxygen-style comments (`@brief`, `@param`, `@return`)

## Building and Testing

### Environment Setup

Set the `MSAPI_PATH` environment variable to the repository root:
```bash
export MSAPI_PATH=/path/to/MSAPI
```

### Build Commands

**Using CMake directly:**
```bash
cd library/build
cmake .
cmake --build . -j $(nproc)
```

**Using helper scripts** (preferred):
```bash
# Build library
bash ${MSAPI_PATH}/bash/buildMsapiLib.sh

# Build applications
bash ${MSAPI_PATH}/bash/buildMsapiApps.sh

# Execute tests
bash ${MSAPI_PATH}/bash/executeMsapiTests.sh

# Run all tests
bash ${MSAPI_PATH}/bash/testMSAPI.sh
```

### Build Profiles

Set `BUILD_PROFILE` environment variable to choose build type:
- `Debug`: Debug build with symbols
- `Release`: Optimized release build (default in CI)

See `library/build/CMakeListsCommonOptions.txt` for more details.

### Testing

- Test executables are located in `tests/*/build/`
- Tests use the custom MSAPI test framework (`library/source/test/test.h`)
- Test structure: Unit tests in `tests/units/`, end-to-end tests in subdirectories
- Tests output results with colored console output (GREEN for passed, RED for failed)

### Code Formatting

Format check is enforced via CI. To check formatting:
```bash
find . -type f \( -iname "*.cpp" -o -iname "*.h" -o -iname "*.hpp" -o -iname "*.js" \) \
  -not -path "*/CMakeFiles/*" -print0 \
  | xargs -0 -r clang-format-18 --dry-run --Werror --style=file:.clang-format --verbose
```

## Important Directories

- `library/source/`: Core library source code
  - `server/`: Server and application framework
  - `protocol/`: Protocol implementations
  - `help/`: Utility modules
  - `test/`: Testing framework
- `library/build/`: CMake build configuration for library
- `tests/`: Test suites
  - `units/`: Unit tests
  - `server/`, `http/`, `objectProtocol/`, `applicationHandlers/`: Integration tests
- `apps/`: Applications built on MSAPI
  - `manager/`: Manager app with web interface
- `bash/`: Build and test helper scripts
- `.github/workflows/`: CI/CD configuration

## Dependencies

- **Build system**: CMake 3.2+
- **Compiler**: C++20 compatible compiler (GCC/Clang)
- **Runtime**: Linux (uses POSIX threads, Linux sockets)
- **No third-party libraries**: Pure C++ standard library implementation

## Contribution Guidelines

- By contributing, you agree to the terms in `CONTRIBUTING.md`
- All contributions must be noncommercial
- Retain copyright but grant perpetual license to project maintainer
- Code must pass all tests and formatting checks before merge
- Update documentation for public API changes

## CI/CD

GitHub Actions workflows:
- `build_and_test.yml`: Builds library, apps, and runs tests
- `clang_format_check.yml`: Enforces code formatting standards
- `test_frontend.yml`: Tests the manager frontend

All workflows run on `ubuntu-latest`.

## When Making Changes

1. **Follow existing patterns**: Match the style and structure of surrounding code
2. **Update tests**: Add or update tests in the appropriate `tests/` subdirectory
3. **Run formatting**: Ensure code passes clang-format checks
4. **Build and test locally**: Use the bash helper scripts
5. **Document public APIs**: Use Doxygen-style comments for new public methods
6. **Respect the license**: Ensure all new files include the required copyright header
7. **No new dependencies**: Do not add third-party libraries

## Common Patterns

### Server Implementation
Servers inherit from `MSAPI::Server` and override `HandleBuffer` for custom data handling.

### Application State Management
Applications use state-based lifecycle management via `HandleRunRequest`, `HandlePauseRequest`, etc.

### Logging
Use the global `MSAPI::logger` object with levels: `TRACE`, `DEBUG`, `INFO`, `WARNING`, `ERROR`.

### Testing
Tests inherit from `MSAPI::Test` and use `AddTest()` to register test cases with pass/fail reporting.