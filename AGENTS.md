# MSAPI Agent Instructions

This file is the repository-wide entry point for coding and code-review agents.
It routes agents to the detailed MSAPI guidelines.

## Architecture

- Use `README.md` as a source of truth for the repository project structure.

## Required Workflow

- Before changing MSAPI files, identify the applicable guidelines below and read them before editing, including guidelines referenced by the documents being changed.
- Follow mandatory requirements and apply recommendations with engineering judgment, using [Requirement severity and conflicts](guidelines/codeGeneral.md#requirement-severity-and-conflicts). Report unresolved conflicts rather than silently choosing an interpretation.

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

- Examine the relevant code, interfaces, call sites, tests, and guidelines to understand the existing design before editing. Investigate missing context in the repository; ask for clarification when intent or an engineering policy decision remains unresolved.
- Preserve established architecture, behavior, and intentional implementation trade-offs unless the task explicitly authorizes changing them. If changes outside the authorized scope appear necessary, explain the rationale and request explicit approval before proceeding.
- Make minimal, focused changes. Avoid unrelated refactoring or changing unrelated existing code solely to enforce style preferences.
- Code behavior must be explicit, as required by [Explicit behavior](guidelines/codeGeneral.md#explicit-behavior). Prefer the simplest clear solution and avoid unnecessary abstractions.
- Explicitness takes priority over minimizing comments. Follow [Comments](guidelines/codeSyntax.md#comments) without generating redundant explanations; comment quantity is not a measure of code quality.
- Briefly report significant decisions, their rationale, limitations, and unresolved guideline ambiguities after completing a task.
- Agents should preserve reusable project context across sessions in local files when it may benefit future work. Such files must not be tracked by Git and should be stored in a dedicated Git-ignored directory. Treat saved context as supplementary information, not authoritative documentation, and verify it against the current code and guidelines before reuse.

## Git Workflow

- Git stashes represent changes explicitly reviewed and preserved by an engineer. Agents must not create, modify, apply, pop, drop, or otherwise manipulate Git stashes without explicit user authorization.
- Agents may inspect Git stashes, including listing them and reading their contents, provided these operations do not modify the stashes or the working tree.
- Agent-generated changes must remain available for engineering review. Only an engineer may decide to preserve reviewed changes in a Git stash.
