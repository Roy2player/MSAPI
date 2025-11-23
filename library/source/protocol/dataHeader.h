/**************************
 * @file        dataHeader.h
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

#ifndef MSAPI_DATA_HEADER_H
#define MSAPI_DATA_HEADER_H

#include <iostream>

namespace MSAPI {

/**************************
 * @brief Object for collecting common data for all protocol data objects: cipher and buffer size.
 */
class DataHeader {
protected:
	const size_t m_cipher;
	size_t m_bufferSize{ 16 };

public:
	/**************************
	 * @brief Construct a new Data Header object, parse cipher and buffer size from buffer.
	 *
	 * @attention Buffer must be at least 16 bytes long, otherwise undefined behaviour.
	 *
	 * @param buffer Buffer with data to parse.
	 *
	 * @test Has unit test.
	 */
	DataHeader(const void* buffer);

	/**************************
	 * @brief Construct a new Data Header object with specific cipher, empty constructor.
	 *
	 * @param cipher Cipher for data.
	 *
	 * @test Has unit test.
	 */
	DataHeader(size_t cipher) noexcept;

	/**************************
	 * @return Cipher of data.
	 *
	 * @test Has unit test.
	 */
	size_t GetCipher() const noexcept;

	/**************************
	 * @return Buffer size of data.
	 *
	 * @test Has unit test.
	 */
	size_t GetBufferSize() const noexcept;

	/**************************
	 * @example Data header:
	 * {
	 * 		cipher      : 2666999999
	 * 		buffer size : 60
	 * }
	 *
	 * @test Has unit test.
	 */
	std::string ToString() const;

	/**************************
	 * @test Has unit test.
	 */
	bool operator==(const DataHeader& x) const noexcept = default;

	/**************************
	 * @test Has unit test.
	 */
	bool operator!=(const DataHeader& x) const noexcept = default;
};

}; //* namespace MSAPI

#endif //* MSAPI_DATA_HEADER_H