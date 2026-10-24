---
name: Code General
description: "General engineering rules"
---

# Code General

## Requirement severity and conflicts

- `MUST` / `MUST NOT` identify mandatory requirements. Direct instructions such as "Use", "Do not", and "is prohibited" are also mandatory unless explicitly qualified.
- `SHOULD` / `SHOULD NOT` identify strong recommendations permitting exceptions supported by a concrete engineering rationale.
- `MAY` identifies an optional practice. `PREFER` identifies a preferred approach when multiple valid implementations exist.
- If existing wording leaves severity uncertain, preserve the requirement and report the ambiguity for maintainer review.
- Apply the more specific rule within its applicable context while preserving correctness, safety, and explicit behavior. If requirements cannot be reconciled without changing engineering policy, report the conflict rather than silently relaxing a requirement or inventing an exception.

## Explicit behavior

- MSAPI code must make its behavior explicit to the engineer reading it. Important behavior, assumptions, invariants, and intentional trade-offs must be evident from the code or its documentation, without guessing argument meanings, relying on hidden defaults, or investigating unrelated implementation details.
- Use meaningful identifiers, explicit arguments, appropriately strong types and enumerations, named constants, and comments explaining information the code does not readily convey. See [Function arguments and literal values](codeSyntax.md#function-arguments-and-literal-values) and [Comments](codeSyntax.md#comments).

## Obvious caller check rule

The caller must check obvious failure conditions before a call, such as validating that a required pointer is not null.

## Container inserting

If an insertion operation returns a composite result containing an element and a status, the status must be checked and the error path must be handled.

## Error Contracts

- Do not throw exceptions from MSAPI library code.
- Use the established boolean, optional, status, default-value, and logging patterns to report failure.
- Check and preserve `errno` immediately after a failed POSIX operation when it is relevant to the failure report.
- Distinguish malformed peer input, recoverable operation failure, invariant violation, and connection or process shutdown. Each path must have a clear caller-visible outcome.
- Do not change public behavior or ABI unless the task explicitly permits it.

## Application Data

- Keep `Application` parameters non-owning. The field that owns a value retains its lifetime; parameters observe it.
- Prefer value semantics for small values and `std::optional<T>` when an empty value is not valid.
