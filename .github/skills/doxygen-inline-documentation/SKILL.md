---
name: doxygen-inline-documentation
description: "Use when writing or fixing Doxygen documentation in files. Applies a compact, strict guideline for file, abstraction, function, friend, enum, and concept documentation with exact tag ordering and wording rules."
---

# Doxygen Inline Documentation

Use this skill when the task is to add, rewrite, normalize, or review Doxygen comments in MSAPI C++ sources.

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
- `@version` Version of the file library.
- `@date` Date of the file creation.
- `@author` Author of the file.
- `@copyright` Copyright information.
- Legal information, if any.
- `@brief` Short description of the functionality provided by the file.
- `@todo` Description of future work, if any.

## Abstraction

Use for class, struct, and union.

- `@brief` Full description of function behavior, data mutation, and calling flow.
- `@tparam` Description of a template parameter.
- `@concurrency` `True` if the abstraction is designed to be used concurrency safely, `False` otherwise.
- `@todo` Description of future work, if any.

## Function

- `@brief` Full description of function behavior, data mutation, and calling flow. For simple getters, only `@return` is required.
- `@tparam` Description of a template parameter.
- `@param` Description of a parameter.
- `@pre` Expectations in `contract_assert` or another expectations.
- `@locking` Internal locking behavior and expectations about external locks.
- `@blocking` Description of blocking behavior, if any.
- `@return` Description of return value, especially in case of multiple return values.
- `@test Has unit tests.` if the function has unit tests.
- `@todo` Description of future work, if any. If no unit tests, add `@todo Add unit tests.`.

## Friend, enum, concept

- `@brief` Description of purpose of friendship.

## MSAPI usage notes

- Apply the correct block type before editing.
- Keep the stable public contract in the header when `.h` and `.inl` are paired.
- Keep implementation-specific semantics in the `.inl` file.
- Do not duplicate the same contract in both files unless the inline file is the only declaration site.
- Verify section order, blank lines, sentence endings, and required `@test` or `@todo` tags before finishing.