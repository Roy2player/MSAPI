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
		std::stringstream stream;
		stream << "All tests counter " << m_counter << ".\n";

		if (!m_passedTests.empty()) {
			stream << GREEN_BEGIN << "Passed counter " << m_passedTests.size() << ":\n";
			for (const auto& name : m_passedTests) {
				stream << "\t" << name << "\n";
			}
			if (m_failedTests.empty()) {
				stream << "Passed, " << COLOR_END;
			}
			else {
				goto failed;
			}
		}
		if (!m_failedTests.empty()) {
		failed:
			stream << RED_BEGIN << "Failed counter " << m_failedTests.size() << ":\n";
			for (const auto& name : m_failedTests) {
				stream << "\t" << name << "\n";
			}
			stream << "Failed, " << COLOR_END;
		}
		stream << "elapsed wall time: " << Timer::Duration{ Timer{} - m_timer }.GetNanoseconds() << " ns";
		LOG_INFO(stream.str());

		return;
	}

	LOG_INFO("\033[0;32mThere were no running tests\033[0m");
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