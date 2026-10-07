---
name: File Organization
description: "Inlinable library layout and structure of .inl files"
---

# File Organization

## Principles

- The library is expected to be fully inlinable. Each module is declared and defined in one `.inl` file.
- All functions are declared and defined with `FORCE_INLINE`, see [Code Syntax](codeSyntax.md).
- Static data members and namespace scope variables are `inline` (e.g. `static inline constexpr`), so they are defined once across all translation units.

## Structure of `.inl` file

Parts follow in the order below, separated by an empty line.

1. Doxygen file header, see [Doxygen Inline Documentation](doxygenInlineDocumentation.md#file).
2. Include guard opening, see [Code Style](codeStyle.md#include-guard).
3. Includes: MSAPI files by relative path first, then standard and system headers.
4. Namespaces opening, see [Code Style](codeStyle.md#namespace).
5. Declaration part, marked by `Declarations` separator. Contains concepts, constants, forward declarations and abstractions with their documentation. Functions are only declared here, except defaulted and deleted special members.
6. Definition part, marked by `Definitions` separator. Contains definitions of all functions declared in the declaration part.
   - Each submodule (abstraction, nested abstraction or group of free functions) is explicitly marked by its own separator with its qualified name, e.g. `Server::IpLimits`.
   - Submodules follow the order of their declarations.
7. Namespaces closing.
8. Specializations of standard templates (e.g. `std::formatter`, `std::hash`) for the module types, which must be defined outside of MSAPI namespace.
9. End of the file: include guard closing, `#endif // GUARD_NAME`, is the last line.

Separator is a block comment with a line of 81 dashes before and after the title.

```cpp
/**************************
 * @file        recvBuffer.inl
 * @date        2026-01-01
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2026 Maksim Andreevich Leonov
 *
 * ... license text ...
 */

#ifndef MSAPI_RECV_BUFFER_INL
#define MSAPI_RECV_BUFFER_INL

#include "../help/lock.inl"
#include <cstdint>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Buffer for received data.
 *
 * @concurrency No.
 */
class RecvBuffer {
public:
    /**************************
     * @brief Accumulated data of the buffer.
     *
     * @concurrency No.
     */
    class Chunk {
    public:
        /**************************
         * @return Size of the chunk.
         */
        FORCE_INLINE [[nodiscard]] uint64_t GetSize() const noexcept;
    };

    /**************************
     * @return Size of the buffer.
     */
    FORCE_INLINE [[nodiscard]] uint64_t GetSize() const noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
RecvBuffer::Chunk
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::Chunk::GetSize() const noexcept { return 0; }

/*---------------------------------------------------------------------------------
RecvBuffer
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::GetSize() const noexcept { return 0; }

} // namespace MSAPI

template <> struct std::formatter<MSAPI::RecvBuffer> {
    ...
};

#endif // MSAPI_RECV_BUFFER_INL
```
