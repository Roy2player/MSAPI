# MSAPI Agent Instructions

This file is the repository-wide entry point for coding and code-review agents.
It routes agents to the detailed MSAPI guidelines.

## Architecture

- Agents must use `README.md` as a source of truth for the repository project structure.

## Required Workflow

- Before editing MSAPI files, agents must identify and read the applicable guidelines below, including guidelines referenced by the documents being changed.
- Agents must use [Requirement severity and conflicts](guidelines/codeGeneral.md#requirement-severity-and-conflicts) as the authoritative interpretation mechanism for these instructions and the Engineering Guidelines.
- Agents must enforce mandatory requirements, apply engineering judgment to recommendations, distinguish preferences from obligations, and treat optional practices as discretionary.
- Agents must not promote recommendations into mandatory requirements or interpret imperative wording alone as mandatory.
- Agents must report genuinely ambiguous or contradictory requirements rather than inventing resolutions.

## Guideline Map

- [Code General](guidelines/codeGeneral.md): General guidelines.
- [Code Style](guidelines/codeStyle.md): Naming, source layout, and formatting. Applies to every code change.
- [Code Syntax](guidelines/codeSyntax.md): Declarations, attributes, types, initialization, calls, and friendship. Applies to C++ code changes.
- [Concurrency](guidelines/concurrency.md): Lock ownership and scope, shared data, lifetime across threads, and atomic state. Applies when execution can overlap across threads or callbacks.
- [Doxygen Documentation](guidelines/doxygenDocumentation.md): Documentation block structure and Doxygen tags. Applies when adding or modifying public APIs, abstractions, or inline documentation.
- [File Organization](guidelines/fileOrganization.md): Inlinable library layout and structure of .inl files.
- [Logging](guidelines/logging.md): Logging macro selection, level semantics, message grammar, and value formatting. Applies when adding or modifying logs.

## Code Review Requirements

For every pull request or code review:

- Reviewers must determine which guidelines apply to the changed code.
- Reviewers must evaluate the changed code against each applicable guideline at its declared severity, applying engineering judgment to justified deviations from recommendations.
- Reviewers must report each material violation with the relevant guideline link and a concise, concrete explanation.
- Reviewers must verify concurrency, ownership, protocol parsing, error handling, logging, and documentation where the change makes them relevant.
- Reviewers must verify tests or test metadata when behavior changes or coverage is added.
- Reviewers must not report or require fixes for unrelated, pre-existing deviations unless they directly affect the changed code or introduce a regression.

## Engineering Approach

- Before editing, agents must examine the relevant code, interfaces, call sites, tests, and guidelines to understand the existing design.
- Agents must investigate missing context in the repository and ask for clarification when intent or an engineering policy decision remains unresolved.
- Agents must preserve established architecture, behavior, and intentional implementation trade-offs unless the task explicitly authorizes changing them.
- If changes outside the authorized scope appear necessary, agents must explain the rationale and request explicit approval before proceeding.
- Changes should be minimal and focused.
- Agents must not perform unrelated refactoring or change unrelated existing code solely to enforce style preferences.
- Code behavior must be explicit, as required by [Explicit behavior](guidelines/codeGeneral.md#explicit-behavior).
- The simplest clear solution is preferred.
- Unnecessary abstractions should be avoided.
- Agents must prioritize explicitness over minimizing comments and follow [Comments](guidelines/codeSyntax.md#comments).
- Redundant explanations should be avoided; comment quantity is not a measure of code quality.
- Agents must briefly report significant decisions, their rationale, limitations, and unresolved guideline ambiguities after completing a task.
- Agents should preserve reusable project context across sessions in local files when it may benefit future work.
- Such files must not be tracked by Git and should be stored in a dedicated Git-ignored directory.
- Agents must treat saved context as supplementary information rather than authoritative documentation and verify it against the current code and guidelines before reuse.

## Git Workflow

Git stashes represent changes explicitly reviewed and preserved by an engineer.

- Agents must not create, modify, apply, pop, drop, or otherwise manipulate Git stashes without explicit user authorization.
- Agents may inspect Git stashes, including listing them and reading their contents, provided these operations do not modify the stashes or the working tree.
- Agent-generated changes must remain available for engineering review.
- An engineer must make the decision to preserve reviewed changes in a Git stash.
