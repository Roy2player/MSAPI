/**************************
 * @file        actionsCounter.inl
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

#ifndef MSAPI_ACTIONS_COUNTER_INL
#define MSAPI_ACTIONS_COUNTER_INL

#include "test.inl"
#include <atomic>
#include <unistd.h>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Actions counter for tests.
 *
 * @concurrency Yes.
 */
class ActionsCounter {
private:
	std::atomic<uint64_t> m_counter{};

public:
	/**************************
	 * @locking Not required.
	 *
	 * @return Readable reference to number of actions.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetActionsNumber() const noexcept;

	/**************************
	 * @locking Not required.
	 *
	 * @brief Increment number of actions.
	 */
	FORCE_INLINE void IncrementActionsNumber() noexcept;

	/**************************
	 * @locking Not required.
	 *
	 * @brief Clear number of actions.
	 */
	FORCE_INLINE void ClearActionsNumber() noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] uint64_t ActionsCounter::GetActionsNumber() const noexcept
{
	return m_counter.load(std::memory_order_acquire);
}

FORCE_INLINE void ActionsCounter::IncrementActionsNumber() noexcept
{
	(void)m_counter.fetch_add(1, std::memory_order_release);
}

FORCE_INLINE void ActionsCounter::ClearActionsNumber() noexcept { m_counter.store(0, std::memory_order_release); }

} // namespace MSAPI

#endif // MSAPI_ACTIONS_COUNTER_INL