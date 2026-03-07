---
name: msapi-manager-frontend
description: Manager web frontend guidance for MSAPI. Use when changing `apps/manager/web/` HTML, CSS, JS modules, views, or frontend tests.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI manager frontend guide

## Current frontend shape

- The manager UI is a static frontend under `apps/manager/web/`.
- There is no application-level package manifest; keep changes lightweight and self-contained.

## Core JS modules

- `view.js`: window/view creation, movement, resizing, maximize/hide/close flows
- `table.js`: dynamic table creation and validation
- `grid.js`: tabular display, sorting, filtering, and row/column operations
- `timer.js` / `duration.js`: normalization and validation for time inputs
- `select.js`: searchable select inputs with metadata
- `helper.js`: shared validation, formatting, and utility helpers

## Default views

- Installed apps, created apps, new app, modify app, app iframe view, table view, select view, and grid settings all live under `apps/manager/web/js/views/`.

## Frontend conventions

- Keep JavaScript modular and avoid implicit globals.
- Follow existing naming: PascalCase for constructors/classes, camelCase for functions and variables.
- Reuse the current helpers and view abstractions before adding new patterns.

## Frontend testing

- Tests live in `apps/manager/web/js/tests/`.
- Use the existing `TestRunner`, `TableChecker`, and `test.Assert(...)` helpers.
- Prefer focused tests for the specific module or view being changed.

## Frontend security

- Avoid injecting untrusted content through `innerHTML`.
- Validate postMessage payloads and restrict iframe interactions to expected actions.
- Validate response shapes before mutating the DOM.
