---
name: Code General
description: "General engineering rules"
---

# Code General

## Requirement severity and conflicts

This section is the authoritative severity model for MSAPI Engineering Guidelines.

- `must` and `must not` express mandatory requirements.
- `should` and `should not` identify strong recommendations permitting exceptions supported by a concrete engineering rationale.
- Preferences, expressed through wording such as "is preferred" or "prefer X over Y", favor one otherwise acceptable alternative. Other alternatives are permitted without a formal exception. Preferences must not override correctness, mandatory requirements, or stronger recommendations.
- `may` expresses an optional practice or explicitly permitted alternative. It establishes neither an obligation nor a preferred implementation.

## Explicit behavior

- MSAPI code must make its behavior explicit to the engineer reading it.
- Important behavior, assumptions, invariants, and intentional trade-offs must be evident from the code or its documentation, without guessing argument meanings, relying on hidden defaults, or investigating unrelated implementation details.
- Code must use meaningful identifiers, explicit arguments, appropriately strong types and enumerations, named constants, and comments as needed to explain information it does not readily convey. See [Function arguments and literal values](codeSyntax.md#function-arguments-and-literal-values) and [Comments](codeSyntax.md#comments).

## Obvious caller check rule

The caller must check obvious failure conditions before a call, such as validating that a required pointer is not null.

## Container inserting

If an insertion operation returns a composite result containing an element and a status, the caller must check the status and handle the error path.

## Error Contracts

- MSAPI library code must not throw exceptions itself and must handle all exceptions within its scope, must log report such failures and may return an error indication to the caller.
- Failures must be reported through the established boolean, optional, status, default-value, and logging patterns.
- Code must check and preserve `errno` immediately after a failed POSIX operation when it is relevant to the failure report.
- Error handling must distinguish malformed peer input, recoverable operation failure, invariant violation, and connection or process shutdown.
- Each path must provide a clear caller-visible outcome.
- Public behavior or ABI must not change unless the task explicitly permits it.
