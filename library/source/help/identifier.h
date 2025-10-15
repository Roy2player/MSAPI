/**************************
 * @file        identifier.h
 * @version     6.0
 * @date        2023-12-09
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

#ifndef MSAPI_IDENTIFIER_H
#define MSAPI_IDENTIFIER_H

#include <random>

namespace MSAPI {

/**************************
 * @brief Common class to store identifier. Provide methods to generate UUID version 4 and public access to random
 * generator.
 */
class Identifier {
public:
	static std::mt19937 mersenne;

private:
	static std::uniform_int_distribution<int> m_dist;
	static std::uniform_int_distribution<int> m_dist3;

protected:
	int m_id;

public:
	/**************************
	 * @brief Construct a new empty Identifier object, empty constructor.
	 */
	Identifier() noexcept = default;

	/**************************
	 * @brief Construct a new Identifier object, empty constructor.
	 *
	 * @param id Identifier.
	 */
	Identifier(int id) noexcept;

	/**************************
	 * @return Identifier.
	 */
	int GetId() const noexcept;

	/**************************
	 * @brief Set identifier.
	 *
	 * @param id Identifier.
	 */
	void SetId(int id) noexcept;

	/**************************
	 * @brief Generate UUID.
	 *
	 * @param uuid Reference to string for UUID.
	 */
	static void GenerateUuid(std::string& uuid);
};

}; //* namespace MSAPI

#endif //* MSAPI_IDENTIFIER_H