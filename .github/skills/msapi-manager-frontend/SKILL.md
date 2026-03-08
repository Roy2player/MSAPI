---
name: msapi-manager-frontend
description: Manager web frontend guidance for MSAPI. Use when changing `apps/manager/web/` HTML, CSS, JS modules, views, or frontend tests.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI manager frontend guide

## Frontend Workflow (Manager App)

### Current State

- The manager frontend is a static application under `apps/manager/web/`.
- There is no application-level `package.json`; keep JavaScript modular without introducing a framework by default.
- Avoid global pollution by using IIFEs or ES modules where the current code permits it.
- Optimize images before commit when an asset change is part of the task.

### JS Core

- **View** (`apps/manager/web/js/view.js`): view creation, movement, resizing, snapping, maximize/hide/close flows, and error handling
- **Table** (`apps/manager/web/js/table.js`): dynamic table creation and management for mutable and immutable tables
- **Grid** (`apps/manager/web/js/grid.js`): sorting, filtering, and row/column management for tabular data
- **Timer** (`apps/manager/web/js/timer.js`): timestamp normalization and validation with timezone support
- **Duration** (`apps/manager/web/js/duration.js`): duration parsing, normalization, and validation
- **Select** (`apps/manager/web/js/select.js`): searchable select input behavior with metadata integration
- **Helper** (`apps/manager/web/js/helper.js`): type limits, validation, formatting, equality, and general utilities

### Default views

- **InstalledApps** (`apps/manager/web/js/views/installedApps.js`): grid of installed MSAPI applications
- **CreatedApps** (`apps/manager/web/js/views/createdApps.js`): grid of created or running apps with parameters and action buttons
- **NewApp** (`apps/manager/web/js/views/newApp.js`): form for creating applications
- **ModifyApp** (`apps/manager/web/js/views/modifyApp.js`): parameter editing view
- **AppView** (`apps/manager/web/js/views/appView.js`): iframe-hosted application UI view
- **TableView** (`apps/manager/web/js/views/tableView.js`): read-only MSAPI table display for a parameter
- **SelectView** (`apps/manager/web/js/views/selectView.js`): searchable, case-sensitive select dialog
- **GridSettingsView** (`apps/manager/web/js/views/gridSettingsView.js`): column alignment, sorting, and filtering settings view

### JS Style & Conventions

- Use the same Polyform license block as the existing JS files.
- Use PascalCase for classes and constructors.
- Use camelCase for functions and variables.
- Use UPPER_SNAKE or PascalCase for constants, matching existing usage.
- Avoid implicit globals and prefer pure helpers in `helper.js` when possible.

### Frontend Testing (Node-Based)

Location: `apps/manager/web/js/tests/`

```bash
cd apps/manager/web/js/tests
bash runJsTests.sh
```

- `testRunner.js` provides `TestRunner` and `TableChecker`.
- Use `test.Assert(actual, expected, message)` for assertions.
- Use `await TestRunner.WaitFor(() => condition, 'description', timeoutMs)` for async checks.

### Security Considerations

- Avoid `innerHTML` with untrusted input.
- Restrict iframe bridge messages to expected actions and validate payload shapes.
- Validate response structures before injecting or mutating DOM content.
