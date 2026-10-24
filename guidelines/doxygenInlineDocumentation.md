---
name: Doxygen Inline Documentation
description: "Required inline documentation and comment structure"
---

# Doxygen Inline Documentation

New source files must use the project copyright and license header.

## Common rules

- Documentation sections must follow the order presented below.
- A section tag must appear at most once per documentation block, except for `@param`, `@tparam`, and `@todo`.
- Problems, technical debt and open questions must be recorded in separate `@todo` entries in the documentation block of the related file, abstraction or function.
- An inline `// TODO: comment` may duplicate an existing `@todo` to mark the place for further work. It must not appear without the corresponding `@todo`, which remains the source of truth.
- Sections must be separated by an empty line.
- Sentences must end with a period.
- Documentation must use active voice, present tense, and third person.

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

- `@brief`: Full description of function behavior, data mutation, and calling flow.
- `@attention`: Critical information, such as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note`: Outstanding information, such as design choices or performance measurements.
- `@see`: Reference to details in another brief.
- `@tparam`: Description of a template parameter.
- `@concurrency`: Synchronization support and guarantees of the abstraction.
- `@purging`: Purging behavior for an abstraction that accumulates resources in internal storage and has a mechanism to release them.
- `@todo`: Description of future work, if any.

When `@concurrency` is present, it must use `Yes` only if the abstraction provides its own synchronization mechanisms (e.g. internal locks or atomics) to manage concurrent access to itself. An entry using `Yes` must describe the guarantees. In all other cases, the entry must use `No`, including for abstractions without setters or other modifying methods.

## Function

Documentation for functions must follow the function format below. Simple getters must include `@return` without `@brief`.

- `@brief`: Full description of function behavior, data mutation, and calling flow.
- `@attention`: Critical information, such as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note`: Outstanding information, such as design choices or performance measurements.
- `@see`: Reference to details in another brief.
- `@tparam`: Description of a template parameter.
- `@param`: Description of a parameter.
- `@pre`: Precondition expectations.
- `@locking`: Locking behavior within the module and expectations about external locks, see [Locking documentation scope](#locking-documentation-scope).
- `@return`: Description of the return value, including the possible outcomes when there is more than one.
- `@test`: Test-coverage indication, written as `@test Yes.` for a covered function.
- `@todo`: Description of future work, if any. If function does not yet have test coverage, it should indicate that: `@todo Add tests coverage.`

The `@locking` tag must not be declared for non static members of a class with `@concurrency No`.

## Locking documentation scope

- Locking documentation must describe locking performed with locks owned by the documented abstraction or by other abstractions in the same module, including locking performed through calls within that module.
- It must identify the locks, read/write modes where applicable, acquisition order, and caller requirements for external locking. Examples include `@locking Write lock m_lock inside.` and `@locking Performs read locking of Entry::m_lock in Entry::Read call.`.
- It must not enumerate nested locking chains inside other modules. Those modules document their own internal locking.
- Callers must still document any external-lock requirements imposed on them.
- When no locking within this scope occurs and the caller has no external-lock requirements, the entry must use `@locking Is not required.`, even if called modules perform their own internal locking.

This scope makes the module's actual locking behavior explicit without duplicating implementation details from its dependencies.

## Enum

Documentation for enums must follow the format below.

- `@brief`: Description of the purpose of the enum.

## Friend and concept

Documentation for friends and concepts must use `// comment` style instead of Doxygen tags and describe their purpose clearly.
