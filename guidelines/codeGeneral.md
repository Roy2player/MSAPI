---
name: Code General
description: "General engineering rules"
---

# Code General

## Obvious caller check rule

Caller must check obvious miss conditions on before call, like: not null pointer validation.

## Container inserting

If inserting operation return composite result as element and status, status must be checked and error path is handled.

## Error Contracts

- Do not throw exceptions from MSAPI library code.
- Use the established boolean, optional, status, default-value, and logging patterns to report failure.
- Check and preserve `errno` immediately after a failed POSIX operation when it is relevant to the failure report.
- Distinguish malformed peer input, recoverable operation failure, invariant violation, and connection or process shutdown. Each path must have a clear caller-visible outcome.
- Do not change public behavior or ABI unless the task explicitly permits it.

## Application Data

- Keep `Application` parameters non-owning. The field that owns a value retains its lifetime; parameters observe it.
- Prefer value semantics for small values and `std::optional<T>` when an empty value is not valid.
