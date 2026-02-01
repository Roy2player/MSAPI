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
 *
 * @brief SHA-256 hashing implementation (FIPS PUB 180-4).
 *
 * SHA-256 algorithm authorship and license:
 * - The SHA-256 algorithm was designed by the U.S. National Security Agency (NSA)
 *   and standardized by NIST in the Secure Hash Standard (FIPS PUB 180-4).
 * - The algorithm specification itself is published as a U.S. federal standard and
 *   is generally treated as being in the public domain.
 * - This file contains an independent implementation of that published algorithm
 *   as part of MSAPI and is licensed under the Polyform Noncommercial License 1.0.0
 *   (see LICENSE.md for details).
 */

#ifndef MSAPI_SHA256_INL
#define MSAPI_SHA256_INL

#include "log.h"

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**
 * @brief SHA-256 hashing class.
 */
class Sha256 {
private:
	std::array<uint32_t, 8> m_state;
	std::array<uint8_t, 64> m_buffer;
	size_t m_bufferLen;
	uint64_t m_bitLen;

public:
	/**
	 * @brief Construct a new Sha256 object, call Reset inside.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE Sha256() noexcept;

	/**
	 * @brief Update the hash with new data.
	 *
	 * @param data The input data to be hashed.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Update(std::string_view data) noexcept;

	/**
	 * @brief Compute and retrieve the final digits and reset the state.
	 *
	 * @return The computed digits as a byte array.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE std::array<uint8_t, 32> GetDigits() noexcept;

	/**
	 * @brief Compute and retrieve the final digits as a hexadecimal string and reset the state.
	 *
	 * @return The computed digits as a hexadecimal string.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE std::array<char, 65> GetHexDigits() noexcept;

private:
	static constexpr std::array<uint32_t, 64> K{ 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
		0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
		0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85,
		0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
		0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c,
		0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

private:
	FORCE_INLINE [[nodiscard]] static uint32_t Rotr(uint32_t x, uint32_t n) noexcept;
	FORCE_INLINE [[nodiscard]] static uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) noexcept;
	FORCE_INLINE [[nodiscard]] static uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) noexcept;
	FORCE_INLINE [[nodiscard]] static uint32_t Bsig0(uint32_t x) noexcept;
	FORCE_INLINE [[nodiscard]] static uint32_t Bsig1(uint32_t x) noexcept;
	FORCE_INLINE [[nodiscard]] static uint32_t Ssig0(uint32_t x) noexcept;
	FORCE_INLINE [[nodiscard]] static uint32_t Ssig1(uint32_t x) noexcept;

	/**
	 * @brief Perform the main SHA-256 transformation on the current buffer.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Transform() noexcept;

	/**
	 * @brief Pad the current buffer according to SHA-256 specifications.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Pad() noexcept;

	/**
	 * @brief Reset the SHA-256 state to initial values.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Reset() noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE Sha256::Sha256() noexcept { Reset(); }

FORCE_INLINE void Sha256::Update(const std::string_view data) noexcept
{
	const auto* dataPtr{ reinterpret_cast<const uint8_t*>(data.data()) };
	const size_t len{ data.size() };

	for (size_t i{}; i < len; ++i) {
		m_buffer[m_bufferLen++] = dataPtr[i];
		m_bitLen += 8;

		if (m_bufferLen == 64) {
			Transform();
			m_bufferLen = 0;
		}
	}
}

FORCE_INLINE std::array<uint8_t, 32> Sha256::GetDigits() noexcept
{
	Pad();
	std::array<uint8_t, 32> out{};
	for (size_t i{}; i < 8; ++i) {
		out[i * 4 + 0] = (m_state[i] >> 24) & 0xff;
		out[i * 4 + 1] = (m_state[i] >> 16) & 0xff;
		out[i * 4 + 2] = (m_state[i] >> 8) & 0xff;
		out[i * 4 + 3] = (m_state[i]) & 0xff;
	}

	// In optimized builds, when the object is not reused after this call, the compiler can eliminate the Reset() stores
	// as dead code. Reset() is kept here to make repeated use of the same Sha256 instance safe without requiring
	// callers to remember to reinitialize it.
	Reset();

	return out;
}

FORCE_INLINE std::array<char, 65> Sha256::GetHexDigits() noexcept
{
	static constexpr char hexChars[] = "0123456789abcdef";

	const auto d{ GetDigits() };
	std::array<char, 65> out{};
	for (size_t i{}; i < d.size(); ++i) {
		const uint8_t byte{ d[UINT64(i)] };
		out[2 * i] = hexChars[(byte >> 4) & 0x0F];
		out[2 * i + 1] = hexChars[byte & 0x0F];
	}

	out[64] = '\0';
	return out;
}

FORCE_INLINE uint32_t Sha256::Rotr(uint32_t x, uint32_t n) noexcept { return (x >> n) | (x << (32 - n)); }

FORCE_INLINE uint32_t Sha256::Ch(uint32_t x, uint32_t y, uint32_t z) noexcept { return (x & y) ^ (~x & z); }

FORCE_INLINE uint32_t Sha256::Maj(uint32_t x, uint32_t y, uint32_t z) noexcept { return (x & y) ^ (x & z) ^ (y & z); }

FORCE_INLINE uint32_t Sha256::Bsig0(uint32_t x) noexcept { return Rotr(x, 2) ^ Rotr(x, 13) ^ Rotr(x, 22); }

FORCE_INLINE uint32_t Sha256::Bsig1(uint32_t x) noexcept { return Rotr(x, 6) ^ Rotr(x, 11) ^ Rotr(x, 25); }

FORCE_INLINE uint32_t Sha256::Ssig0(uint32_t x) noexcept { return Rotr(x, 7) ^ Rotr(x, 18) ^ (x >> 3); }

FORCE_INLINE uint32_t Sha256::Ssig1(uint32_t x) noexcept { return Rotr(x, 17) ^ Rotr(x, 19) ^ (x >> 10); }

FORCE_INLINE void Sha256::Transform() noexcept
{
	uint8_t* const block{ m_buffer.data() };

	uint32_t w[64];
	for (int8_t i{}; i < 16; ++i) {
		w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) | (static_cast<uint32_t>(block[i * 4 + 1]) << 16)
			| (static_cast<uint32_t>(block[i * 4 + 2]) << 8) | static_cast<uint32_t>(block[i * 4 + 3]);
	}
	for (int8_t i{ 16 }; i < 64; ++i) {
		w[i] = Ssig1(w[i - 2]) + w[i - 7] + Ssig0(w[i - 15]) + w[i - 16];
	}

	uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
	uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];

	for (int8_t i{}; i < 64; ++i) {
		uint32_t t1{ h + Bsig1(e) + Ch(e, f, g) + K[UINT64(i)] + w[UINT64(i)] };
		uint32_t t2{ Bsig0(a) + Maj(a, b, c) };
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	m_state[0] += a;
	m_state[1] += b;
	m_state[2] += c;
	m_state[3] += d;
	m_state[4] += e;
	m_state[5] += f;
	m_state[6] += g;
	m_state[7] += h;
}

FORCE_INLINE void Sha256::Pad() noexcept
{
	m_buffer[m_bufferLen++] = 0x80;
	if (m_bufferLen > 56) {
		while (m_bufferLen < 64) {
			m_buffer[m_bufferLen++] = 0;
		}
		Transform();
		m_bufferLen = 0;
	}
	while (m_bufferLen < 56) {
		m_buffer[m_bufferLen++] = 0;
	}

	for (int8_t i{ 7 }; i >= 0; --i) {
		m_buffer[m_bufferLen++] = (m_bitLen >> (i * 8)) & 0xff;
	}

	Transform();
}

FORCE_INLINE void Sha256::Reset() noexcept
{
	m_state = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	m_bufferLen = 0;
	m_bitLen = 0;
}

} // namespace MSAPI

#endif // MSAPI_SHA256_INL