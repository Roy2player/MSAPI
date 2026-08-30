---
name: code-syntax
description: "Use when writing or fixing code syntax in files. Applies a compact, strict guideline for code syntax with exact tag ordering and wording rules."
---

# Code Syntax

## General

- Each non trivial code block, statement, call or expression must be commented with a brief description of its purpose and behavior. Use `//` for external small comments and ` /* */ ` for external big and internal comments.

### Cases

- Parameter which should be passed by reference, passed by value: ` std::shared_ptr<SomeType> object /* by value as moved */ `

## Function

### Declaration, definition, and call

- Use FORCE_INLINE for all declarations and definitions.
- Use `[[nodiscard]]` for all declarations and definitions of functions that return a value.
- Use `noexcept` for all declarations and definitions of functions that do not throw exceptions. Make sure that internal calls do not throw exceptions as well or make sure that exceptions are caught and handled inside the function.
- On function call use `/*tag=*/value` for all constants.

### Body

- Use `[[likely]]` and `[[unlikely]]` for all branches that are likely or unlikely to be taken, respectively.
- Use `[[maybe_unused]]` for all variables that are not used in the function body.
- Use `[[gnu::uninitialized]]` for all local variables that can be uninitialized at the point of declaration.

## Type

### Basic data type

- Use `int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` instead of build int integer types.
- Use basic data types instead of aliases, like ptrdiff_t, size_t, ssize_t, etc.

### User defined type

- Do not loose scope of user defined types, like class, struct, enum, etc. Use `std::` or `MSAPI::` prefix when using them outside of their namespace. If class is derived from another class, use `BaseClass::` prefix when accessing its data or functions.

#### Declaration

- Use `class` for all class declarations, public or protected fields fields are prohibited.
- Use `explicit` for all constructors that can be called with a single argument.
- Explicitly mark copy and move constructors and assignment operators as `= default` or `= delete`.

## Variable

Use `{}` for all basic data types to highlight that they are initialized with default value.
Use `{ value }` for all initializations of types where no specific std::initializer_list construction exists.
