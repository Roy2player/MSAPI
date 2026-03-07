---
name: msapi-cpp-conventions
description: C++ coding, API, and source layout conventions for MSAPI. Use when editing C++ headers, source files, enums, or public APIs.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI C++ conventions

## Style and naming

- Follow `.clang-format` exactly. The project uses tabs, a WebKit-derived layout, and a 120-column limit.
- Classes, namespaces, and functions use PascalCase.
- Member variables use the `m_camelCase` form.
- Favor modern C++ features that fit the codebase and compiler baseline.

## API and error handling

- Library code must not throw exceptions.
- Prefer explicit ownership, RAII, and value semantics where they fit the existing design.
- Keep APIs small, predictable, and consistent with current naming and module boundaries.
- Document public APIs with Doxygen tags such as `@brief`, `@param`, `@return`, and `@test` when applicable.

## New source files

- Add the standard MSAPI copyright/license header to new source files.
- Keep `.inl` files in the current style: declarations first, followed by inline implementations.
- Use `FORCE_INLINE` where the codebase already expects it.

## Enum pattern

- Prefer `enum class` with an explicit underlying type.
- Preserve sentinel values such as `Undefined` and `Max`.
- Use `switch` statements with `static_assert` coverage checks and log unexpected values with `LOG_ERROR`.

## Change strategy

- Prefer incremental refactors over sweeping rewrites.
- Preserve API and ABI expectations unless the task explicitly allows breaking changes.
