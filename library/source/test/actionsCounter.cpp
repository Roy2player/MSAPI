/**************************
 * @file        actionsCounter.cpp
 * @version     6.0
 * @date        2024-05-02
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

#include "actionsCounter.h"
#include <thread>

namespace MSAPI {

void ActionsCounter::WaitActionsNumber(const Test& test, size_t delay, const size_t expected) const
{
	test.Wait(delay, [this, expected]() { return m_counter == expected; });
}

const size_t& ActionsCounter::GetActionsNumber() const noexcept { return m_counter; }

void ActionsCounter::IncrementActionsNumber() { ++m_counter; }

void ActionsCounter::ClearActionsNumber() { m_counter = 0; }

}; //* namespace MSAPI