/**************************
 * @file        dataHeader.cpp
 * @version     6.0
 * @date        2024-04-09
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

#include "dataHeader.h"
#include "../help/log.h"
#include <cstring>

namespace MSAPI {

DataHeader::DataHeader(const void* buffer)
	: m_cipher{ static_cast<const size_t*>(buffer)[0] }
{
	memcpy(&m_bufferSize, static_cast<const int8_t*>(buffer) + sizeof(size_t), sizeof(size_t));
}

DataHeader::DataHeader(const size_t cipher) noexcept
	: m_cipher{ cipher }
{
}

size_t DataHeader::GetCipher() const noexcept { return m_cipher; }

size_t DataHeader::GetBufferSize() const noexcept { return m_bufferSize; }

std::string DataHeader::ToString() const
{
	std::string result;
	// 88 bytes is the maximum possible size of the formatted string
	result.reserve(88);
	BI(result, "Data header:\n{{\n\tcipher      : {}\n\tbuffer size : {}\n}}", m_cipher, m_bufferSize);
	return result;
}

}; //* namespace MSAPI