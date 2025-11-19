# MSAPI Copilot Instructions

## Project Overview

MSAPI is a modular, high-performance C++ library for building Linux-based microservice and macrosystem architectures. It provides internal protocols, utilities, and server logic for efficient, scalable system development.

**Key characteristics:**
- No third-party dependencies
- Thoroughly documented with Doxygen-style comments
- Well covered by unit and end-to-end tests
- Licensed under Polyform Noncommercial License 1.0.0

## Architecture

### Core Components

1. **Server Framework** (`library/source/server/`)
   - `Server`: TCP socket communication and server lifetime management
   - `Application`: Customization via parameters and state management

2. **Protocol Modules** (`library/source/protocol/`)
   - `object`: Object protocol for simple copyable objects
   - `standard`: Dynamic-size messages as key-value pairs
   - `http`: Basic HTTP message parsing

3. **Utility Modules** (`library/source/help/`)
   - `log`: Logging with multiple levels and outputs
   - `time`: Time utilities and event scheduling
   - `html`: HTML parser
   - `json`: JSON parser and producer
   - `table`: Data structure utilities
   - `pthread`: Thread synchronization
   - `bin`: Filesystem operations
   - `helper`: Miscellaneous utilities
   - `meta`: Static enum translation and compile-time utilities

4. **Testing Framework** (`library/source/test/`)
   - Custom test utilities with daemon support
   - No external test frameworks (e.g., no Google Test, no Catch2)

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
