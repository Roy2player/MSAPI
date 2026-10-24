# MSAPI Agent Instructions

This file is the repository-wide entry point for coding and code-review agents.
It routes agents to the detailed MSAPI guidelines.

## Architecture

- Use `README.md` as a source of truth for the repository project structure.

## Required Workflow

- Before changing MSAPI code, identify the applicable guidelines below and read them before editing.

## Guideline Map

- [Code General](guidelines/codeGeneral.md): General guidelines.
- [Code Style](guidelines/codeStyle.md): Naming, source layout, and formatting. Applies to every code change.
- [Code Syntax](guidelines/codeSyntax.md): Declarations, attributes, types, initialization, calls, and friendship. Applies to C++ code changes.
- [Concurrency](guidelines/concurrency.md): Lock ownership and scope, shared data, lifetime across threads, and atomic state. Applies when execution can overlap across threads or callbacks.
- [Doxygen Inline Documentation](guidelines/doxygenInlineDocumentation.md): Documentation block structure and Doxygen tags. Applies when adding or modifying public APIs, abstractions, or inline documentation.
- [File Organization](guidelines/fileOrganization.md): Inlinable library layout and structure of .inl files.
- [Logging](guidelines/logging.md): Logging macro selection, level semantics, message grammar, and value formatting. Applies when adding or modifying logs.

## Code Review Requirements

For every pull request or code review:

- Determine which guidelines apply to the changed code.
- Verify the changed code follows each applicable guideline.
- Report each material violation with the relevant guideline link and a concise, concrete explanation.
- Verify concurrency, ownership, protocol parsing, error handling, logging, and documentation where the change makes them relevant.
- Verify tests or test metadata when behavior changes or coverage is added.
- Do not report or require fixes for unrelated, pre-existing deviations unless they directly affect the changed code or introduce a regression.

## Engineering Approach

- Preserve the established engineering philosophy, architecture, and conventions of MSAPI. Do not introduce conflicting design patterns or unnecessary abstractions.
- Before making changes, examine the relevant code, interfaces, call sites, tests, and engineering guidelines to understand the existing design and intended behavior.
- If the task description lacks sufficient context, first investigate the repository to resolve ambiguities. Ask for clarification only when the intended behavior cannot be reliably determined or requires an explicit engineering decision.
- Prefer explicit and understandable implementations. Important behavior, assumptions, and trade-offs must be evident from the code or its documentation. Do not force engineers to infer undocumented intent.
- Make minimal, focused changes. Preserve existing behavior and avoid unrelated refactoring unless explicitly requested.
- After completing a task, briefly explain significant design decisions, their rationale, and any remaining limitations. Avoid repeating self-explanatory code or providing unnecessary line-by-line descriptions.
