---
name: msapi-performance-and-reliability
description: Performance-sensitive and reliability-focused guidance for MSAPI. Use when changing hot paths, memory usage, logging, or low-level server/protocol behavior.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI performance and reliability guide

## Performance Recommendations

- Prefer continuous or contiguous-memory allocators for frequently accessed objects where the current design already uses them.
- MSAPI library code must never throw exceptions.
- Keep `.inl` files in the project pattern where declarations come before inline implementations.
- Use `FORCE_INLINE` in the same cases as the rest of the repository.

### Server Loop

- Minimize per-connection dynamic allocations and reuse buffers.
- Batch reads when possible and avoid per-byte processing.
- Keep the hot path predictable and non-blocking.

### Protocol Encode/Decode

- Prefer contiguous layouts and validate `DataHeader` as early as possible.
- Precompute total encoded sizes before writing to avoid repeated resizes.
- Be conscious of copies and cache behavior in protocol-heavy code.

### Logging

- Avoid logging in hot loops unless a runtime guard makes it safe.
- Move expensive formatting outside critical paths when practical.
- Keep performance optimizations clear and maintainable.
