---
name: Code Syntax
description: "Syntax and declaration rules"
---

# Code Syntax

## Comments

- Each non trivial code block, statement, call or expression must be commented with a brief description of its purpose and behavior.
- Comments use `// Comment` style: `//`, one space, then the text. Multi-line comment is a sequence of `//` lines, empty line inside it is `//`. Markers right after `//` like `//*` are not used.
- `/* */` is used only in particular situations:
  - short note inside code, e.g. argument tag `/*tag=*/value` or parameter note ` /* by value as moved */ `, also short note inside an expression or initializer list;
  - comment inside a multi-line macro definition, where `//` would comment out the line continuation `\`.
- Doxygen documentation blocks and `/*---...---*/` section separators follow [Doxygen Inline Documentation](doxygenInlineDocumentation.md).
- Other types of comments are prohibited, including markers (`//*`, `//!`, `//?`) and task comments (`// FIXME`, etc.).
- Exception: for further simplicity `// TODO: comment` can duplicate an existing `@todo` in code to explicitly mark the place for further work. It is never used without the corresponding `@todo`, which remains the source of truth.
- Problems, technical debt and open questions are described by the `@todo` tag in the documentation block of the related function or abstraction, see [Doxygen Inline Documentation](doxygenInlineDocumentation.md).

```cpp
// Retry is required, as connection can be closed by peer during handshake
// and new attempt is made with the same id
const auto connection{ OpenConnection(ip, port, /*doReconnection=*/false) };
```

## Function

### Declaration, definition, and call

- Use FORCE_INLINE for all declarations and definitions.
- Use `[[nodiscard]]` for all declarations and definitions of functions that return a value.
- Use `noexcept` for all declarations and definitions of functions that do not throw exceptions. Make sure that internal calls do not throw exceptions as well or make sure that exceptions are caught and handled inside the function.
- On function call use `/*tag=*/value` for all constants.

### Body

- Use `[[likely]]` and `[[unlikely]]` for all branches that are likely or unlikely to be taken, respectively.
- Use `[[maybe_unused]]` for all variables that are not used in the function body.
- Use `[[indeterminate]]` for all local variables that can be uninitialized at the point of declaration.
- `goto` operator is prohibited. Control flow is expressed by structured statements: loops, `break`/`continue` and early `return`.

## Type

### Basic data type

- Use `int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` instead of build int integer types.
- Use basic data types instead of aliases, like ptrdiff_t, size_t, ssize_t, etc.

### User defined type

- Do not loose scope of user defined types, like class, struct, enum, etc. Use `std::` or `MSAPI::` prefix when using them outside of their namespace. If class is derived from another class, use `BaseClass::` prefix when accessing its data or functions.

#### Declaration

- Use `class` for all class declarations, public or protected fields fields are prohibited.
- Use `explicit` for all constructors that can be called with a single argument.
- Copy constructor, move constructor, copy assignment and move assignment operators are always explicitly declared as `= default` or `= delete`, all four together. Implicit generation of them depends on other members silently: user declared destructor suppresses implicit move and makes the type copied instead, lock or `std::unique_ptr` field makes copying deleted. Explicit declaration documents the intent and lets the compiler check it.
- Destructor and default constructor are not required to be declared explicitly, as their implicit behavior is not surprising:
  - destructor is declared only if it has a behavior (e.g. releases a resource) or the class is a polymorphic base, which requires `virtual ~Base() = default;`;
  - default constructor is declared as `= default` if default construction is required together with other constructors, as declaring any other constructor suppresses the implicit one; in other cases its declaration is optional.

```cpp
class Connection {
public:
    FORCE_INLINE explicit Connection(int32_t socket) noexcept;
    FORCE_INLINE ~Connection() noexcept;

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;
};
```

- Use `mutable` only for lock fields, see [Concurrency](concurrency.md#lock-fields-and-constness).

#### Enum

- Use `enum class` with an explicit, appropriately sized integer underlying type.
- Preserve documented sentinel values such as `Undefined` and `Max`.
- Keep `EnumToString` switches, their `static_assert` coverage checks, and unknown-value logging synchronized whenever an enum changes.

#### Overriding

- Overridden methods are grouped by the parent and each group is marked by `// Parent` comment with full qualified name of the parent whose methods are overridden, e.g. `// MSAPI::Server`.
- Override which is not expected to be overridden further is marked by `final`.
- Override which is expected to be overridden further is marked by `override`. `virtual` is not repeated on overrides.

```cpp
class Manager : public MSAPI::Server, public MSAPI::Protocol::HTTP::IHandler {
public:
    // MSAPI::Server
    void HandleBuffer(MSAPI::RecvBuffer& recvBuffer) final;
    // MSAPI::Application
    void HandleRunRequest() override;
    void HandlePauseRequest() final;
    // MSAPI::Protocol::HTTP::IHandler
    void HandleHttp(const std::shared_ptr<MSAPI::Connection::Data>& connectionData,
        const MSAPI::Protocol::HTTP::Data& data) final;
};
```

## Variable

Use `{}` for all basic data types to highlight that they are initialized with default value.
Use `{ value }` for all initializations of types where no specific std::initializer_list construction exists.

## Macros

- `#define` macros are avoided. Functions, lambdas and templates are used instead to keep code readable. Macros take place only in very specific cases.
- Lambda does not lose in performance to macro. Its call operator is implicitly inline and its closure type is unique, so with enabled optimization (`-O2`, `-O3`) a directly called lambda is inlined and the generated code is the same as for an equivalent macro. Without optimization a lambda is called as a function, and in case inlining is required in any build configuration, it is forced by `[[gnu::always_inline]]` attribute, the lambda analog of `FORCE_INLINE`.

```cpp
// Instead of #define SQUARE(x) ((x) * (x))
const auto square{ [] [[gnu::always_inline]] (const uint64_t value) { return value * value; } };
const auto result{ square(size + 1) };
```

## Friendships

- Use `// Comment` to define the purpose of friendship.
- Use `// Comment` to mark place where direct access via friendship is happening.
