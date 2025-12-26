/**************************
 * @file        identifier.cpp
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

#include "identifier.h"
#include <array>
#include <random>

namespace MSAPI {

Identifier::Identifier(int id) noexcept
	: m_id(id)
{
}

int Identifier::GetId() const noexcept { return m_id; }

void Identifier::SetId(const int id) noexcept { m_id = id; }

void Identifier::GenerateUuid(std::string& uuid)
{
	std::array<char, 36> buffer;
	unsigned int index{ 0 };

	auto addHex = [&](int value, int length) {
		for (int i = 0; i < length; ++i) {
			buffer[index++] = "0123456789abcdef"[value & 0xF];
			value >>= 4;
		}
	};

	std::mt19937 mersenne{ std::random_device{}() };

	std::uniform_int_distribution<int> dist{ 0, 15 };
	std::uniform_int_distribution<int> dist3{ 8, 11 };

	for (int i = 0; i < 8; ++i) {
		addHex(dist(mersenne), 1);
	}
	buffer[index++] = '-';
	for (int i = 0; i < 4; ++i) {
		addHex(dist(mersenne), 1);
	}
	buffer[index++] = '-';
	buffer[index++] = '4'; //* UUID version 4
	for (int i = 0; i < 3; ++i) {
		addHex(dist(mersenne), 1);
	}
	buffer[index++] = '-';
	addHex(dist3(mersenne), 1); //* UUID variant
	for (int i = 0; i < 3; ++i) {
		addHex(dist(mersenne), 1);
	}
	buffer[index++] = '-';
	for (int i = 0; i < 12; ++i) {
		addHex(dist(mersenne), 1);
	}

	uuid = std::string{ buffer.data(), buffer.size() };
}

}; //* namespace MSAPI