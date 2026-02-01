/**************************
 * @file        actionsCounter.h
 * @version     6.0
 * @date        2024-05-02
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

#ifndef MSAPI_ACTIONS_COUNTER_H
#define MSAPI_ACTIONS_COUNTER_H

#include "test.h"
#include <unistd.h>

namespace MSAPI {

/**************************
 * @brief Class for counting actions and waiting for their number in tests.
 */
class ActionsCounter {
private:
	size_t m_counter{ 0 };

public:
	/**************************
	 * @brief Blocks the pthread for a specified time and wait the number of actions. If the number of actions is
	 * zero, will wait only time. Delay between checks is 100 microseconds.
	 *
	 * @param test Test object.
	 * @param delay Time to wait in microseconds.
	 * @param expected Expected number of actions.
	 */
	void WaitActionsNumber(const Test& test, size_t delay, size_t expected = 0) const;

	/**************************
	 * @return Readable reference to number of actions.
	 */
	const size_t& GetActionsNumber() const noexcept;

	/**************************
	 * @brief Increment number of actions.
	 */
	void IncrementActionsNumber();

	/**************************
	 * @brief Clear number of actions.
	 */
	void ClearActionsNumber();
};

}; //* namespace MSAPI

#endif //* MSAPI_ACTIONS_COUNTER_H