---
name: File Organization
description: "Inlinable library layout and structure of .inl files"
---

# File Organization

## Principles

- The library must remain fully inlinable.
- Each module must be declared and defined in one `.inl` file.
- All functions must be declared and defined with `FORCE_INLINE`, see [Code Syntax](codeSyntax.md).
- Static data members and namespace-scope variables must be `inline` (e.g. `static inline constexpr`), so they have one definition across translation units.
- Files must not end with empty lines unless their format requires them.

## Structure of `.inl` file

Parts must follow the order below, separated by an empty line:

1. Doxygen file header, see [Doxygen Documentation](doxygenDocumentation.md#file).
2. Include guard opening, see [Code Style](codeStyle.md#include-guard).
3. Includes, with MSAPI files by relative path first, then standard and system headers.
4. Namespace openings, see [Code Style](codeStyle.md#namespace).
5. Declaration part, marked by the `Declarations` separator, containing concepts, constants, forward declarations and abstractions with their documentation. Functions must only be declared here, except defaulted and deleted special members.
6. Definition part, marked by the `Definitions` separator, containing definitions of all functions declared in the declaration part.
   - Each abstraction or nested abstraction must have its own separator using its qualified name, e.g. `Server::IpLimits`.
   - Namespace-scope free function definitions must be grouped under a `Free` separator rather than separate separators named after individual functions.
   - Submodules must follow the order of their declarations.
7. Namespace closings.
8. Specializations of standard templates (e.g. `std::formatter`, `std::hash`) for the module types, outside of the MSAPI namespace.
9. Include guard closing, `#endif // GUARD_NAME`, which must be the last line of the file.

Each separator must be a block comment with a line of 81 dashes before and after the title.

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

/**************************
 * @param buffer Buffer to inspect.
 *
 * @return True if the buffer contains no data.
 */
FORCE_INLINE [[nodiscard]] bool IsEmpty(const RecvBuffer& buffer) noexcept;

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

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool IsEmpty(const RecvBuffer& buffer) noexcept { return buffer.GetSize() == 0; }

} // namespace MSAPI

template <> struct std::formatter<MSAPI::RecvBuffer> {
    ...
};

#endif // MSAPI_RECV_BUFFER_INL
```
