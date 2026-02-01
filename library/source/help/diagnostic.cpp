/**************************
 * @file        diagnostic.cpp
 * @version     6.0
 * @date        2023-09-24
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
 */

#include "diagnostic.h"
#include "log.h"
#include <bitset>

namespace MSAPI {

namespace Diagnostic {

#define suppress
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wsign-conversion"

void PrintBinaryDescriptor(const void* data, const size_t size, const std::string& title)
{
	std::stringstream stream;
	stream << "---- " << title << " ----" << std::endl;
	for (size_t index{ 0 }; index < size; ++index) {
		// if (index % 4 == 0) {
		// stream << (void*)&reinterpret_cast<const char*>(data)[index] << " ";
		// }
		stream << std::bitset<8>(reinterpret_cast<const char*>(data)[index]);
		if ((index + 1) % 4 == 0 || index + 1 == size) {
			stream << std::endl;
		}
	}
	LOG_DEBUG(stream.str());
}

#pragma GCC diagnostic pop
#undef suppress

}; //* namespace Diagnostic

}; //* namespace MSAPI