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
 *
 * @brief SHA-1 hashing implementation (FIPS PUB 180-4).
 *
 * SHA-1 algorithm authorship and license:
 * - The SHA-1 algorithm was designed by the U.S. National Security Agency (NSA)
 *   and standardized by NIST in the Secure Hash Standard (FIPS PUB 180-4).
 * - The algorithm specification itself is published as a U.S. federal standard and
 *   is generally treated as being in the public domain.
 * - This file contains an independent implementation of that published algorithm
 *   as part of MSAPI and is licensed under the Polyform Noncommercial License 1.0.0
 *   (see LICENSE.md for details).
 */

#ifndef MSAPI_SHA1_INL
#define MSAPI_SHA1_INL

#include "log.h"

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**
 * @brief SHA-1 hashing class.
 */
class SHA1 {
private:
	uint64_t m_bitLen{};
	std::array<uint8_t, 64> m_buffer{};
	std::array<uint32_t, 80> m_processBuffer{};
	uint32_t m_h0{ 0x67452301 };
	uint32_t m_h1{ 0xEFCDAB89 };
	uint32_t m_h2{ 0x98BADCFE };
	uint32_t m_h3{ 0x10325476 };
	uint32_t m_h4{ 0xC3D2E1F0 };
	size_t m_bufferSize{};

public:
	/**
	 * @brief Update the hash with a chunk of data, this method can be called multiple times to hash data in chunks.
	 *
	 * @param data The input data to be hashed.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Update(std::span<const uint8_t> data) noexcept;

	/**
	 * @brief Finalize the hash and return the resulting 20-byte digest.
	 * After calling this method, the SHA1 instance should not be used for further updates.
	 *
	 * @attention The returned span points to internal buffer data that will be overwritten by subsequent calls to
	 * Update or Final.
	 *
	 * @tparam Reset If true, the SHA1 instance will be reset after finalizing. True by default.
	 *
	 * @return The view on 20-byte SHA-1 digest of the input data.
	 *
	 * @test Has unit tests.
	 */
	template <bool Reset = true> FORCE_INLINE [[nodiscard]] std::span<uint8_t> Final() noexcept;

private:
	/**
	 * @brief Rotate left operation for 32-bit unsigned integers.
	 *
	 * @param x The value to rotate.
	 * @param n The number of bits to rotate left.
	 *
	 * @return The result of rotating x left by n bits.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] static uint32_t Rol(uint32_t x, uint32_t n) noexcept;

	/**
	 * @brief Write a 32-bit unsigned integer to a byte array in big-endian order.
	 *
	 * @param dst The destination byte array where the integer will be written (must have at least 4 bytes).
	 * @param x The 32-bit unsigned integer to write.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE static void WriteBe32(uint8_t* const dst, uint32_t x) noexcept;

	/**
	 * @brief Read a 32-bit unsigned integer from a byte array in big-endian order.
	 *
	 * @param p The source byte array from which the integer will be read (must have at least 4 bytes).
	 *
	 * @return The 32-bit unsigned integer read from the byte array.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] static uint32_t ReadBe32(const uint8_t* const p) noexcept;

	/**
	 * @brief Process a single 512-bit block of input data and update the hash state.
	 *
	 * @param block A pointer to a 64-byte block of input data to be processed.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void ProcessBlock(const uint8_t* const block) noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE void SHA1::Update(const std::span<const uint8_t> data) noexcept
{
	const auto size{ data.size() };
	m_bitLen += static_cast<uint64_t>(size) * 8;

	size_t index{};
	if (m_bufferSize != 0) {
		while (true) {
			m_buffer[m_bufferSize++] = data[index++];

			if (m_bufferSize == 64) {
				ProcessBlock(m_buffer.data());
				m_bufferSize = 0;
				break;
			}

			if (index >= size) {
				return;
			}
		}
	}

	while (index + 63 < size) {
		ProcessBlock(data.data() + index);
		index += 64;
	}

	while (index < size) {
		m_buffer[m_bufferSize++] = data[index++];
	}
}

template <bool Reset> FORCE_INLINE [[nodiscard]] std::span<uint8_t> SHA1::Final() noexcept
{
	m_buffer[m_bufferSize++] = 0x80;
	if (m_bufferSize > 56) {
		while (m_bufferSize < 64) {
			m_buffer[m_bufferSize++] = 0x00;
		}

		ProcessBlock(m_buffer.data());
		m_bufferSize = 0;
	}

	while (m_bufferSize < 56) {
		m_buffer[m_bufferSize++] = 0x00;
	}

	for (int index{ 7 }; index >= 0; --index) {
		m_buffer[m_bufferSize++] = static_cast<uint8_t>((m_bitLen >> (index * 8)) & 0xFF);
	}

	ProcessBlock(m_buffer.data());
	m_bufferSize = 0;

	auto* dataPtr{ m_buffer.data() };
	WriteBe32(dataPtr, m_h0);
	dataPtr += 4;
	WriteBe32(dataPtr, m_h1);
	dataPtr += 4;
	WriteBe32(dataPtr, m_h2);
	dataPtr += 4;
	WriteBe32(dataPtr, m_h3);
	dataPtr += 4;
	WriteBe32(dataPtr, m_h4);

	if constexpr (Reset) {
		m_bitLen = 0;
		m_h0 = 0x67452301;
		m_h1 = 0xEFCDAB89;
		m_h2 = 0x98BADCFE;
		m_h3 = 0x10325476;
		m_h4 = 0xC3D2E1F0;
	}

	return std::span<uint8_t>{ m_buffer.data(), 20 };
}

FORCE_INLINE [[nodiscard]] uint32_t SHA1::Rol(const uint32_t x, const uint32_t n) noexcept
{
	return (x << n) | (x >> (32 - n));
}

FORCE_INLINE void SHA1::WriteBe32(uint8_t* const dst, const uint32_t x) noexcept
{
	dst[0] = static_cast<uint8_t>((x >> 24) & 0xFF);
	dst[1] = static_cast<uint8_t>((x >> 16) & 0xFF);
	dst[2] = static_cast<uint8_t>((x >> 8) & 0xFF);
	dst[3] = static_cast<uint8_t>(x & 0xFF);
}

FORCE_INLINE [[nodiscard]] uint32_t SHA1::ReadBe32(const uint8_t* const p) noexcept
{
	return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

FORCE_INLINE void SHA1::ProcessBlock(const uint8_t* const block) noexcept
{
	uint32_t* const w{ m_processBuffer.data() };
	for (uint32_t index{}; index < 16; ++index) {
		w[index] = ReadBe32(block + index * 4);
	}
	for (uint32_t index{ 16 }; index < 80; ++index) {
		w[index] = Rol(w[index - 3] ^ w[index - 8] ^ w[index - 14] ^ w[index - 16], 1);
	}

	uint32_t a{ m_h0 };
	uint32_t b{ m_h1 };
	uint32_t c{ m_h2 };
	uint32_t d{ m_h3 };
	uint32_t e{ m_h4 };

	for (uint32_t index{}; index < 80; ++index) {
		uint32_t f;
		uint32_t k;
		if (index < 20) {
			f = (b & c) | ((~b) & d);
			k = 0x5A827999;
		}
		else if (index < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		}
		else if (index < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		}
		else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}

		const uint32_t temp{ Rol(a, 5) + f + e + k + w[index] };
		e = d;
		d = c;
		c = Rol(b, 30);
		b = a;
		a = temp;
	}

	m_h0 += a;
	m_h1 += b;
	m_h2 += c;
	m_h3 += d;
	m_h4 += e;
}

} // namespace MSAPI

#endif // MSAPI_SHA1_INL