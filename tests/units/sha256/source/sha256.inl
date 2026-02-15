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
		// NIST and common SHA-256 test vectors
		MSAPI::Sha256 sha256;

		const auto getSha256{ [&sha256](const std::string_view text) noexcept -> std::span<const uint8_t> {
			sha256.Update(std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(text.data()), text.size() });
			return sha256.Final<true>();
		} };

		{
			const std::array<uint8_t, 32> data{ 186, 120, 22, 191, 143, 1, 207, 234, 65, 65, 64, 222, 93, 174, 34, 35,
				176, 3, 97, 163, 150, 23, 122, 156, 180, 16, 255, 97, 242, 0, 21, 173 };
			RETURN_IF_FALSE(t.Assert(
				getSha256("abc"), std::span<const uint8_t>(data.data(), 32), "Sha256('abc') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 227, 176, 196, 66, 152, 252, 28, 20, 154, 251, 244, 200, 153, 111, 185,
				36, 39, 174, 65, 228, 100, 155, 147, 76, 164, 149, 153, 27, 120, 82, 184, 85 };
			RETURN_IF_FALSE(t.Assert(
				sha256.Final<true>(), std::span<const uint8_t>(data.data(), 32), "Sha256('') should be correct"));
		}

		{
			std::string longInput(1000000, 'a');
			const std::array<uint8_t, 32> data{ 205, 199, 110, 92, 153, 20, 251, 146, 129, 161, 199, 226, 132, 215, 62,
				103, 241, 128, 154, 72, 164, 151, 32, 14, 4, 109, 57, 204, 199, 17, 44, 208 };
			RETURN_IF_FALSE(t.Assert(getSha256(longInput), std::span<const uint8_t>(data.data(), 32),
				"Sha256(1 million 'a's) should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 202, 151, 129, 18, 202, 27, 189, 202, 250, 194, 49, 179, 154, 35, 220,
				77, 167, 134, 239, 248, 20, 124, 78, 114, 185, 128, 119, 133, 175, 238, 72, 187 };
			RETURN_IF_FALSE(
				t.Assert(getSha256("a"), std::span<const uint8_t>(data.data(), 32), "Sha256('a') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 54, 187, 229, 14, 217, 104, 65, 209, 4, 67, 188, 182, 112, 214, 85, 79,
				10, 52, 183, 97, 190, 103, 236, 156, 74, 138, 210, 192, 196, 76, 164, 44 };
			RETURN_IF_FALSE(t.Assert(
				getSha256("abcde"), std::span<const uint8_t>(data.data(), 32), "Sha256('abcde') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 218, 42, 176, 49, 99, 23, 122, 57, 158, 103, 230, 153, 202, 156, 141,
				214, 17, 115, 136, 221, 17, 151, 111, 126, 153, 65, 186, 59, 14, 182, 51, 110 };
			RETURN_IF_FALSE(t.Assert(getSha256("message digits"), std::span<const uint8_t>(data.data(), 32),
				"Sha256('message digits') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 113, 196, 128, 223, 147, 214, 174, 47, 30, 250, 209, 68, 124, 102, 201,
				82, 94, 49, 98, 24, 207, 81, 252, 141, 158, 216, 50, 242, 218, 241, 139, 115 };
			RETURN_IF_FALSE(t.Assert(getSha256("abcdefghijklmnopqrstuvwxyz"), std::span<const uint8_t>(data.data(), 32),
				"Sha256('abcdefghijklmnopqrstuvwxyz') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 219, 75, 252, 189, 77, 160, 205, 133, 166, 12, 60, 55, 211, 251, 216,
				128, 92, 119, 241, 95, 198, 177, 253, 254, 97, 78, 224, 167, 200, 253, 180, 192 };
			RETURN_IF_FALSE(t.Assert(getSha256("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
				std::span<const uint8_t>(data.data(), 32), "Sha256('A..Za..z0..9') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 199, 117, 231, 183, 87, 237, 230, 48, 205, 10, 161, 17, 59, 209, 2, 102,
				26, 179, 136, 41, 202, 82, 166, 66, 42, 183, 130, 134, 47, 38, 134, 70 };
			RETURN_IF_FALSE(t.Assert(getSha256("1234567890"), std::span<const uint8_t>(data.data(), 32),
				"Sha256('1234567890') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 215, 168, 251, 179, 7, 215, 128, 148, 105, 202, 154, 188, 176, 8, 46,
				79, 141, 86, 81, 228, 109, 60, 219, 118, 45, 2, 208, 191, 55, 201, 229, 146 };
			RETURN_IF_FALSE(t.Assert(getSha256("The quick brown fox jumps over the lazy dog"),
				std::span<const uint8_t>(data.data(), 32), "Sha256('quick brown fox') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 239, 83, 127, 37, 200, 149, 191, 167, 130, 82, 101, 41, 169, 182, 61,
				151, 170, 99, 21, 100, 213, 215, 137, 194, 183, 101, 68, 140, 134, 53, 251, 108 };
			RETURN_IF_FALSE(t.Assert(getSha256("The quick brown fox jumps over the lazy dog."),
				std::span<const uint8_t>(data.data(), 32), "Sha256('quick brown fox.') should be correct"));
		}

		{
			const std::array<uint8_t, 32> data{ 36, 141, 106, 97, 210, 6, 56, 184, 229, 192, 38, 147, 12, 62, 96, 57,
				163, 60, 228, 89, 100, 255, 33, 103, 246, 236, 237, 212, 25, 219, 6, 193 };
			RETURN_IF_FALSE(t.Assert(getSha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
				std::span<const uint8_t>(data.data(), 32), "Sha256(multi-block NIST vector) should be correct"));
		}
	}

	{
		// Chunked vs single update equivalence
		const std::string text{ "The quick brown fox jumps over the lazy dog" };
		const auto* base{ reinterpret_cast<const uint8_t*>(text.data()) };

		MSAPI::Sha256 shaSingle;
		shaSingle.Update(std::span<const uint8_t>{ base, text.size() });

		MSAPI::Sha256 shaChunked;
		shaChunked.Update(std::span<const uint8_t>{ base, 10 });
		shaChunked.Update(std::span<const uint8_t>{ base + 10, 15 });
		shaChunked.Update(std::span<const uint8_t>{ base + 25, text.size() - 25 });

		RETURN_IF_FALSE(t.Assert(shaChunked.Final(), shaSingle.Final(), "Chunked update should match single update"));
	}

	{
		// Different order
		const std::string pass{ "password" };
		const auto* passPtr{ reinterpret_cast<const uint8_t*>(pass.data()) };
		const std::string salt{ "NaCl" };
		const auto* saltPtr{ reinterpret_cast<const uint8_t*>(salt.data()) };

		const std::array<uint8_t, 32> firstData{ 178, 10, 183, 74, 162, 84, 159, 126, 19, 160, 232, 134, 203, 68, 113,
			204, 46, 112, 252, 210, 206, 128, 117, 192, 238, 100, 131, 171, 186, 97, 50, 243 };

		MSAPI::Sha256 sha1;
		sha1.Update(std::span<const uint8_t>{ saltPtr, salt.size() });
		sha1.Update(std::span<const uint8_t>{ passPtr, pass.size() });
		RETURN_IF_FALSE(t.Assert(sha1.Final<true>(), std::span<const uint8_t>(firstData.data(), firstData.size()),
			"Sha256('NaCl' + 'password') should be correct"));

		MSAPI::Sha256 sha2;
		std::array<uint8_t, 32> secondData{ 2, 132, 128, 151, 17, 4, 179, 118, 145, 244, 28, 67, 14, 89, 224, 127, 212,
			197, 174, 15, 83, 49, 123, 42, 162, 224, 108, 248, 221, 187, 254, 16 };

		sha2.Update(std::span<const uint8_t>{ passPtr, pass.size() });
		sha2.Update(std::span<const uint8_t>{ saltPtr, salt.size() });
		RETURN_IF_FALSE(t.Assert(sha2.Final(), std::span<const uint8_t>(secondData.data(), secondData.size()),
			"Sha256('password' + 'NaCl') should be correct"));
	}

	// Boundary lengths around padding edge cases
	for (size_t len : { 55u, 56u, 57u, 63u, 64u, 65u }) {
		const std::string data(len, 'x');
		const auto* base{ reinterpret_cast<const uint8_t*>(data.data()) };

		MSAPI::Sha256 shaSingle;
		shaSingle.Update(std::span<const uint8_t>{ base, data.size() });

		MSAPI::Sha256 shaChunked;
		const size_t mid{ len / 2 };
		shaChunked.Update(std::span<const uint8_t>{ base, mid });
		shaChunked.Update(std::span<const uint8_t>{ base + mid, len - mid });

		RETURN_IF_FALSE(
			t.Assert(shaChunked.Final(), shaSingle.Final(), "Boundary length chunked vs single should match"));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_SHA256_INL