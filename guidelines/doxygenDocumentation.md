---
name: Doxygen Documentation
description: "Required documentation structure, content, and conventions"
---

# Doxygen Documentation

New source files must use the project copyright and license header.

## Common rules

- Documentation sections must follow the order presented below.
- A section tag must appear at most once per documentation block, except for `@param`, `@tparam`, and `@todo`.
- Problems, technical debt, and open questions must be recorded in separate `@todo` entries in the documentation block of the related file, abstraction, or function.
- An inline `// TODO: Comment.` may duplicate an existing `@todo` to mark the place for further work. It must not appear without the corresponding `@todo`, which remains the source of truth.
- Sections must be separated by an empty line.
- Sentences must end with a period.
- Documentation must use active voice, present tense, and third person.
- Documentation must include the tags listed for the corresponding element, except where their applicability is explicitly restricted by a condition or another rule.
- Tag content must comply with the corresponding descriptions and applicable requirements.

## File

File documentation must include applicable legal information and follow the structure outlined below.

- `@file`: Filename.
- `@date`: Date of file creation.
- `@author`: Author of the file.
- `@copyright`: Copyright information.
- Legal information, if any.
- `@brief`: Short description of the functionality provided by the file.
- `@note`: Outstanding information, such as design choices or performance measurements.
- `@todo`: Description of future work, if any.

## Abstraction

Documentation for classes, structs, and unions must follow the abstraction format below.

- `@brief`: Description of the abstraction's purpose, responsibilities, and behavior.
- `@attention`: Critical information, if any, such as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note`: Outstanding information, if any, such as design choices or performance measurements.
- `@see`: Reference to details in another brief, if any.
- `@tparam`: Description of a template parameter.
- `@concurrency`: Synchronization support flag.
- `@purging`: Purging behavior for an abstraction that accumulates resources in internal storage and has a mechanism to release them.
- `@todo`: Description of future work, if any.

The `@concurrency` tag must contain only `Yes.` or `No.`, with no additional prose or continuation lines. Use `Yes.` only if the abstraction provides its own synchronization mechanisms (e.g. internal locks or atomics) to manage concurrent access to itself. In all other cases, use `No.`, including for abstractions without setters or other modifying methods.

A file- or abstraction-level `@note` may briefly describe the architectural concurrency model and its design trade-offs. Keep this model description out of method documentation.

Method documentation must be the single source of operational concurrency contracts. Document guarantees during access, access lifetime requirements, and operation-specific synchronization rationale on the relevant methods, not in abstraction-level `@brief` or `@note`. Use method-level `@brief` or `@note` for these guarantees and rationale, `@pre` for caller requirements, and `@locking` for locking modes, acquisition order, and required caller locking, following [Locking documentation scope](#locking-documentation-scope).

## Function

Documentation for functions must follow the function format below. Simple getters must include `@return` without `@brief`.

- `@brief`: Description of the operation, its effects on data, and calling flow. Details assigned to dedicated tags below belong in those tags.
- `@attention`: Critical information, if any, such as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note`: Outstanding information, if any, such as design choices or performance measurements.
- `@see`: Reference to details in another brief, if any.
- `@tparam`: Description of a template parameter.
- `@param`: Description of a parameter.
- `@pre`: Preconditions, if any, that callers must satisfy.
- `@locking`: Locking behavior within the module and expectations about external locks; see [Locking documentation scope](#locking-documentation-scope).
- `@return`: Meaning of the returned value and the conditions selecting each possible outcome.
- `@test`: Test-coverage indication, written as `@test Yes.` for a covered function. The tag must be omitted when the function has no test coverage.
- `@todo`: Description of future work, if any. If a function does not yet have test coverage, it should indicate that with `@todo Add tests coverage.`.

The `@return` tag must contain only return-value semantics and conditions that distinguish outcomes. It must not repeat operational behavior, side effects, preconditions, or locking details from other tags. For example, cleanup that completes even after an I/O failure belongs in `@brief` or `@note`; `@return` states which value reports that failure. Conditions needed to distinguish returned values must remain explicit, even when they refer to the operation described in `@brief`.

When a wrapper returns another function's result unchanged, its `@return` may refer to that function's documented return contract instead of repeating the outcome descriptions.

The `@locking` tag must not be used for non-static member functions of a class documented with `@concurrency No.`.

## Locking documentation scope

- Locking documentation must describe locking performed with locks owned by the documented abstraction or by other abstractions in the same module, including locking performed through calls within that module.
- It must identify the locks, read/write modes where applicable, acquisition order, and caller requirements for external locking. Examples include `@locking Write lock m_lock inside.` and `@locking Performs read locking of Entry::m_lock in Entry::Read call.`.
- It must not enumerate nested locking chains inside other modules. Those modules document their own internal locking.
- Any external-lock requirements imposed on callers must still be documented.
- When no locking within this scope occurs and the caller has no external-lock requirements, the entry must use `@locking Is not required.`, even if called modules perform their own internal locking.

This scope makes the module's actual locking behavior explicit without duplicating implementation details from its dependencies.

## Enum

Documentation for enums must follow the format below.

- `@brief`: Description of the enum's overall purpose. It should not describe individual enumerators.

Enumerator names should be self-descriptive. Use `// Comment.` only when an enumerator has non-obvious details to explain; avoid comments that repeat the meaning already conveyed by its name.

## Friend and concept

Documentation for friends and concepts must use `// Comment.` style instead of Doxygen tags and describe their purpose clearly.
