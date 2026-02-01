/**************************
 * @file        sha256.inl
 * @version     6.0
 * @date        2025-12-23
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

#ifndef MSAPI_UNIT_TEST_SHA256_INL
#define MSAPI_UNIT_TEST_SHA256_INL

#include "../../../../library/source/help/sha256.inl"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Sha256.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Sha256();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Sha256()
{
	LOG_INFO_UNITTEST("MSAPI Sha256");
	MSAPI::Test t;

	{
		MSAPI::Sha256 sha256;
		sha256.Update("abc");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "Sha256('abc') hex should be correct"));
		sha256.Update("abc");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 186, 120, 22, 191, 143, 1, 207, 234, 65, 65, 64, 222, 93, 174, 34, 35, 176, 3, 97,
				163, 150, 23, 122, 156, 180, 16, 255, 97, 242, 0, 21, 173 },
			"Sha256('abc') should be correct"));

		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "Sha256('') hex should be correct"));
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 227, 176, 196, 66, 152, 252, 28, 20, 154, 251, 244, 200, 153, 111, 185, 36, 39,
				174, 65, 228, 100, 155, 147, 76, 164, 149, 153, 27, 120, 82, 184, 85 },
			"Sha256('') should be correct"));

		std::string longInput(1000000, 'a');
		sha256.Update(longInput);
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
			"Sha256(1 million 'a's) hex should be correct"));
		sha256.Update(longInput);
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 205, 199, 110, 92, 153, 20, 251, 146, 129, 161, 199, 226, 132, 215, 62, 103, 241,
				128, 154, 72, 164, 151, 32, 14, 4, 109, 57, 204, 199, 17, 44, 208 },
			"Sha256(1 million 'a's) should be correct"));

		sha256.Update("a");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", "Sha256('a') hex should be correct"));
		sha256.Update("a");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 202, 151, 129, 18, 202, 27, 189, 202, 250, 194, 49, 179, 154, 35, 220, 77, 167,
				134, 239, 248, 20, 124, 78, 114, 185, 128, 119, 133, 175, 238, 72, 187 },
			"Sha256('a') should be correct"));

		sha256.Update("abcde");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"36bbe50ed96841d10443bcb670d6554f0a34b761be67ec9c4a8ad2c0c44ca42c",
			"Sha256('abcde') hex should be correct"));
		sha256.Update("abcde");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 54, 187, 229, 14, 217, 104, 65, 209, 4, 67, 188, 182, 112, 214, 85, 79, 10, 52,
				183, 97, 190, 103, 236, 156, 74, 138, 210, 192, 196, 76, 164, 44 },
			"Sha256('abcde') should be correct"));

		sha256.Update("message digits");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"da2ab03163177a399e67e699ca9c8dd6117388dd11976f7e9941ba3b0eb6336e",
			"Sha256('message digits') hex should be correct"));
		sha256.Update("message digits");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 218, 42, 176, 49, 99, 23, 122, 57, 158, 103, 230, 153, 202, 156, 141, 214, 17, 115,
				136, 221, 17, 151, 111, 126, 153, 65, 186, 59, 14, 182, 51, 110 },
			"Sha256('message digits') should be correct"));

		sha256.Update("abcdefghijklmnopqrstuvwxyz");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"71c480df93d6ae2f1efad1447c66c9525e316218cf51fc8d9ed832f2daf18b73",
			"Sha256('abcdefghijklmnopqrstuvwxyz') hex should be correct"));
		sha256.Update("abcdefghijklmnopqrstuvwxyz");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 113, 196, 128, 223, 147, 214, 174, 47, 30, 250, 209, 68, 124, 102, 201, 82, 94, 49,
				98, 24, 207, 81, 252, 141, 158, 216, 50, 242, 218, 241, 139, 115 },
			"Sha256('abcdefghijklmnopqrstuvwxyz') should be correct"));

		sha256.Update("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"db4bfcbd4da0cd85a60c3c37d3fbd8805c77f15fc6b1fdfe614ee0a7c8fdb4c0",
			"Sha256('A..Za..z0..9') hex should be correct"));
		sha256.Update("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 219, 75, 252, 189, 77, 160, 205, 133, 166, 12, 60, 55, 211, 251, 216, 128, 92, 119,
				241, 95, 198, 177, 253, 254, 97, 78, 224, 167, 200, 253, 180, 192 },
			"Sha256('A..Za..z0..9') should be correct"));

		sha256.Update("1234567890");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"c775e7b757ede630cd0aa1113bd102661ab38829ca52a6422ab782862f268646",
			"Sha256('1234567890') hex should be correct"));
		sha256.Update("1234567890");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 199, 117, 231, 183, 87, 237, 230, 48, 205, 10, 161, 17, 59, 209, 2, 102, 26, 179,
				136, 41, 202, 82, 166, 66, 42, 183, 130, 134, 47, 38, 134, 70 },
			"Sha256('1234567890') should be correct"));

		sha256.Update("The quick brown fox jumps over the lazy dog");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
			"Sha256('quick brown fox') hex should be correct"));
		sha256.Update("The quick brown fox jumps over the lazy dog");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 215, 168, 251, 179, 7, 215, 128, 148, 105, 202, 154, 188, 176, 8, 46, 79, 141, 86,
				81, 228, 109, 60, 219, 118, 45, 2, 208, 191, 55, 201, 229, 146 },
			"Sha256('quick brown fox') should be correct"));

		sha256.Update("The quick brown fox jumps over the lazy dog.");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c",
			"Sha256('quick brown fox.') hex should be correct"));
		sha256.Update("The quick brown fox jumps over the lazy dog.");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 239, 83, 127, 37, 200, 149, 191, 167, 130, 82, 101, 41, 169, 182, 61, 151, 170, 99,
				21, 100, 213, 215, 137, 194, 183, 101, 68, 140, 134, 53, 251, 108 },
			"Sha256('quick brown fox.') should be correct"));

		sha256.Update("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
			"Sha256(multi-block NIST vector) hex should be correct"));
		sha256.Update("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 36, 141, 106, 97, 210, 6, 56, 184, 229, 192, 38, 147, 12, 62, 96, 57, 163, 60, 228,
				89, 100, 255, 33, 103, 246, 236, 237, 212, 25, 219, 6, 193 },
			"Sha256(multi-block NIST vector) should be correct"));
	}

	{
		const std::string_view text{ "The quick brown fox jumps over the lazy dog" };
		MSAPI::Sha256 shaSingle;
		shaSingle.Update(text);

		MSAPI::Sha256 shaChunked;
		shaChunked.Update(text.substr(0, 10));
		shaChunked.Update(text.substr(10, 15));
		shaChunked.Update(text.substr(25));

		RETURN_IF_FALSE(t.Assert(std::string_view{ shaChunked.GetHexDigits().data() },
			std::string_view{ shaSingle.GetHexDigits().data() }, "Chunked update hex should match single update hex"));

		shaSingle.Update(text);
		shaChunked.Update(text.substr(0, 10));
		shaChunked.Update(text.substr(10, 15));
		shaChunked.Update(text.substr(25));
		RETURN_IF_FALSE(t.Assert(shaChunked.GetDigits(), shaSingle.GetDigits(),
			"Double chunked update digits should match double single update digits"));
	}

	{
		MSAPI::Sha256 sha256;
		const std::string_view data{ "password" };
		const std::string_view salt{ "NaCl" };

		sha256.Update(salt);
		sha256.Update(data);
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"b20ab74aa2549f7e13a0e886cb4471cc2e70fcd2ce8075c0ee6483abba6132f3",
			"Sha256('NaCl' + 'password') hex should be correct"));

		sha256.Update(salt);
		sha256.Update(data);
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 178, 10, 183, 74, 162, 84, 159, 126, 19, 160, 232, 134, 203, 68, 113, 204, 46, 112,
				252, 210, 206, 128, 117, 192, 238, 100, 131, 171, 186, 97, 50, 243 },
			"Sha256('NaCl' + 'password') should be correct"));

		sha256.Update(data);
		sha256.Update(salt);
		RETURN_IF_FALSE(t.Assert(std::string_view{ sha256.GetHexDigits().data() },
			"028480971104b37691f41c430e59e07fd4c5ae0f53317b2aa2e06cf8ddbbfe10",
			"Sha256('password' + 'NaCl') hex should be correct"));

		sha256.Update(data);
		sha256.Update(salt);
		RETURN_IF_FALSE(t.Assert(sha256.GetDigits(),
			std::array<uint8_t, 32>{ 2, 132, 128, 151, 17, 4, 179, 118, 145, 244, 28, 67, 14, 89, 224, 127, 212, 197,
				174, 15, 83, 49, 123, 42, 162, 224, 108, 248, 221, 187, 254, 16 },
			"Sha256('password' + 'NaCl') should be correct"));
	}

	for (size_t len : { 55u, 56u, 57u, 63u, 64u, 65u }) {
		std::string data(len, 'x');

		MSAPI::Sha256 shaSingle;
		shaSingle.Update(data);

		MSAPI::Sha256 shaChunked;
		const size_t mid{ len / 2 };
		shaChunked.Update(std::string_view(data).substr(0, mid));
		shaChunked.Update(std::string_view(data).substr(mid));

		RETURN_IF_FALSE(t.Assert(std::string_view{ shaChunked.GetHexDigits().data() },
			std::string_view{ shaSingle.GetHexDigits().data() }, "Boundary length chunked vs single hex should match"));

		shaSingle.Update(data);
		shaChunked.Update(std::string_view(data).substr(0, mid));
		shaChunked.Update(std::string_view(data).substr(mid));
		RETURN_IF_FALSE(t.Assert(shaChunked.GetDigits(), shaSingle.GetDigits(),
			"Boundary length double chunked vs single digits should match"));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_SHA256_INL