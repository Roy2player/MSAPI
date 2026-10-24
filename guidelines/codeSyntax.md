---
name: Code Syntax
description: "Syntax and declaration rules"
---

# Code Syntax

## Comments

- Comments must explain information that cannot be readily inferred from the code itself. When not otherwise sufficiently explicit, the following information must be explained in comments:
  - non-obvious function or template arguments, including ambiguous boolean values and unexplained magic values;
  - implementation intent, important assumptions and invariants, and non-obvious algorithmic decisions;
  - concurrency guarantees, synchronization requirements, and object lifetime and ownership constraints;
  - intentional performance trade-offs, non-obvious error-handling behavior, and intentional deviations that could make code appear incorrect or unnecessary.
- Comments that merely restate self-explanatory code should be avoided. Complexity alone does not require an explanatory comment; even a simple operation requires one when an important assumption is not evident. Explicitness takes priority over minimizing comments.
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

For example, `// Calculate the connection timeout.` merely repeats `CalculateConnectionTimeout(retryCount)`. If the call does not make an important backoff constraint evident, explain it instead:

```cpp
// Exponential backoff is capped to avoid excessive reconnection delays.
const auto timeout{ CalculateConnectionTimeout(retryCount) };
```

## Function

### Declaration, definition, and call

- Use FORCE_INLINE for all declarations and definitions.
- Use `[[nodiscard]]` for all declarations and definitions of functions that return a value.
- Use `noexcept` for all declarations and definitions of functions that do not throw exceptions. Make sure that internal calls do not throw exceptions as well or make sure that exceptions are caught and handled inside the function.
- Avoid default values for function parameters and template parameters. Require callers to provide them explicitly so behavior remains visible at each call site. An intrinsic, unambiguous default that does not hide a behavioral choice is allowed, such as the standard comparison epsilon selected from a floating-point type.
- Function and template arguments must make the intended behavior understandable; follow [Function arguments and literal values](#function-arguments-and-literal-values).
- Arguments passed by value are declared without `const` qualifier, as it is not a part of the function signature. Definition adds `const` to the argument if it is not modified in the function body.

```cpp
FORCE_INLINE void SetPort(uint16_t port) noexcept;

FORCE_INLINE void Server::SetPort(const uint16_t port) noexcept { m_port = port; }
```

### Function arguments and literal values

- Ambiguous arguments must be made understandable through meaningful identifiers, strongly typed enumerations, named constants, intermediate variables, or explanatory comments. If the code does not otherwise make their semantics sufficiently explicit, a comment is required. A concise comment is sufficient for an existing API; do not require redesign solely to clarify a call.
- Constants whose meaning is not evident from the API or argument identifiers at a function or template call must use inline argument tags in the form `/*tag=*/value`, e.g. `OpenConnection(ip, port, /*doReconnection=*/false)` or `SaveStr<OVERWRITE, /*Mode=*/0644>()`. A preceding or nearby explanatory comment does not replace required inline argument tags.
- Boolean literals require inline argument tags when their meaning is ambiguous. Do not add redundant comments when the API or surrounding code already makes their meaning evident, e.g. `SetSaved(true)`.
- Avoid unexplained numeric literals representing timeouts, protocol constants, limits, thresholds, units, or implementation assumptions. Prefer named constants or inline argument tags; unexplained meaning must still be clarified. Not every numeric literal is a magic value: common, self-explanatory values and clear calls such as `SetPort(8080)` do not require extra documentation.

```cpp
Connect(server, /*enableTLS=*/true, /*autoReconnect=*/false, /*timeoutSec=*/30);
```

### Body

- Use `[[likely]]` and `[[unlikely]]` for all branches that are likely or unlikely to be taken, respectively.
- Use `[[maybe_unused]]` for all variables that are not used in the function body.
- Use `[[indeterminate]]` for all local variables that can be uninitialized at the point of declaration.
- `goto` operator is prohibited. Control flow is expressed by structured statements: loops, `break`/`continue` and early `return`.

### Lambda

- Capture defaults `[&]` and `[=]` are prohibited. Each captured variable is listed explicitly, by reference or by value, so it is visible what the lambda depends on and how, e.g. dangling reference or unintended copy is noticeable at the place of capture.

## Type

### Basic data type

- Use `int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` instead of build int integer types.
- Use basic data types instead of aliases, like ptrdiff_t, size_t, ssize_t, etc.

### User defined type

- Do not loose scope of user defined types, like class, struct, enum, etc. Use `std::` or `MSAPI::` prefix when using them outside of their namespace. If class is derived from another class, use `BaseClass::` prefix when accessing its data or functions.
- If the base class name is long or templated, define a type alias for it and access inherited data and functions via the alias, e.g. `BaseT::m_path`. For a template base class the prefix is also required, as names of a dependent base are not found without it.

```cpp
template <template <typename> typename Container, typename Type>
class Single : public Base<Single<Container, Type>> {
    using BaseT = Base<Single<Container, Type>>;

    FORCE_INLINE void Clear() noexcept
    {
        const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ BaseT::m_lock };
        BaseT::m_timestamp = MSAPI::Timer{};
    }
};
```

#### Declaration

- Use `class` for all class declarations, public fields are prohibited. Protected fields are allowed only in a base class designed to share its state with derived classes, e.g. CRTP base, and derived classes access them via the base type alias, e.g. `BaseT::m_path`.
- `struct` is kept where the standard library convention requires it: specializations of standard templates declared as `struct` (`std::hash`, `std::formatter`), allocator `rebind` and metaprogramming type traits, e.g. `template <typename T> struct is_optional : std::false_type {};`.
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
    FORCE_INLINE Connection(Connection&&) noexcept = default;
    Connection& operator=(const Connection&) = delete;
    FORCE_INLINE Connection& operator=(Connection&&) noexcept = default;
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

- Use brace initialization for all variables and fields.
- Types without a user-defined constructor (basic data types, aggregates) initialized by default value use empty braces `{}`, including static fields, not explicit default value: `uint64_t m_size{};` instead of `uint64_t m_size{ 0 };`, `bool m_isSaved{};` instead of `bool m_isSaved{ false };`, `int8_t* m_buffer{};` instead of `int8_t* m_buffer{ nullptr };`.
- Use `{ value }` for all initializations of types where no specific std::initializer_list construction exists.

## Macros

- `#define` macros are avoided. Functions, lambdas and templates are used instead to keep code readable. Macros take place only in very specific cases.
- Lambda does not lose in performance to macro. Its call operator is implicitly inline and its closure type is unique, so with enabled optimization (`-O2`, `-O3`) a directly called lambda is inlined and the generated code is the same as for an equivalent macro. Without optimization a lambda is called as a function, and in case inlining is required in any build configuration, it is forced by `[[gnu::always_inline]]` attribute, the lambda analog of `FORCE_INLINE`.

```cpp
// Instead of #define SQUARE(x) ((x) * (x))
const auto square{ [] [[gnu::always_inline]] (const uint64_t value) { return value * value; } };
const auto result{ square(size + 1) };
```

## Friendships

- Place friendship declarations at the end of the abstraction, as specified in [Class layout](codeStyle.md#class-layout).
- Use `// Comment` to define the purpose of friendship.
- Use `// Comment` to mark place where direct access via friendship is happening.
- Consider usage of private methods of the class over direct access to its private members, e.g. `object.SetSaved(true)` instead of `object.m_isSaved = true`, so the class keeps control of its state.
