---
name: msapi-performance-and-reliability
description: Performance-sensitive and reliability-focused guidance for MSAPI. Use when changing hot paths, memory usage, logging, or low-level server/protocol behavior.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI performance and reliability guide

## Core principles

- MSAPI targets low-latency, high-reliability Linux services.
- Prefer deterministic behavior, localized memory ownership, and minimal allocations in hot code.
- Keep library code exception-free.

## Allocation and layout

- Reuse buffers and contiguous storage where the current implementation already does so.
- Avoid unnecessary `std::string` copies and repeated resizing in protocol-heavy paths.
- Be careful not to mix allocator domains across the same object graph.

## Hot-path guidance

- Server loops should minimize per-connection allocations and avoid blocking work in dispatch handlers.
- Protocol encode/decode paths should validate early and precompute sizes when possible.
- Logging in hot loops should be guarded and formatting work should stay outside critical paths when practical.

## Existing project patterns

- Prefer stack allocation or reuse over transient heap churn in tight loops.
- Use `.inl` and `FORCE_INLINE` the same way the rest of the project does.
- Favor small, safe optimizations that keep the code readable and aligned with current module boundaries.
