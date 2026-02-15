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
	size_t m_bufferSize{};
	uint64_t m_bitSize{};
	std::array<uint32_t, 8> m_state{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab,
		0x5be0cd19 };
	std::array<uint8_t, 64> m_buffer{};

public:
	/**
	 * @brief Update the hash with new data.
	 *
	 * @param data The input data to be hashed.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Update(std::span<const uint8_t> data) noexcept;

	/**
	 * @brief Finalize the hash and return the 32-byte digest.
	 *
	 * @attention The returned span points to internal buffer data that will be overwritten by subsequent calls to
	 * Update or Final.
	 *
	 * @tparam Reset If true, the Sha256 instance will be reset after finalizing. False by default.
	 *
	 * @return The view on 32-byte SHA-256 digest of the input data.
	 *
	 * @test Has unit tests.
	 */
	template <bool Reset = false> FORCE_INLINE std::span<const uint8_t> Final() noexcept;

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
	 * @brief Process a single 512-bit block of input data and update the hash state.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void ProcessBlock(const uint8_t* block) noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE void Sha256::Update(const std::span<const uint8_t> data) noexcept
{
	const auto size{ data.size() };
	m_bitSize += static_cast<uint64_t>(size) * 8;

	size_t index{};
	if (m_bufferSize != 0) {
		while (true) {
			if (index >= size) {
				return;
			}

			m_buffer[m_bufferSize++] = data[index++];

			if (m_bufferSize == 64) {
				ProcessBlock(m_buffer.data());
				m_bufferSize = 0;
				break;
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

template <bool Reset> FORCE_INLINE std::span<const uint8_t> Sha256::Final() noexcept
{
	m_buffer[m_bufferSize++] = 0x80;
	if (m_bufferSize > 56) {
		while (m_bufferSize < 64) {
			m_buffer[m_bufferSize++] = 0;
		}

		ProcessBlock(m_buffer.data());
		m_bufferSize = 0;
	}

	while (m_bufferSize < 56) {
		m_buffer[m_bufferSize++] = 0;
	}

	for (int8_t index{ 7 }; index >= 0; --index) {
		m_buffer[m_bufferSize++] = (m_bitSize >> (index * 8)) & 0xff;
	}

	ProcessBlock(m_buffer.data());

	for (size_t index{}; index < 8; ++index) {
		m_buffer[index * 4 + 0] = (m_state[index] >> 24) & 0xff;
		m_buffer[index * 4 + 1] = (m_state[index] >> 16) & 0xff;
		m_buffer[index * 4 + 2] = (m_state[index] >> 8) & 0xff;
		m_buffer[index * 4 + 3] = (m_state[index]) & 0xff;
	}

	if constexpr (Reset) {
		m_bufferSize = 0;
		m_bitSize = 0;
		m_state = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	}

	return std::span<const uint8_t>{ m_buffer.data(), 32 };
}

FORCE_INLINE uint32_t Sha256::Rotr(uint32_t x, uint32_t n) noexcept { return (x >> n) | (x << (32 - n)); }

FORCE_INLINE uint32_t Sha256::Ch(uint32_t x, uint32_t y, uint32_t z) noexcept { return (x & y) ^ (~x & z); }

FORCE_INLINE uint32_t Sha256::Maj(uint32_t x, uint32_t y, uint32_t z) noexcept { return (x & y) ^ (x & z) ^ (y & z); }

FORCE_INLINE uint32_t Sha256::Bsig0(uint32_t x) noexcept { return Rotr(x, 2) ^ Rotr(x, 13) ^ Rotr(x, 22); }

FORCE_INLINE uint32_t Sha256::Bsig1(uint32_t x) noexcept { return Rotr(x, 6) ^ Rotr(x, 11) ^ Rotr(x, 25); }

FORCE_INLINE uint32_t Sha256::Ssig0(uint32_t x) noexcept { return Rotr(x, 7) ^ Rotr(x, 18) ^ (x >> 3); }

FORCE_INLINE uint32_t Sha256::Ssig1(uint32_t x) noexcept { return Rotr(x, 17) ^ Rotr(x, 19) ^ (x >> 10); }

FORCE_INLINE void Sha256::ProcessBlock(const uint8_t* const block) noexcept
{
	uint32_t w[64];
	for (int8_t index{}; index < 16; ++index) {
		w[index] = (static_cast<uint32_t>(block[index * 4]) << 24) | (static_cast<uint32_t>(block[index * 4 + 1]) << 16)
			| (static_cast<uint32_t>(block[index * 4 + 2]) << 8) | static_cast<uint32_t>(block[index * 4 + 3]);
	}
	for (int8_t index{ 16 }; index < 64; ++index) {
		w[index] = Ssig1(w[index - 2]) + w[index - 7] + Ssig0(w[index - 15]) + w[index - 16];
	}

	uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
	uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];

	for (int8_t index{}; index < 64; ++index) {
		uint32_t t1{ h + Bsig1(e) + Ch(e, f, g) + K[UINT64(index)] + w[UINT64(index)] };
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

} // namespace MSAPI

#endif // MSAPI_SHA256_INL