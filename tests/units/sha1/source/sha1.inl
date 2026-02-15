/**************************
 * @file        sha1.inl
 * @version     6.0
 * @date        2026-02-08
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

#ifndef MSAPI_UNIT_TEST_SHA1_INL
#define MSAPI_UNIT_TEST_SHA1_INL

#include "../../../../library/source/help/sha1.inl"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Sha1.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Sha1();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Sha1()
{
	LOG_INFO_UNITTEST("MSAPI Sha1");
	MSAPI::Test t;

	{
		// NIST and common SHA-1 test vectors
		MSAPI::Sha1 sha1;

		const auto getSha1{ [&sha1](const std::string_view text) noexcept -> std::span<const uint8_t> {
			sha1.Update(std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(text.data()), text.size() });
			return sha1.Final<true>();
		} };

		{
			const std::array<uint8_t, 20> data{ 169, 153, 62, 54, 71, 6, 129, 106, 186, 62, 37, 113, 120, 80, 194, 108,
				156, 208, 216, 157 };
			RETURN_IF_FALSE(
				t.Assert(getSha1("abc"), std::span<const uint8_t>(data.data(), 20), "Sha1('abc') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 218, 57, 163, 238, 94, 107, 75, 13, 50, 85, 191, 239, 149, 96, 24, 144,
				175, 216, 7, 9 };
			RETURN_IF_FALSE(
				t.Assert(sha1.Final<true>(), std::span<const uint8_t>(data.data(), 20), "Sha1('') should be correct"));
		}

		{
			std::string longInput(1000000, 'a');
			const std::array<uint8_t, 20> data{ 52, 170, 151, 60, 212, 196, 218, 164, 246, 30, 235, 43, 219, 173, 39,
				49, 101, 52, 1, 111 };
			RETURN_IF_FALSE(t.Assert(getSha1(longInput), std::span<const uint8_t>(data.data(), 20),
				"Sha1(1 million 'a's) should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 134, 247, 228, 55, 250, 165, 167, 252, 225, 93, 29, 220, 185, 234, 234,
				234, 55, 118, 103, 184 };
			RETURN_IF_FALSE(
				t.Assert(getSha1("a"), std::span<const uint8_t>(data.data(), 20), "Sha1('a') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 3, 222, 108, 87, 11, 254, 36, 191, 195, 40, 204, 215, 202, 70, 183, 110,
				173, 175, 67, 52 };
			RETURN_IF_FALSE(t.Assert(
				getSha1("abcde"), std::span<const uint8_t>(data.data(), 20), "Sha1('abcde') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 193, 34, 82, 206, 218, 139, 232, 153, 77, 95, 160, 41, 10, 71, 35, 28,
				29, 22, 170, 227 };
			RETURN_IF_FALSE(t.Assert(getSha1("message digest"), std::span<const uint8_t>(data.data(), 20),
				"Sha1('message digest') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 50, 209, 12, 123, 140, 249, 101, 112, 202, 4, 206, 55, 242, 161, 157,
				132, 36, 13, 58, 137 };
			RETURN_IF_FALSE(t.Assert(getSha1("abcdefghijklmnopqrstuvwxyz"), std::span<const uint8_t>(data.data(), 20),
				"Sha1('abcdefghijklmnopqrstuvwxyz') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 118, 28, 69, 123, 247, 59, 20, 210, 126, 158, 146, 101, 196, 111, 75,
				77, 218, 17, 249, 64 };
			RETURN_IF_FALSE(t.Assert(getSha1("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
				std::span<const uint8_t>(data.data(), 20), "Sha1('A..Za..z0..9') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 1, 179, 7, 172, 186, 79, 84, 245, 90, 175, 195, 59, 176, 107, 187, 246,
				202, 128, 62, 154 };
			RETURN_IF_FALSE(t.Assert(getSha1("1234567890"), std::span<const uint8_t>(data.data(), 20),
				"Sha1('1234567890') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 47, 212, 225, 198, 122, 45, 40, 252, 237, 132, 158, 225, 187, 118, 231,
				57, 27, 147, 235, 18 };
			RETURN_IF_FALSE(t.Assert(getSha1("The quick brown fox jumps over the lazy dog"),
				std::span<const uint8_t>(data.data(), 20), "Sha1('quick brown fox') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 64, 141, 148, 56, 66, 22, 248, 144, 255, 122, 12, 53, 40, 232, 190, 209,
				224, 176, 22, 33 };
			RETURN_IF_FALSE(t.Assert(getSha1("The quick brown fox jumps over the lazy dog."),
				std::span<const uint8_t>(data.data(), 20), "Sha1('quick brown fox.') should be correct"));
		}

		{
			const std::array<uint8_t, 20> data{ 132, 152, 62, 68, 28, 59, 210, 110, 186, 174, 74, 161, 249, 81, 41, 229,
				229, 70, 112, 241 };
			RETURN_IF_FALSE(t.Assert(getSha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
				std::span<const uint8_t>(data.data(), 20), "Sha1(multi-block NIST vector) should be correct"));
		}
	}

	{
		// Chunked vs single update equivalence
		const std::string text{ "The quick brown fox jumps over the lazy dog" };
		const auto* base{ reinterpret_cast<const uint8_t*>(text.data()) };

		MSAPI::Sha1 shaSingle;
		shaSingle.Update(std::span<const uint8_t>{ base, text.size() });

		MSAPI::Sha1 shaChunked;
		shaChunked.Update(std::span<const uint8_t>{ base, 10 });
		shaChunked.Update(std::span<const uint8_t>{ base + 10, 15 });
		shaChunked.Update(std::span<const uint8_t>{ base + 25, text.size() - 25 });

		RETURN_IF_FALSE(
			t.Assert(shaChunked.Final(), shaSingle.Final(), "Chunked update digits should match single update digits"));
	}

	{
		// Different order
		const std::string pass{ "password" };
		const auto* passPtr{ reinterpret_cast<const uint8_t*>(pass.data()) };
		const std::string salt{ "NaCl" };
		const auto* saltPtr{ reinterpret_cast<const uint8_t*>(salt.data()) };

		const std::array<uint8_t, 20> firstData{ 227, 41, 212, 5, 74, 255, 57, 5, 108, 9, 4, 25, 147, 254, 133, 154,
			134, 29, 39, 47 };

		MSAPI::Sha1 sha1;
		sha1.Update(std::span<const uint8_t>{ saltPtr, salt.size() });
		sha1.Update(std::span<const uint8_t>{ passPtr, pass.size() });
		RETURN_IF_FALSE(t.Assert(sha1.Final<true>(), std::span<const uint8_t>(firstData.data(), firstData.size()),
			"Sha1('NaCl' + 'password') should be correct"));

		MSAPI::Sha1 sha2;
		const std::array<uint8_t, 20> secondData{ 64, 39, 72, 146, 210, 254, 1, 166, 171, 30, 15, 189, 229, 194, 43,
			131, 18, 209, 7, 128 };
		sha2.Update(std::span<const uint8_t>{ passPtr, pass.size() });
		sha2.Update(std::span<const uint8_t>{ saltPtr, salt.size() });
		RETURN_IF_FALSE(t.Assert(sha2.Final(), std::span<const uint8_t>(secondData.data(), secondData.size()),
			"Sha1('password' + 'NaCl') should be correct"));
	}

	// Boundary lengths around padding edge cases
	for (size_t len : { 55u, 56u, 57u, 63u, 64u, 65u }) {
		const std::string data(len, 'x');
		const auto* base{ reinterpret_cast<const uint8_t*>(data.data()) };

		MSAPI::Sha1 shaSingle;
		shaSingle.Update(std::span<const uint8_t>{ base, len });

		MSAPI::Sha1 shaChunked;
		const size_t mid{ len / 2 };
		shaChunked.Update(std::span<const uint8_t>{ base, mid });
		shaChunked.Update(std::span<const uint8_t>{ base + mid, len - mid });

		RETURN_IF_FALSE(
			t.Assert(shaChunked.Final(), shaSingle.Final(), "Boundary length chunked vs single digits should match"));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_SHA1_INL