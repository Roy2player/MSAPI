---
name: Code Style
description: "Naming conventions and code formatting expectations"
---

# Code Style

## General

- Function names must use CamelCase.
- Global and static variable names must use the `LIKE_THAT` style.
- Non-static abstraction fields must use the `m_` prefix.
- Thread-local variables must use the `t_` prefix.
- Boolean flags should be named so they read as questions at the point of use, for example `m_isRunning` or `m_hasData`.
- Code formatting must follow /.clang-format.
- Markdown formatting must follow /.mdlrc.

## Statement grouping

- Groups of statements that are not directly related should be separated by one empty line. Each group represents one logical step, such as input validation, response construction, or sending data.
- Directly related statements should stay together, such as a declaration and the check of its value, or an operation and handling of its result.
- A comment must remain adjacent to the statements it describes, with the separating empty line before the comment.
- After a multiline `if` condition, the empty line must appear between the condition's opening brace and the first statement in its body.

```cpp
if (location.empty() || scheme.empty()
    || host.empty()) [[unlikely]] {

    return false;
}

// Build the response before sending it.
const auto length{ BuildResponse(location, response) };
if (length == 0) {
    return false;
}

return SendResponse(connection, response, length);
```

## Static fields

- Names of static constants used as template or function arguments must make their meaning clear at the point of use, e.g. `Save<CLEAR>`.
- If the function name does not provide enough context, the constant name must include additional annotation, e.g. `Emplace<MODIFY_NOTIFY>` instead of `Emplace<NOTIFY>`.
- Each static field must have a comment explaining its purpose and usage.

```cpp
// Save policy: keep objects in the container after saving
static inline constexpr bool REMAIN{ true };
// Save policy: clear the container after saving
static inline constexpr bool CLEAR{};
```

## Class layout

Class members must be organized into blocks in the following order:

1. Types: enums, nested classes and type aliases.
2. Static fields.
3. Non-static fields.
4. Functions: constructors, destructor, operators and other functions. Public member-function blocks must come first, followed by non-public member-function blocks, with static functions last. Constructors, destructor and operators must be grouped together even when they have different access specifiers, with empty lines separating sub-blocks.
5. Friendship declarations. All friend declarations must follow the fields and functions, including in nested abstractions.

- Each block must start with its access specifier, even if it is the same as that of the previous block.
- Fields in small data abstractions should be as compact as possible to improve cache locality and reduce memory footprint.
- Fields in larger abstractions should be grouped by their related responsibility.
- Each lock must immediately follow the data it protects and be named `m_<data>Lock`.

```cpp
class Single {
public:
    class Object {
        ...
    };

public:
    // Save policy: keep objects in the container after saving
    static inline constexpr bool REMAIN{ true };

private:
    Container<Object> m_container;
    mutable MSAPI::Lock::AtomicRW m_containerLock;

public:
    FORCE_INLINE explicit Single(std::string_view dir, std::string_view name) noexcept;

protected:
    // Comment explaining why it is protected, because that is not obvious
    FORCE_INLINE virtual ~Single() noexcept;

public:
    Single(const Single&) = delete;
    Single(Single&&) = delete;
    Single& operator=(const Single&) = delete;
    Single& operator=(Single&&) = delete;

    FORCE_INLINE [[nodiscard]] bool Read() noexcept;

private:
    FORCE_INLINE [[nodiscard]] bool ReadImpl() noexcept;
};
```

## Namespace

- Namespaces must be declared separately; nested namespace definitions such as `namespace A::B {` must not be used.
- Namespace braces must be separated from their content by an empty line, and each closing brace must have a comment containing the namespace name.

```cpp
namespace MSAPI {

namespace Test {

class Observer;

} // namespace Test

} // namespace MSAPI
```

## Semicolon after closing brace

- A semicolon must not follow a closing brace where the language does not require it, such as after function definitions (including member functions defined inside a class), namespaces, and control-statement blocks.

## Include guard

- Each header must be protected by an `#ifndef`/`#define`/`#endif` include guard; `#pragma once` must not be used.
- The guard name must contain the `MSAPI_` prefix, the filename converted from camelCase to UPPER_SNAKE_CASE, and a suffix corresponding to the file extension, e.g. `recvBuffer.inl` is guarded by `MSAPI_RECV_BUFFER_INL`.
- A location-specific part must follow the `MSAPI_` prefix:
  - `PROTOCOL_` for files in `library/source/protocol/`, e.g. `MSAPI_PROTOCOL_OBJECT_INL`.
  - `UNIT_TEST_` for unit tests, e.g. `MSAPI_UNIT_TEST_SERVER_INL`.
  - `INTEGRATION_TEST_` for integration tests, e.g. `MSAPI_INTEGRATION_TEST_HTTP_PROTOCOL_INL`.
  - `APP_` for applications, e.g. `MSAPI_APP_MANAGER_H`.
- When many files in a suite share the same name (e.g. `test.inl` in each integration test), another identifier must replace the filename to keep guards unique and meaningful. For example, the tested module provides the identifier in `tests/integration/objectProtocol/source/test.inl`, which is guarded by `MSAPI_INTEGRATION_TEST_OBJECT_PROTOCOL_INL`.
- The closing `#endif` must have a comment containing the guard name.

```cpp
#ifndef MSAPI_RECV_BUFFER_INL
#define MSAPI_RECV_BUFFER_INL

... code ...

#endif // MSAPI_RECV_BUFFER_INL
```
