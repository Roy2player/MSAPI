/**************************
 * @file        diagnostic.inl
 * @version     6.0
 * @date        2023-12-05
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2026 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2026 Maksim Andreevich Leonov, maks.angels@mail.ru
 *
 * @brief Deep diagnostic of data.
 */

#ifndef MSAPI_DIAGNOSTIC_INL
#define MSAPI_DIAGNOSTIC_INL

#include "log.h"
#include <string>

namespace MSAPI {

namespace Diagnostic {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

constexpr inline bool hex{ true };
constexpr inline bool binary{ false };

/**************************
 * @brief Log raw data.
 *
 * @tparam Notation HEX or Binary notations.
 *
 * @param data Pointer to data.
 * @param size Bytes to be printed.
 * @param title Description to be printed.
 *
 * @example ---- title ----
 * Descriptor of 24 byte(s)
 * 0	01010101 01010101 01010101 01010101 01010101 01010101 01010101 01010101
 * 8	01010101 01010101 01010101 01010101 01010101 01010101 01010101 01010101
 * 16	01010101 01010101 01010101 01010101 01010101 01010101 01010101 01010101
 * @example ---- title ----
 * Descriptor of 24 byte(s)
 * 0	2f 45 aa 89 bc 93 31 99
 * 8	2f 45 aa 89 bc 93 31 99
 * 16	2f 45 aa 89 bc 93 31 99
 */
template <bool Notation> FORCE_INLINE void PrintBinaryDescriptor(const void* data, size_t size, std::string_view title);

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

template <bool Notation>
FORCE_INLINE void PrintBinaryDescriptor(const void* data, const size_t size, const std::string_view title)
{
	std::string str{ std::format("{}\nDescriptor of {} byte(s)", title, size) };
	auto backIt{ std::back_inserter(str) };
	for (size_t index{}; index < size; ++index) {
		if constexpr (Notation == binary) {
			if (index % 8 == 0) {
				std::format_to(backIt, "\n{}\t{:08b} ", index, static_cast<const uint8_t*>(data)[index]);
			}
			else {
				std::format_to(backIt, "{:08b} ", static_cast<const uint8_t*>(data)[index]);
			}
		}
		else {
			if (index % 8 == 0) {
				std::format_to(backIt, "\n{}\t{:02x} ", index, static_cast<const uint8_t*>(data)[index]);
			}
			else {
				std::format_to(backIt, "{:02x} ", static_cast<const uint8_t*>(data)[index]);
			}
		}
	}
	LOG_INFO_NEW("{}\n", str);
}

} // namespace Diagnostic

} // namespace MSAPI

#endif // MSAPI_DIAGNOSTIC_INL