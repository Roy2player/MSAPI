---
name: Code Syntax
description: "Syntax and declaration rules"
---

# Code Syntax

## Comments

- Comments must explain information that cannot be readily inferred from the code itself. When the code does not make the following information sufficiently explicit, comments must explain it:
  - Non-obvious function or template arguments, including ambiguous boolean values and unexplained magic values.
  - Implementation intent, important assumptions and invariants, and non-obvious algorithmic decisions.
  - Concurrency guarantees, synchronization requirements, and object lifetime and ownership constraints.
  - Intentional performance trade-offs, non-obvious error-handling behavior, and intentional deviations that could make code appear incorrect or unnecessary.
- Comments that merely restate self-explanatory code should be avoided. Complexity alone does not require an explanatory comment.
- Even a simple operation must have a comment when an important assumption is not evident. Explicitness must take priority over minimizing comments.
- Comments must use `// Comment.` style: `//`, one space, then the text. A multiline comment must be a sequence of `//` lines, with `//` for an empty line inside it.
- Markers immediately after `//`, such as `//*`, must not be used.
- Apart from the documentation blocks and separators below, `/* */` may be used for the following purposes and must not be used elsewhere:
  - A short note within code, such as an argument tag `/*tag=*/value`, a parameter note ` /* by value as moved */ `, or a note inside an expression or initializer list.
  - A comment inside a multiline macro definition, where `//` would comment out the line continuation `\`.
- Doxygen documentation blocks must follow [Doxygen Documentation](doxygenDocumentation.md). Section separators must follow [File Organization](fileOrganization.md#structure-of-inl-file).
- Other comment types must not be used, including markers (`//*`, `//!`, `//?`) and task comments (`// FIXME`, etc.).
- `// TODO: Comment.` may duplicate an existing `@todo` to mark the place for further work. It must not appear without the corresponding `@todo`, which remains the source of truth.
- Problems, technical debt and open questions must be described by `@todo` in the documentation block of the related function or abstraction, see [Doxygen Documentation](doxygenDocumentation.md).

```cpp
// Retry is required, as connection can be closed by peer during handshake and new attempt is made with the same id.
const auto connection{ OpenConnection(ip, port, /*doReconnection=*/false) };
```

For example, a comment such as `// Check whether the connection is already closed.` would merely describe the condition. The existing comment explains the descriptor-reuse hazard that makes the guard necessary:

```cpp
// Only the first caller closes the descriptor, otherwise its number can be reused and closed by mistake.
if (m_isClosed.exchange(true, std::memory_order_acq_rel)) {
    return;
}
```

## Function

### Declaration, definition, and call

- All declarations and definitions must use FORCE_INLINE except deleted functions.
- All declarations and definitions of functions that return a value must use `[[nodiscard]]`.
- All declarations and definitions of functions that do not throw exceptions must use `noexcept`. Internal calls must either not throw or have their exceptions caught and handled inside the function.
- Default values should be avoided for function and template parameters, and callers should supply arguments explicitly so behavior remains visible at each call site.
- An intrinsic, unambiguous default that does not hide a behavioral choice may be used, such as the standard comparison epsilon selected from a floating-point type.
- Function and template arguments must make the intended behavior understandable and follow [Function arguments and literal values](#function-arguments-and-literal-values).
- Arguments passed by value must be declared without `const`, as it is not part of the function signature. The definition must add `const` if the argument is not modified in the function body.

```cpp
FORCE_INLINE void SetPort(uint16_t port) noexcept;

FORCE_INLINE void Server::SetPort(const uint16_t port) noexcept { m_port = port; }
```

### Function arguments and literal values

- Ambiguous arguments must be made understandable through meaningful identifiers, strongly typed enumerations, named constants, intermediate variables, or explanatory comments.
- If the code does not otherwise make argument semantics sufficiently explicit, a comment must be added. A concise comment is sufficient for an existing API; reviewers must not require redesign solely to clarify a call.
- Constants whose meaning is not evident from the API or argument identifiers at a function or template call must use inline argument tags in the form `/*tag=*/value`, e.g. `OpenConnection(ip, port, /*doReconnection=*/false)` or `SaveStr<OVERWRITE, /*Mode=*/0644>()`. A preceding or nearby explanatory comment does not replace required inline tags.
- Boolean literals must have inline argument tags when their meaning is ambiguous. Redundant comments must not be added when the API or surrounding code already makes their meaning evident, e.g. `SetSaved(true)`.
- The meaning of unexplained numeric literals representing timeouts, protocol constants, limits, thresholds, units, or implementation assumptions must be clarified.
- Named constants or inline argument tags are preferred. Not every numeric literal is a magic value: common, self-explanatory values and clear calls such as `SetPort(8080)` do not require extra documentation.

```cpp
Connect(server, /*enableTLS=*/true, /*autoReconnect=*/false, /*timeoutSec=*/30);
```

### Body

- Branches that are likely or unlikely to be taken should use `[[likely]]` or `[[unlikely]]`, respectively.
- Variables that are not used in the function body must use `[[maybe_unused]]`.
- Local variables that can be uninitialized at the point of declaration must use `[[indeterminate]]`.
- The `goto` operator must not be used. Control flow must use structured statements: loops, `break`/`continue` and early `return`.

### Lambda

- Capture defaults `[&]` and `[=]` must not be used.
- Each captured variable must be listed explicitly, by reference or by value, so the lambda's dependencies are visible and dangling references or unintended copies can be noticed at the point of capture.

## Type

### Basic data type

- Code must use `int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t` instead of built-in integer types.
- Basic data types must be used instead of aliases such as ptrdiff_t, size_t, and ssize_t if it does not break portability between different architectures.

### User defined type

- User-defined types such as classes, structs, and enums must retain their scope through the `std::` or `MSAPI::` prefix when used outside their namespace.
- Access to inherited data or functions in a derived class must use the `BaseClass::` prefix.
- A long or templated base class name must have a type alias, and inherited data and functions must be accessed through that alias, e.g. `BaseT::m_path`. The prefix also supports name lookup in a dependent base class.

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

- Class declarations must use `class`, except for the standard-library-convention `struct` cases below.
- Public fields must not be declared.
- Protected fields may be used only in a base class designed to share state with derived classes, e.g. a CRTP base. Derived classes must access those fields through the base type alias, e.g. `BaseT::m_path`.
- `struct` must be retained where the standard library convention requires it: specializations of standard templates declared as `struct` (`std::hash`, `std::formatter`), allocator `rebind`, and metaprogramming type traits, e.g. `template <typename T> struct is_optional : std::false_type {};`.
- All parameterized constructors must be declared `explicit`, except copy, move, deleted and default constructors.
- The copy constructor, move constructor, copy assignment, and move assignment operators must all be explicitly declared as `= default` or `= delete`. Their implicit generation depends on other members: a user-declared destructor suppresses implicit move operations, while a lock or `std::unique_ptr` field makes copying deleted. Explicit declarations document the intent and let the compiler check it.
- The destructor and default constructor may be left implicit under the following conditions:
  - A destructor must be declared only if it has behavior (e.g. releases a resource) or the class is a polymorphic base.
  - A polymorphic base must declare a virtual destructor, e.g. `virtual ~Base() = default;`.
  - The default constructor must be declared as `= default` if default construction is required together with other constructors, since declaring another constructor suppresses the implicit one. Its declaration may be omitted in other cases.

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

- `mutable` must be restricted to lock fields, see [Concurrency](concurrency.md#lock-fields-and-constness).

#### Enum

- Enums must use `enum class` with an explicit, appropriately sized integer underlying type.
- Enums should declare `Undefined` as the first enumerator to explicitly represent an empty or uninitialized state, and `Max` as the last enumerator to represent the exclusive upper bound of valid values.
- Documented sentinel values such as `Undefined` and `Max` must be preserved.
- Each enum should provide an `EnumToString` overload in its namespace, or a static member overload in its owning class for a nested enum.
- When provided, the overload must use the signature `FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(EnumType value) noexcept`. A member overload must also be declared `static`. The declaration must pass the enum by value without `const`; the definition must add `const`, following [Declaration, definition, and call](#declaration-definition-and-call).
- The implementation must use a `switch` with an explicit case for every enumerator, including any sentinels, and return string literals through `std::string_view`. Cases for `Undefined` and `Max` must return `"Undefined"` and `"Max"`, respectively.
- When the enum has a `Max` sentinel, the function must contain a `static_assert` comparing `U(EnumType::Max)` with its expected numeric value to flag changes requiring a coverage review. Explicitly numbered or sparse enums still require checking every enumerator because additions may leave `Max` unchanged.
- The `default` case must log the unrecognized numeric value using `LOG_ERROR_NEW` and `U(value)`, following [Logging](logging.md), then return `"Unknown"`.
- `EnumToString` switches, their `static_assert` coverage checks, and unknown-value logging must stay synchronized whenever an enum changes.

```cpp
enum class State : int8_t { Undefined, Ready, Max };

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(State value) noexcept;

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const State value) noexcept
{
    static_assert(U(State::Max) == 2, "State::Max has changed, update EnumToString");

    switch (value) {
    case State::Undefined:
        return "Undefined";
    case State::Ready:
        return "Ready";
    case State::Max:
        return "Max";
    default:
        LOG_ERROR_NEW("Unknown state: {}", U(value));
        return "Unknown";
    }
}
```

#### Overriding

- Overridden methods must be grouped by parent. Each group must have a `// Parent` comment containing the fully qualified name of the parent whose methods are overridden, e.g. `// MSAPI::Server`.
- An override that is not expected to be overridden further must be marked `final`.
- An override that is expected to be overridden further must be marked `override`.
- `virtual` must not be repeated on overrides.

```cpp
class Manager : public MSAPI::Server, public MSAPI::Protocol::HTTP::IHandler {
public:
    // MSAPI::Server
    FORCE_INLINE void HandleBuffer(MSAPI::RecvBuffer& recvBuffer) final;
    // MSAPI::Application
    FORCE_INLINE void HandleRunRequest() override;
    FORCE_INLINE void HandlePauseRequest() final;
    // MSAPI::Protocol::HTTP::IHandler
    FORCE_INLINE void HandleHttp(const std::shared_ptr<MSAPI::Connection::Data>& connectionData,
        const MSAPI::Protocol::HTTP::Data& data) final;
};
```

## Type aliases

- Type aliases must use `using` instead of `typedef`.
- Type aliases may be declared at namespace, class, or function scope, but not in the global namespace.
- Type alias names must follow the `CamelCaseT` naming convention to distinguish them from other types.

## Variable

- Variables and fields must use brace initialization.
- Types without a user-defined constructor (basic data types, aggregates) initialized to their default value must use empty braces `{}`, including static fields, rather than an explicit default value: `uint64_t m_size{};` instead of `uint64_t m_size{ 0 };`, `bool m_isSaved{};` instead of `bool m_isSaved{ false };`, `int8_t* m_buffer{};` instead of `int8_t* m_buffer{ nullptr };`.
- Initializations of types with no specific `std::initializer_list` construction must use `{ value }`.

## Macros

- `#define` macros should be avoided.
- Functions, lambdas and templates are preferred for readability.
- Macros may be used in specific cases where those alternatives are unsuitable.

A lambda's call operator is implicitly inline, and its closure type is unique. With optimization (`-O2`, `-O3`), a directly called lambda can be inlined to produce the same code as an equivalent macro. Without optimization, a lambda is called as a function; `[[gnu::always_inline]]` is the lambda equivalent of `FORCE_INLINE` for forcing inlining in any build configuration.

```cpp
// Instead of #define SQUARE(x) ((x) * (x))
const auto square{ [] [[gnu::always_inline]] (const uint64_t value) { return value * value; } };
const auto result{ square(size + 1) };
```

## Friendships

- Access decisions must distinguish a type's declaration visibility from access to the owning abstraction's stored instances. Check whether the public API exposes those instances through references, pointers, containers, or callbacks.
- When a helper's stored instances remain internal, its ordinary operations should be public methods. Friendship should not be introduced or retained solely to make those methods private.
- When public API callers can access an instance, operations whose external invocation could violate lifecycle constraints or invariants must not be public. Use private methods with friendship for designated collaborators, or protected methods for an intentional inheritance interface. Restricting access between internal collaborators may also justify private methods and friendship when it protects a concrete invariant.
- Friendship declarations must appear at the end of the abstraction, as specified in [Class layout](codeStyle.md#class-layout).
- A `// Comment` must explain the purpose of a friendship.
- A `// Comment` must mark each place where direct access through friendship occurs.
- Private methods are preferred over direct access to private members, e.g. `object.SetSaved(true)` instead of `object.m_isSaved = true`, so the class retains control of its state.
