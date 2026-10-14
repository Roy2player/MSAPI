---
name: Doxygen Inline Documentation
description: "Required inline documentation and comment structure"
---

# Doxygen Inline Documentation

New source files must use the project copyright and license header.

## Common rules

- Order as presented below.
- Section tag is used once per documentation block, except for `@param`, `@tparam`, and `@todo`.
- `@todo` is the only place to explicitly define problems, technical debt and open questions. Each of them is described by a separate `@todo` in the documentation block of the related file, abstraction or function. For further simplicity `// TODO: comment` can duplicate the `@todo` in code to explicitly mark the place for further work, but inline `// TODO` without the corresponding `@todo` is prohibited.
- Empty line between sections.
- Dots at the end of sentences.
- Use active voice.
- Use present tense.
- Use third person.

## File

- `@file` Filename.
- `@date` Date of the file creation.
- `@author` Author of the file.
- `@copyright` Copyright information.
- Legal information, if any.
- `@brief` Short description of the functionality provided by the file.
- `@note` Outstanding information, as design choose thoughts, performance measurements and etc.
- `@todo` Description of future work, if any.

## Abstraction

Use for class, struct, and union.

- `@brief` Full description of function behavior, data mutation, and calling flow.
- `@attention` Critical information to be highlighted, as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note` Outstanding information, as design choose thoughts, performance measurements and etc.
- `@see` To point that some details can be found in other brief.
- `@tparam` Description of a template parameter.
- `@concurrency` `Yes` only if the abstraction provides its own synchronization mechanisms (e.g. internal locks or atomics) to manage concurrent access to itself, with description of the guarantees. `No` in all other cases, including abstractions without setters or other modifying methods.
- `@purging` In case the abstraction accumulates resources in internal storage and has a mechanism to release them, describe the purging behavior.
- `@todo` Description of future work, if any.

## Function

- `@brief` Full description of function behavior, data mutation, and calling flow. For simple getters, only `@return` is required.
- `@attention` Critical information to be highlighted, as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note` Outstanding information, as design choose thoughts, performance measurements and etc.
- `@see` To point that some details can be found in other brief.
- `@tparam` Description of a template parameter.
- `@param` Description of a parameter.
- `@pre` Precondition expectations.
- `@locking` Locking behavior within the module and expectations about external locks, see [Locking documentation scope](#locking-documentation-scope).
- `@return` Description of return value, especially in case of multiple return values.
- `@test Yes.` if the function has tests coverage.
- `@todo` Description of future work, if any. If tests coverage, add `@todo Add tests coverage.`.

## Locking documentation scope

- Describe locking performed with locks owned by the documented abstraction or by other abstractions in the same module, including locking performed through calls within that module.
- Identify the locks, read/write modes where applicable, acquisition order and caller requirements for external locking. For example: `@locking Write lock m_lock inside.` or `@locking Performs read locking of Entry::m_lock in Entry::Read call.`.
- Do not enumerate nested locking chains inside other modules. Those modules document their own internal locking; callers still document any external-lock requirements imposed on them.
- Use `@locking Is not required.` when no locking within this scope occurs and the caller has no external-lock requirements, even if called modules perform their own internal locking.
- This scope keeps the module's actual locking behavior explicit without duplicating implementation details from its dependencies.

## Friend, enum, concept

- `@brief` Description of purpose of friendship.
