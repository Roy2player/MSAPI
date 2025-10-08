/**************************
 * @file        dataHeader.cpp
 * @version     6.0
 * @date        2024-04-09
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

#include "dataHeader.h"
#include "../test/test.h"
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

bool DataHeader::UNITTEST()
{
	LOG_INFO_UNITTEST("MSAPI Data header");
	MSAPI::Test t;

	RETURN_IF_FALSE(t.Assert(DataHeader{ 8 }.GetCipher(), 8, "Cipher is expected"));
	RETURN_IF_FALSE(t.Assert(DataHeader{ 8 }.GetBufferSize(), 16, "Buffer size is expected"));
	RETURN_IF_FALSE(t.Assert(DataHeader{ 8 }.ToString(),
		"Data header:\n{"
		"\n\tcipher      : 8"
		"\n\tbuffer size : 16\n}",
		"Data to string is expected"));

	RETURN_IF_FALSE(t.Assert(DataHeader{ 8 }, DataHeader{ 8 }, "Objects are equal, operator=="));
	RETURN_IF_FALSE(t.Assert(DataHeader{ 8 } != DataHeader{ 8 }, false, "Objects are equal, operator!="));
	RETURN_IF_FALSE(t.Assert(DataHeader{ 7 } == DataHeader{ 8 }, false, "Objects are not equal by cipher, operator=="));
	RETURN_IF_FALSE(t.Assert(DataHeader{ 7 } != DataHeader{ 8 }, true, "Objects are not equal by cipher, operator!="));

	{
		std::array<uint64_t, 2> data1{ UINT64(67125387623456789), UINT64(98765434) };
		RETURN_IF_FALSE(t.Assert(DataHeader{ data1.data() }.GetCipher(), 67125387623456789, "Cipher is expected"));
		RETURN_IF_FALSE(t.Assert(DataHeader{ data1.data() }.GetBufferSize(), 98765434, "Buffer size is expected"));
		RETURN_IF_FALSE(t.Assert(DataHeader{ data1.data() }.ToString(),
			"Data header:\n{"
			"\n\tcipher      : 67125387623456789"
			"\n\tbuffer size : 98765434\n}",
			"Data to string is expected"));
		std::array<uint64_t, 2> data2{ UINT64(67125387623456789), UINT64(98765435) };
		RETURN_IF_FALSE(t.Assert(DataHeader{ data1.data() } == DataHeader{ data2.data() }, false,
			"Objects are not equal by buffer size, operator=="));
		RETURN_IF_FALSE(t.Assert(DataHeader{ data1.data() } != DataHeader{ data2.data() }, true,
			"Objects are not equal by buffer size, operator!="));
	}

	return true;
}

}; //* namespace MSAPI