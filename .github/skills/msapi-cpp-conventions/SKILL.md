---
name: msapi-cpp-conventions
description: C++ coding, API, logging, and source layout conventions for MSAPI. Use when editing C++ headers, source files, enums, or public APIs.
license: Repository content under Polyform Noncommercial License 1.0.0
---

# MSAPI C++ conventions

## Coding Standards

### C++ Style

- **Formatting**: follow `.clang-format` exactly
- **Indentation**: use tabs, with tab width and indent width equal to 4
- **Line length**: maximum 120 columns
- **Braces**:
  - after function: newline
  - after class, control statement, and namespace: same line
  - before `else` and `catch`: newline
- **Naming conventions**:
  - classes: PascalCase
  - member variables: `m_camelCase`
  - functions: PascalCase
  - namespaces: PascalCase
- Use modern C++ features such as attributes and concepts when they fit the current compiler baseline.
- Prefer `std::optional` over raw pointers where the API should model optional values explicitly.
- Use `namespace MSAPI` for library code.
- Document public APIs with Doxygen tags such as `@brief`, `@param`, `@return`, and `@test`.

### File Headers

All new source files should use the standard MSAPI copyright header:

```cpp
/**************************
 * @file        filename.ext
 * @version     CURRENT_VERSION
 * @date        YYYY-MM-DD
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–YYYY Maksim Andreevich Leonov
 *
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: maks.angels@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–YYYY Maksim Andreevich Leonov, maks.angels@mail.ru
 */
```

- Replace `filename.ext`, `CURRENT_VERSION`, and `YYYY` as appropriate.
- The current project version can be checked in existing source files such as `library/source/help/log.cpp`.

### Enum Guidance

- Use `enum class` with an explicit underlying type sized appropriately for the domain.
- Preserve sentinel values such as `Undefined` and `Max`, and document what they mean.
- Use `switch` statements with `static_assert` coverage checks and log unexpected values with `LOG_ERROR`.
- Match the established conversion pattern used in files such as `standardType.hpp`, `object.cpp`, and `log.cpp`.

```cpp
static FORCE_INLINE std::string_view EnumToString(const InternalAction value)
{
	static_assert(U(InternalAction::Max) == 4, "Need to specify a new internal action");

	switch (value) {
	case InternalAction::Undefined:
		return "Undefined";
	case InternalAction::Usual:
		return "Usual";
	case InternalAction::Fill:
		return "Fill";
	case InternalAction::Cancel:
		return "Cancel";
	case InternalAction::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown internal action: " + _S(U(value)));
		return "Unknown";
	}
}
```

## Common Patterns

### Logging

- Use macros from `log.h`.
- Prefer the `*_NEW` formatted-string logging helpers when available.
- Format floating-point values with the established precision: `%.9f` for `float`, `%.17f` for `double`, and `%.21Lf` for `long double`.
