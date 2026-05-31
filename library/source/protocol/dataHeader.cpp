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

DataHeader::DataHeader(const std::span<const uint8_t> buffer) noexcept
{
	if (buffer.size() < 16) [[unlikely]] {
		m_cipher = 0;
		m_bufferSize = 0;
		return;
	}

	const auto* data{ buffer.data() };
	memcpy(&m_cipher, data, sizeof(uint64_t));
	memcpy(&m_bufferSize, data + sizeof(uint64_t), sizeof(uint64_t));
}

DataHeader::DataHeader(const uint64_t cipher) noexcept
	: m_cipher{ cipher }
	, m_bufferSize{ 16 }
{
}

uint64_t DataHeader::GetCipher() const noexcept { return m_cipher; }

uint64_t DataHeader::GetBufferSize() const noexcept { return m_bufferSize; }

std::string DataHeader::ToString() const
{
	std::string result;
	// 88 bytes is the maximum possible size of the formatted string
	result.reserve(88);
	BI(result, "Data header:\n{{\n\tcipher      : {}\n\tbuffer size : {}\n}}", m_cipher, m_bufferSize);
	return result;
}

}; //* namespace MSAPI