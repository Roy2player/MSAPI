---
name: msapi-architecture
description: Repository structure, module boundaries, detailed component references, and data ownership guidance for MSAPI. Use when a task needs architectural context or when a change spans multiple subsystems.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI architecture guide

## MSAPI Project Overview

### Key characteristics

- MSAPI is a modular, high-performance C++ library for Linux-based microservice and macrosystem architectures.
- The repository intentionally avoids third-party runtime dependencies.
- Public APIs are documented with Doxygen-style comments.
- The codebase is covered by unit, integration, and frontend tests.
- The repository is licensed under the Polyform Noncommercial License 1.0.0.

## Architecture

### Core Components (Concrete File References)

1. **Server Framework** (`library/source/server/`)
   - `server.h` / `server.cpp`: socket accept loop, connection read/write, and buffer dispatch through `HandleBuffer`
   - `application.h` / `application.cpp`: application lifecycle (`HandleRunRequest`, `HandlePauseRequest`, `HandleStopRequest`), parameter registration, and parameter retrieval
   - `authorization.inl`: thread-safe account management and authentication with extensible data models
2. **Protocol Modules** (`library/source/protocol/`)
   - `dataHeader.h` / `dataHeader.cpp`: common framing header with size and type identifiers
   - `standard.h` / `standard.cpp`: dynamic key-value message container optimized around minimal copies
   - `object.h` / `object.cpp`: lightweight object protocol for POD-like copyable structs
   - `http.h` / `http.cpp`: minimal HTTP parsing for request lines and headers
3. **Utility Modules** (`library/source/help/`)
   - `log.h` / `log.cpp`: logger with levels and console/file output support
   - `time.h` / `time.cpp`: timer, date, duration, and scheduling helpers
   - `html.h` / `html.cpp`: lightweight HTML parser
   - `json.h` / `json.cpp`: custom JSON DOM and serializer
   - `table.h` / `table.cpp`: tabular data structures (`Table`, `TableData`)
   - `io.inl`: filesystem I/O utilities
   - `helper.h` / `helper.cpp`: string, path, environment, and numeric helpers
   - `pthread.hpp`: threading primitives and RAII wrappers
   - `meta.hpp`: compile-time patterns and type mappings
   - `diagnostic.h` / `diagnostic.cpp`: diagnostic counters and event tracking
   - `identifier.h` / `identifier.cpp`: UID generation
   - `standardType.hpp`: shared standard-protocol enumerations
   - `sha256.inl`: SHA-256 hashing implementation
4. **Testing Framework** (`library/source/test/`)
   - `test.h` / `test.cpp`: base test registration and output formatting
   - `daemon.hpp`: helpers for running servers in the background with direct access
   - `actionsCounter.*`: helper for counting test actions
   - No external test frameworks are used
5. **Manager Application Frontend** (`apps/manager/web/`)
   - `html/`, `css/`, `js/`, and `images/` contain the static manager assets

### Interaction Overview

`Server` receives raw bytes, validates `DataHeader`, dispatches to the protocol decoder (`standard`, `object`, or `http`), and hands decoded structures to application logic. Utilities such as logging, JSON, tables, helpers, and allocators support the flow with deterministic resource handling and minimal allocations.

### Extension Points

- Add new protocols by mirroring the existing `*.h` + `*.cpp` pattern and updating the related type mappings.
- Add new applications by fitting into the `Application` lifecycle and parameter model.
- Add new utilities under `library/source/help/` and keep them dependency-free and broadly reusable.

## Data Models & Ownership

### Framing & Headers

- `DataHeader` contains size, type, and optional flags. Always bounds-check against the received buffer length before decoding payload data.

### Standard Protocol

- `Standard::Data` stores dynamic key-value messages keyed by integral identifiers. Prefer buffer reuse and avoid transient `std::string` copies when passing keys around.

### Object Protocol

- `object.*` assumes trivially copyable struct layouts. Use it only for plain aggregates with no virtual functions or internal owning pointers.

### Tabular Data

- `TableData` owns the underlying storage. `Table` provides structural and access helpers around that data.

### Parameters

- `Application` stores parameters as non-owning references.
- Prefer value semantics for small parameter values.
- Use `std::optional<T>` for values that should express emptiness explicitly.
- For shared large data such as tables, the field owns the memory and the parameter observes it.

### Memory Strategy

- Minimize dynamic allocations in hot paths.
- Prefer stack allocation or buffer reuse where practical.
- Avoid mixing allocator domains within the same object graph.

## Important Directories

- `library/source/`: core library source code
  - `server/`: server and application framework
  - `protocol/`: protocol implementations
  - `help/`: utility modules
  - `test/`: testing framework
- `library/build/`: CMake build configuration
- `tests/`: test suites
- `apps/manager/`: manager app with web interface
- `bash/`: build and test helper scripts
- `.github/workflows/`: CI/CD configuration

## Requirements

- **Build system**: CMake 3.2+
- **Compiler**: C++20 compatible GCC or Clang
- **Runtime**: Linux with POSIX threads and Linux sockets
- **No third-party libraries**: pure C++ standard library at runtime
