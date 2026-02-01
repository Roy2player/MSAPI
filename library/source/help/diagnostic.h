/**************************
 * @file        diagnostic.h
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

#ifndef MSAPI_DIAGNOSTIC_H
#define MSAPI_DIAGNOSTIC_H

#include <string>

namespace MSAPI {

namespace Diagnostic {

/**************************
 * @brief Print binary descriptor of data in stdout.
 *
 * @param data Pointer to data.
 * @param size Size of data.
 * @param title Title of descriptor.
 *
 * @example ---- title ----
 * 01010101 01010101 01010101 01010101
 * 01010101 01010101 01010101 01010101
 * 01010101 01010101 01010101 01010101
 *
 * @todo Make as template function.
 */
void PrintBinaryDescriptor(const void* data, size_t size, const std::string& title);

}; //* namespace Diagnostic

}; //* namespace MSAPI

#endif //* MSAPI_DIAGNOSTIC_H