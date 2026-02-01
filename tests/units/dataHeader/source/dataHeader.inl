/**************************
 * @file        dataHeader.inl
 * @version     6.0
 * @date        2025-11-20
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

#ifndef MSAPI_UNIT_TEST_DATA_HEADER_INL
#define MSAPI_UNIT_TEST_DATA_HEADER_INL

#include "../../../../library/source/protocol/dataHeader.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for DataHeader class.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool DataHeader();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool DataHeader()
{
	LOG_INFO_UNITTEST("MSAPI Data header");
	MSAPI::Test t;

	RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ 8 }.GetCipher(), 8, "Cipher is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ 8 }.GetBufferSize(), 16, "Buffer size is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ 8 }.ToString(),
		"Data header:\n{"
		"\n\tcipher      : 8"
		"\n\tbuffer size : 16\n}",
		"Data to string is expected"));

	RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ 8 }, MSAPI::DataHeader{ 8 }, "Objects are equal, operator=="));
	RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ 8 } != MSAPI::DataHeader{ 8 }, false, "Objects are equal, operator!="));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::DataHeader{ 7 } == MSAPI::DataHeader{ 8 }, false, "Objects are not equal by cipher, operator=="));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::DataHeader{ 7 } != MSAPI::DataHeader{ 8 }, true, "Objects are not equal by cipher, operator!="));

	{
		std::array<uint64_t, 2> data1{ UINT64(67125387623456789), UINT64(98765434) };
		RETURN_IF_FALSE(
			t.Assert(MSAPI::DataHeader{ data1.data() }.GetCipher(), 67125387623456789, "Cipher is expected"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::DataHeader{ data1.data() }.GetBufferSize(), 98765434, "Buffer size is expected"));
		RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ data1.data() }.ToString(),
			"Data header:\n{"
			"\n\tcipher      : 67125387623456789"
			"\n\tbuffer size : 98765434\n}",
			"Data to string is expected"));
		std::array<uint64_t, 2> data2{ UINT64(67125387623456789), UINT64(98765435) };
		RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ data1.data() } == MSAPI::DataHeader{ data2.data() }, false,
			"Objects are not equal by buffer size, operator=="));
		RETURN_IF_FALSE(t.Assert(MSAPI::DataHeader{ data1.data() } != MSAPI::DataHeader{ data2.data() }, true,
			"Objects are not equal by buffer size, operator!="));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_DATA_HEADER_INL