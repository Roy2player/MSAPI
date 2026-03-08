---
name: msapi-server-and-protocols
description: Server loop, application lifecycle, protocol-handling, and server-side common-pattern guidance for MSAPI. Use when changing networking, request dispatch, framing, or parameter flow.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI server and protocol guide

## Server framework

- `server.h` / `server.cpp` own the accept loop, connection reads/writes, and buffer dispatch through `HandleBuffer`.
- Keep `HandleBuffer` non-blocking and lightweight.
- Reuse existing connection and buffer handling patterns before adding new control flow.

## Application lifecycle

- `application.h` / `application.cpp` implement `HandleRunRequest`, `HandlePauseRequest`, and `HandleStopRequest`.
- New application behavior should fit the current state-based lifecycle model.
- Parameter registration and retrieval should preserve the existing non-owning observer semantics.

## Authorization

- `authorization.inl` provides thread-safe account, login, logout, and session handling patterns.
- Extend authorization through the current generic model instead of adding a separate auth stack.

## Protocol rules

- `DataHeader` is the common framing layer. Always validate announced sizes against the received buffer before decoding payloads.
- `standard.*` is the flexible protocol for dynamic key-value messages.
- `object.*` is for trivially copyable object payloads only.
- `http.*` is intentionally minimal; do not assume full RFC coverage.

## Common Patterns

### Server Implementation

- Servers inherit from `MSAPI::Server` and override `HandleBuffer` for custom data handling.
- Keep server customizations consistent with the current dispatch and lifecycle flow.

### Application State Management

- Applications use state-based lifecycle management through `HandleRunRequest`, `HandlePauseRequest`, and related hooks.
- Fit new application states and transitions into the current model instead of building parallel lifecycle abstractions.

## Practical guidance

- Keep protocol changes contiguous and predictable: precompute sizes, reuse buffers, and avoid transient allocations in hot paths.
- When a change touches framing or decoding, add or update focused tests around the exact protocol behavior.
