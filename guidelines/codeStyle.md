---
name: Code Style
description: "Naming conventions and code formatting expectations"
---

# Code Style

## General

- CamelCase functions naming.
- Global and static variables naming style is `LIKE_THAT`.
- `m_` prefix for all non static abstraction fields.
- `t_` prefix for all thread local variables.

## Static fields

- Static constants, which are used as template or function arguments, are named to give clear indication of what is managed in the place of usage, e.g. `Save<CLEAR>`. If the name of the function is not enough to understand it, the name has additional annotation, e.g. `Emplace<MODIFY_NOTIFY>` instead of `Emplace<NOTIFY>`.
- Each static field has a comment explaining its purpose and usage.

```cpp
// Save policy: keep objects in the container after saving
static inline constexpr bool REMAIN{ true };
// Save policy: clear the container after saving
static inline constexpr bool CLEAR{};
```

## Class layout

- Members of a class follow blocks in the order below:
  1. Types: enums, nested classes and type aliases.
  2. Static fields.
  3. Non static fields.
  4. Functions: constructors, destructor, operators and other functions. Member public functions block is first, non public functions blocks follow it, and the static functions block is last. Sub-blocks: constructor, destructor and operators are grouped together even if they are under different access specifiers with empty lines separating sub-blocks.
- Each block starts with its access specifier, even if it is the same as the access specifier of the previous block.

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
    mutable MSAPI::Lock::AtomicRW m_lock;

public:
    FORCE_INLINE Single(std::string_view dir, std::string_view name) noexcept;

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

- Declare each namespace separately, nested namespace definition `namespace A::B {` is prohibited.
- Separate namespace braces from the content by an empty line and mark each closing brace by a comment with the namespace name.

```cpp
namespace MSAPI {

namespace Test {

class Observer;

} // namespace Test

} // namespace MSAPI
```

## Semicolon after closing brace

- `;` is not placed after a closing brace where it is not required by the language: function definitions (including member functions defined inside class), namespaces, and control statement blocks.

## Include guard

- Each header is protected by `#ifndef`/`#define`/`#endif` include guard, `#pragma once` is not used.
- Guard name is `MSAPI_` prefix, then file name converted from camelCase to UPPER_SNAKE_CASE, then `_INL` suffix according to the file extension, e.g. `recvBuffer.inl` is guarded by `MSAPI_RECV_BUFFER_INL`.
- Location related part is added after `MSAPI_` prefix:
  - `PROTOCOL_` for files in `library/source/protocol/`, e.g. `MSAPI_PROTOCOL_OBJECT_INL`;
  - `UNIT_TEST_` for unit tests, e.g. `MSAPI_UNIT_TEST_SERVER_INL`;
  - `INTEGRATION_TEST_` for integration tests, e.g. `MSAPI_INTEGRATION_TEST_HTTP_PROTOCOL_INL`;
  - `APP_` for applications, e.g. `MSAPI_APP_MANAGER_H`.
  - If many files of one suite share the same name (e.g. `test.inl` in each integration test), another identifier replaces the file name to keep guards unique and meaningful, e.g. the name of the tested module: `tests/integration/objectProtocol/source/test.inl` is guarded by `MSAPI_INTEGRATION_TEST_OBJECT_PROTOCOL_INL`.
- Closing `#endif` is marked by a comment with the guard name.

```cpp
#ifndef MSAPI_RECV_BUFFER_INL
#define MSAPI_RECV_BUFFER_INL

... code ...

#endif // MSAPI_RECV_BUFFER_INL
```

Use /.clang-format.
