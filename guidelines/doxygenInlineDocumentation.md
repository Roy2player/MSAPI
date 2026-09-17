---
name: Doxygen Inline Documentation
description: "Required inline documentation and comment structure"
---

# Doxygen Inline Documentation

New source files must use the project copyright and license header.

## Common rules

- Order as presented below.
- Section tag is used once per documentation block, except for `@param`, `@tparam`, and `@todo`.
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
- `@concurrency` `Yes` if the abstraction is designed to be used concurrency safely, `No` otherwise.
- `@todo` Description of future work, if any.

## Function

- `@brief` Full description of function behavior, data mutation, and calling flow. For simple getters, only `@return` is required.
- `@attention` Critical information to be highlighted, as behavior on internal exception handling: `On bad allocation pointer will be nullptr.`.
- `@note` Outstanding information, as design choose thoughts, performance measurements and etc.
- `@see` To point that some details can be found in other brief.
- `@tparam` Description of a template parameter.
- `@param` Description of a parameter.
- `@pre` Precondition expectations.
- `@locking` Internal locking behavior in their order and expectations about external locks in case if abstraction is marked as `@concurrency Yes.` or function is out of abstraction scope: `@locking Write lock m_lock inside.`. If internal calls can perform locks that should be noticed: `@locking Perform locking in FunctionName call.`. If function has no locks in its scope, or internal calls and does not expect any external locks, then it should be noticed: `@locking Is not required.`.
- `@return` Description of return value, especially in case of multiple return values.
- `@test Yes.` if the function has tests coverage.
- `@todo` Description of future work, if any. If tests coverage, add `@todo Add tests coverage.`.

## Friend, enum, concept

- `@brief` Description of purpose of friendship.
