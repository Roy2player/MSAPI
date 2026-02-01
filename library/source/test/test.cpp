/**************************
 * @file        test.cpp
 * @version     6.0
 * @date        2023-09-17
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

#include "test.h"
#include <thread>

namespace MSAPI {

Test::~Test()
{
	if (m_counter > 0) {
		const auto nanoseconds{ Timer::Duration{ Timer{} - m_totalTimer }.GetNanoseconds() };
		if (m_passedCounter == m_counter) {
			LOG_INFO_NEW("All assertions counter: {}, passed: {}. {}Passed{}, elapsed wall time: {} ns", m_counter,
				m_passedCounter, GREEN_BEGIN, COLOR_END, nanoseconds);
			std::cout << std::format("{}Passed{}, elapsed wall time: {} ns", GREEN_BEGIN, COLOR_END, nanoseconds)
					  << std::endl;
			return;
		}

		LOG_INFO_NEW("All assertions counter: {}, passed: {}. {}Failed{}, elapsed wall time: {} ns", m_counter,
			m_passedCounter, RED_BEGIN, COLOR_END, nanoseconds);
		std::cout << std::format("{}Failed{}, elapsed wall time: {} ns", RED_BEGIN, COLOR_END, nanoseconds)
				  << std::endl;
		return;
	}

	LOG_INFO("\033[0;32mThere were no assertions\033[0m");
}

void Test::Wait(size_t waitTime, const std::function<bool()>& predicate)
{
	if (predicate()) {
		return;
	}
	waitTime /= 100;
	while (true) {
		if (waitTime == 0) {
			return;
		}
		if (predicate()) {
			return;
		}

		--waitTime;
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

}; //* namespace MSAPI