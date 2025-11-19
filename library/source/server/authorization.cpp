/**************************
 * @file        authorization.cpp
 * @version     1.0
 * @date        2025-11-19
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#include "authorization.hpp"

namespace MSAPI {

namespace Authorization {

std::string_view EnumToString(Grade grade) noexcept
{
	static_assert(U(Grade::Admin) == 127, "Missed description for a new authorization grade enum");

	switch (grade) {
	case Grade::Guest:
		return "Guest";
	case Grade::Observer:
		return "Observer";
	case Grade::User:
		return "User";
	case Grade::Moderator:
		return "Moderator";
	case Grade::Admin:
		return "Admin";
	default:
		LOG_ERROR("Unknown authorization grade: " + _S(U(grade)));
		return "Unknown";
	}
}

}; //* namespace Authorization

}; //* namespace MSAPI
