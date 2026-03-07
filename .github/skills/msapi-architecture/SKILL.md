---
name: msapi-architecture
description: Repository structure, module boundaries, and data ownership guidance for MSAPI. Use when a task needs architectural context or when a change spans multiple subsystems.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI architecture guide

## High-level structure

- `library/source/server/`: socket server framework, application lifecycle handling, and authorization helpers
- `library/source/protocol/`: framing and protocol implementations (`dataHeader`, `standard`, `object`, `http`)
- `library/source/help/`: shared utilities such as logging, time, JSON, HTML, tables, helpers, identifiers, diagnostics, and threading primitives
- `library/source/test/`: MSAPI's internal test framework and daemon helpers
- `apps/manager/web/`: static manager frontend assets
- `tests/`: unit and integration suites
- `bash/`: preferred build and test entrypoints

## Design constraints

- MSAPI is modular, performance-oriented, and intentionally avoids third-party dependencies.
- Favor small, localized changes that fit the existing layering.
- Reuse the current domain language from the repository instead of introducing parallel abstractions.

## Ownership and data model expectations

- `Application` parameters are non-owning references; the owning field keeps the storage alive.
- `Standard::Data` is a dynamic key-value protocol optimized to avoid unnecessary copies.
- `object.*` assumes trivially copyable payload layouts; use it only for plain aggregate-style data.
- `TableData` owns tabular storage; `Table` provides access and structure helpers.
- Validate protocol framing early and keep ownership/lifetime explicit when threading data across layers.

## Extension points

- New protocols should follow the existing `*.h` + `*.cpp` pattern and integrate with current type mappings.
- New applications should fit the `Application` lifecycle and parameter model.
- New utilities belong in `library/source/help/` and should stay dependency-free and broadly reusable.
