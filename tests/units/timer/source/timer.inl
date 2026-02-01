/**************************
 * @file        timer.inl
 * @version     6.0
 * @date        2025-11-20
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

#ifndef MSAPI_UNIT_TEST_TIMER_INL
#define MSAPI_UNIT_TEST_TIMER_INL

#include "../../../../library/source/help/time.h"
#include "../../../../library/source/test/test.h"
#include <thread>

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Timer.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Timer();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Timer()
{
	LOG_INFO_UNITTEST("MSAPI Timer");
	MSAPI::Test t;

	RETURN_IF_FALSE(t.Assert(MSAPI::Timer::HowMuchDaysInMonth(1, 2022 % 4 == 0), 31, "Days in month 1, 2022"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Timer::HowMuchDaysInMonth(2, 2022 % 4 == 0), 28, "Days in month 2, 2022"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Timer::HowMuchDaysInMonth(3, 2022 % 4 == 0), 31, "Days in month 3, 2022"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Timer::HowMuchDaysInMonth(1, 2024 % 4 == 0), 31, "Days in month 1, 2024"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Timer::HowMuchDaysInMonth(2, 2024 % 4 == 0), 29, "Days in month 2, 2024"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Timer::HowMuchDaysInMonth(3, 2024 % 4 == 0), 31, "Days in month 3, 2024"));

	{
		MSAPI::Timer timer{ std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{
			std::chrono::seconds{ 1734727947 } } };
		RETURN_IF_FALSE(t.Assert(timer.ToString(), "2024-12-20 20:52:27.000000000", "Timer to string №1"));
	}
	{
		MSAPI::Timer timer{ std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{
			std::chrono::seconds{ 85945150 } } };
		RETURN_IF_FALSE(t.Assert(timer.ToString(), "1972-09-21 17:39:10.000000000", "Timer to string №2"));
	}

	{
		MSAPI::Timer timer;
		MSAPI::Timer timer2;
		RETURN_IF_FALSE(t.Assert(timer < timer2, true, "Timer not greater or equal"));
		const auto timer3{ timer };
		RETURN_IF_FALSE(t.Assert(timer3, timer, "Timer copy equals"));
	}

	{
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2022).ToString(), "2022-01-01 00:00:00.000000000", "Timer::Create(2022) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2022).ToDate().ToString(), "2022-01-01", "Timer::Create(2022) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 2).ToString(), "2022-02-01 00:00:00.000000000",
			"Timer::Create(2022,2) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2022, 2).ToDate().ToString(), "2022-02-01", "Timer::Create(2022,2) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 1, 2).ToString(), "2022-01-02 00:00:00.000000000",
			"Timer::Create(2022,1,2) to string"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2022, 1, 2).ToDate().ToString(), "2022-01-02", "Timer::Create(2022,1,2) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 1, 1, 1).ToString(), "2022-01-01 01:00:00.000000000",
			"Timer::Create(2022,1,1,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 1, 1, 0, 1).ToString(), "2022-01-01 00:01:00.000000000",
			"Timer::Create(2022,1,1,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 1, 1, 0, 0, 1).ToString(), "2022-01-01 00:00:01.000000000",
			"Timer::Create(2022,1,1,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 1, 1, 0, 0, 0, 123456789).ToString(),
			"2022-01-01 00:00:00.123456789", "Timer::Create(2022,1,1,0,0,0,123456789) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2022, 1, 1, 0, 0, 0, 1).ToString(),
			"2022-01-01 00:00:00.000000001", "Timer::Create(2022,1,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(1970).ToString(), "1970-01-01 00:00:00.000000000", "Timer::Create(1970) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(1970).ToDate().ToString(), "1970-01-01", "Timer::Create(1970) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1971, 9, 21, 17, 39, 10, 1234).ToString(),
			"1971-09-21 17:39:10.000001234", "Timer::Create(1971,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1971, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1971-09-21",
			"Timer::Create(1971,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1972, 9, 21, 17, 39, 10, 1234).ToString(),
			"1972-09-21 17:39:10.000001234", "Timer::Create(1972,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1972, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1972-09-21",
			"Timer::Create(1972,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1973, 9, 21, 17, 39, 10, 1234).ToString(),
			"1973-09-21 17:39:10.000001234", "Timer::Create(1973,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1973, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1973-09-21",
			"Timer::Create(1973,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1974, 9, 21, 17, 39, 10, 1234).ToString(),
			"1974-09-21 17:39:10.000001234", "Timer::Create(1974,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1974, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1974-09-21",
			"Timer::Create(1974,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1975, 9, 21, 17, 39, 10, 1234).ToString(),
			"1975-09-21 17:39:10.000001234", "Timer::Create(1975,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1975, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1975-09-21",
			"Timer::Create(1975,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1976, 9, 21, 17, 39, 10, 1234).ToString(),
			"1976-09-21 17:39:10.000001234", "Timer::Create(1976,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1976, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1976-09-21",
			"Timer::Create(1976,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1977, 9, 21, 17, 39, 10, 1234).ToString(),
			"1977-09-21 17:39:10.000001234", "Timer::Create(1977,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(1977, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1977-09-21",
			"Timer::Create(1977,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(1978).ToString(), "1978-01-01 00:00:00.000000000", "Timer::Create(1978) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(1978).ToDate().ToString(), "1978-01-01", "Timer::Create(1978) to date"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(1979).ToString(), "1979-01-01 00:00:00.000000000", "Timer::Create(1979) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(1979).ToDate().ToString(), "1979-01-01", "Timer::Create(1979) to date"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(1980).ToString(), "1980-01-01 00:00:00.000000000", "Timer::Create(1980) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(1980).ToDate().ToString(), "1980-01-01", "Timer::Create(1980) to date"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(1990).ToString(), "1990-01-01 00:00:00.000000000", "Timer::Create(1990) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(1990).ToDate().ToString(), "1990-01-01", "Timer::Create(1990) to date"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2000).ToString(), "2000-01-01 00:00:00.000000000", "Timer::Create(2000) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2000).ToDate().ToString(), "2000-01-01", "Timer::Create(2000) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2024, 12, 20, 0, 1).ToDate().ToString(), "2024-12-20",
			"Timer::Create(2024,12,20,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2024, 9, 21, 17, 39, 10, 1234).ToString(),
			"2024-09-21 17:39:10.000001234", "MSAPI::Timer::Create(2024,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2024, 9, 21, 17, 39, 10, 1234).ToString(),
			"2024-09-21 17:39:10.000001234", "MSAPI::Timer::Create(2024,9,21,17,39,10,1234) to string (repeat)"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2224, 9, 21, 17, 39, 10, 1234).ToString(),
			"2224-09-21 17:39:10.000001234", "MSAPI::Timer::Create(2224,9,21,17,39,10,1234) to string"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2052).ToString(), "2052-01-01 00:00:00.000000000", "Timer::Create(2052) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2052).ToDate().ToString(), "2052-01-01", "Timer::Create(2052) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 2, 29, 0, 0, 0, 1).ToString(),
			"2052-02-29 00:00:00.000000001", "Timer::Create(2052,2,29,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 2, 29, 0, 0, 0, 1).ToDate().ToString(), "2052-02-29",
			"Timer::Create(2052,2,29,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 3, 1, 0, 0, 0, 1).ToString(),
			"2052-03-01 00:00:00.000000001", "Timer::Create(2052,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2052-03-01",
			"Timer::Create(2052,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 7, 20, 20, 20, 20, 1).ToString(),
			"2052-07-20 20:20:20.000000001", "Timer::Create(2052,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2052-07-20",
			"Timer::Create(2052,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 12, 31, 20, 20, 20, 1).ToString(),
			"2052-12-31 20:20:20.000000001", "Timer::Create(2052,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2052, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2052-12-31",
			"Timer::Create(2052,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2053).ToString(), "2053-01-01 00:00:00.000000000", "Timer::Create(2053) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2053).ToDate().ToString(), "2053-01-01", "Timer::Create(2053) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 2, 28, 0, 0, 0, 1).ToString(),
			"2053-02-28 00:00:00.000000001", "Timer::Create(2053,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2053-02-28",
			"Timer::Create(2053,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 3, 1, 0, 0, 0, 1).ToString(),
			"2053-03-01 00:00:00.000000001", "Timer::Create(2053,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2053-03-01",
			"Timer::Create(2053,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 7, 20, 20, 20, 20, 1).ToString(),
			"2053-07-20 20:20:20.000000001", "Timer::Create(2053,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2053-07-20",
			"Timer::Create(2053,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 12, 31, 20, 20, 20, 1).ToString(),
			"2053-12-31 20:20:20.000000001", "Timer::Create(2053,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2053, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2053-12-31",
			"Timer::Create(2053,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2054).ToString(), "2054-01-01 00:00:00.000000000", "Timer::Create(2054) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2054).ToDate().ToString(), "2054-01-01", "Timer::Create(2054) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 2, 28, 0, 0, 0, 1).ToString(),
			"2054-02-28 00:00:00.000000001", "Timer::Create(2054,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2054-02-28",
			"Timer::Create(2054,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 3, 1, 0, 0, 0, 1).ToString(),
			"2054-03-01 00:00:00.000000001", "Timer::Create(2054,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2054-03-01",
			"Timer::Create(2054,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 7, 20, 20, 20, 20, 1).ToString(),
			"2054-07-20 20:20:20.000000001", "Timer::Create(2054,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2054-07-20",
			"Timer::Create(2054,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 12, 31, 20, 20, 20, 1).ToString(),
			"2054-12-31 20:20:20.000000001", "Timer::Create(2054,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2054, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2054-12-31",
			"Timer::Create(2054,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2055).ToString(), "2055-01-01 00:00:00.000000000", "Timer::Create(2055) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2055).ToDate().ToString(), "2055-01-01", "Timer::Create(2055) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 2, 28, 0, 0, 0, 1).ToString(),
			"2055-02-28 00:00:00.000000001", "Timer::Create(2055,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2055-02-28",
			"Timer::Create(2055,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 3, 1, 0, 0, 0, 1).ToString(),
			"2055-03-01 00:00:00.000000001", "Timer::Create(2055,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2055-03-01",
			"Timer::Create(2055,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 7, 20, 20, 20, 20, 1).ToString(),
			"2055-07-20 20:20:20.000000001", "Timer::Create(2055,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2055-07-20",
			"Timer::Create(2055,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 12, 31, 20, 20, 20, 1).ToString(),
			"2055-12-31 20:20:20.000000001", "Timer::Create(2055,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2055, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2055-12-31",
			"Timer::Create(2055,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2156).ToString(), "2156-01-01 00:00:00.000000000", "Timer::Create(2156) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2156).ToDate().ToString(), "2156-01-01", "Timer::Create(2156) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 2, 29, 0, 0, 0, 1).ToString(),
			"2156-02-29 00:00:00.000000001", "Timer::Create(2156,2,29,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 2, 29, 0, 0, 0, 1).ToDate().ToString(), "2156-02-29",
			"Timer::Create(2156,2,29,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 3, 1, 0, 0, 0, 1).ToString(),
			"2156-03-01 00:00:00.000000001", "Timer::Create(2156,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2156-03-01",
			"Timer::Create(2156,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 7, 20, 20, 20, 20, 1).ToString(),
			"2156-07-20 20:20:20.000000001", "Timer::Create(2156,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2156-07-20",
			"Timer::Create(2156,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 12, 31, 20, 20, 20, 1).ToString(),
			"2156-12-31 20:20:20.000000001", "Timer::Create(2156,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2156, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2156-12-31",
			"Timer::Create(2156,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2157).ToString(), "2157-01-01 00:00:00.000000000", "Timer::Create(2157) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2157).ToDate().ToString(), "2157-01-01", "Timer::Create(2157) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 2, 28, 0, 0, 0, 1).ToString(),
			"2157-02-28 00:00:00.000000001", "Timer::Create(2157,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2157-02-28",
			"Timer::Create(2157,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 3, 1, 0, 0, 0, 1).ToString(),
			"2157-03-01 00:00:00.000000001", "Timer::Create(2157,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2157-03-01",
			"Timer::Create(2157,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 7, 20, 20, 20, 20, 1).ToString(),
			"2157-07-20 20:20:20.000000001", "Timer::Create(2157,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2157-07-20",
			"Timer::Create(2157,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 12, 31, 20, 20, 20, 1).ToString(),
			"2157-12-31 20:20:20.000000001", "Timer::Create(2157,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2157, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2157-12-31",
			"Timer::Create(2157,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2158).ToString(), "2158-01-01 00:00:00.000000000", "Timer::Create(2158) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2158).ToDate().ToString(), "2158-01-01", "Timer::Create(2158) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 2, 28, 0, 0, 0, 1).ToString(),
			"2158-02-28 00:00:00.000000001", "Timer::Create(2158,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2158-02-28",
			"Timer::Create(2158,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 3, 1, 0, 0, 0, 1).ToString(),
			"2158-03-01 00:00:00.000000001", "Timer::Create(2158,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2158-03-01",
			"Timer::Create(2158,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 7, 20, 20, 20, 20, 1).ToString(),
			"2158-07-20 20:20:20.000000001", "Timer::Create(2158,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2158-07-20",
			"Timer::Create(2158,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 12, 31, 20, 20, 20, 1).ToString(),
			"2158-12-31 20:20:20.000000001", "Timer::Create(2158,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2158, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2158-12-31",
			"Timer::Create(2158,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2159).ToString(), "2159-01-01 00:00:00.000000000", "Timer::Create(2159) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2159).ToDate().ToString(), "2159-01-01", "Timer::Create(2159) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 2, 28, 0, 0, 0, 1).ToString(),
			"2159-02-28 00:00:00.000000001", "Timer::Create(2159,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2159-02-28",
			"Timer::Create(2159,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 3, 1, 0, 0, 0, 1).ToString(),
			"2159-03-01 00:00:00.000000001", "Timer::Create(2159,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2159-03-01",
			"Timer::Create(2159,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 7, 20, 20, 20, 20, 1).ToString(),
			"2159-07-20 20:20:20.000000001", "Timer::Create(2159,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2159-07-20",
			"Timer::Create(2159,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 12, 31, 20, 20, 20, 1).ToString(),
			"2159-12-31 20:20:20.000000001", "Timer::Create(2159,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2159, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2159-12-31",
			"Timer::Create(2159,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2252).ToString(), "2252-01-01 00:00:00.000000000", "Timer::Create(2252) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2252).ToDate().ToString(), "2252-01-01", "Timer::Create(2252) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 2, 29, 0, 0, 0, 1).ToString(),
			"2252-02-29 00:00:00.000000001", "Timer::Create(2252,2,29,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 2, 29, 0, 0, 0, 1).ToDate().ToString(), "2252-02-29",
			"Timer::Create(2252,2,29,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 3, 1, 0, 0, 0, 1).ToString(),
			"2252-03-01 00:00:00.000000001", "Timer::Create(2252,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2252-03-01",
			"Timer::Create(2252,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 7, 20, 20, 20, 20, 1).ToString(),
			"2252-07-20 20:20:20.000000001", "Timer::Create(2252,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2252-07-20",
			"Timer::Create(2252,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 12, 31, 20, 20, 20, 1).ToString(),
			"2252-12-31 20:20:20.000000001", "Timer::Create(2252,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2252, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2252-12-31",
			"Timer::Create(2252,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2253).ToString(), "2253-01-01 00:00:00.000000000", "Timer::Create(2253) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2253).ToDate().ToString(), "2253-01-01", "Timer::Create(2253) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 2, 28, 0, 0, 0, 1).ToString(),
			"2253-02-28 00:00:00.000000001", "Timer::Create(2253,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2253-02-28",
			"Timer::Create(2253,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 3, 1, 0, 0, 0, 1).ToString(),
			"2253-03-01 00:00:00.000000001", "Timer::Create(2253,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2253-03-01",
			"Timer::Create(2253,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 7, 20, 20, 20, 20, 1).ToString(),
			"2253-07-20 20:20:20.000000001", "Timer::Create(2253,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2253-07-20",
			"Timer::Create(2253,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 12, 31, 20, 20, 20, 1).ToString(),
			"2253-12-31 20:20:20.000000001", "Timer::Create(2253,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2253, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2253-12-31",
			"Timer::Create(2253,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2254).ToString(), "2254-01-01 00:00:00.000000000", "Timer::Create(2254) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2254).ToDate().ToString(), "2254-01-01", "Timer::Create(2254) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 2, 28, 0, 0, 0, 1).ToString(),
			"2254-02-28 00:00:00.000000001", "Timer::Create(2254,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2254-02-28",
			"Timer::Create(2254,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 3, 1, 0, 0, 0, 1).ToString(),
			"2254-03-01 00:00:00.000000001", "Timer::Create(2254,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2254-03-01",
			"Timer::Create(2254,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 7, 20, 20, 20, 20, 1).ToString(),
			"2254-07-20 20:20:20.000000001", "Timer::Create(2254,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2254-07-20",
			"Timer::Create(2254,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 12, 31, 20, 20, 20, 1).ToString(),
			"2254-12-31 20:20:20.000000001", "Timer::Create(2254,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2254, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2254-12-31",
			"Timer::Create(2254,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create(2255).ToString(), "2255-01-01 00:00:00.000000000", "Timer::Create(2255) to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Timer::Create(2255).ToDate().ToString(), "2255-01-01", "Timer::Create(2255) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 2, 28, 0, 0, 0, 1).ToString(),
			"2255-02-28 00:00:00.000000001", "Timer::Create(2255,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2255-02-28",
			"Timer::Create(2255,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 3, 1, 0, 0, 0, 1).ToString(),
			"2255-03-01 00:00:00.000000001", "Timer::Create(2255,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2255-03-01",
			"Timer::Create(2255,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 7, 20, 20, 20, 20, 1).ToString(),
			"2255-07-20 20:20:20.000000001", "Timer::Create(2255,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2255-07-20",
			"Timer::Create(2255,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 12, 31, 20, 20, 20, 1).ToString(),
			"2255-12-31 20:20:20.000000001", "Timer::Create(2255,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2255, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2255-12-31",
			"Timer::Create(2255,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2262, 4, 9, 23, 47, 15, 999999999).ToString(),
			"2262-04-09 23:47:15.999999999", "Timer::Create(2262,4,9,23,47,15,999999999) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2262, 4, 9, 23, 47, 15, 999999999).ToDate().ToString(),
			"2262-04-09", "Timer::Create(2262,4,9,23,47,15,999999999) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer{ 9223372035, 999999999 }.ToString(), "2262-04-09 23:47:15.999999999",
			"Timer{9223372035,999999999} to string"));
	}

	{
		const MSAPI::Timer::Date first{ 2024, 9, 21 }, second{ 2024, 9, 22 };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Date < operator: 2024-09-21 < 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-21 <= 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-21 > 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Date >= operator: 2024-09-21 >= 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Date == operator: 2024-09-21 == 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Date != operator: 2024-09-21 != 2024-09-22"));
	}
	{
		const MSAPI::Timer::Date first{ 2024, 9, 22 }, second{ 2024, 10, 22 };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Date < operator: 2024-09-22 < 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-22 <= 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-22 > 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Date >= operator: 2024-09-22 >= 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Date == operator: 2024-09-22 == 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Date != operator: 2024-09-22 != 2024-10-22"));
	}
	{
		const MSAPI::Timer::Date first{ 2024, 9, 22 }, second{ 2025, 9, 22 };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Date < operator: 2024-09-22 < 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-22 <= 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-22 > 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Date >= operator: 2024-09-22 >= 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Date == operator: 2024-09-22 == 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Date != operator: 2024-09-22 != 2025-09-22"));
	}
	{
		const MSAPI::Timer::Date first{ 2024, 9, 21 }, second{ 2024, 9, 21 };
		RETURN_IF_FALSE(t.Assert(first < second, false, "Date < operator: 2024-09-21 < 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-21 <= 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-21 > 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first >= second, true, "Date >= operator: 2024-09-21 >= 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first == second, true, "Date == operator: 2024-09-21 == 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first != second, false, "Date != operator: 2024-09-21 != 2024-09-21"));
	}

	{
		auto timer{ MSAPI::Timer::Create(2000) };
		const auto timerMoreNanosecond{ MSAPI::Timer::Create(2000, 1, 1, 0, 0, 0, 1) };
		const auto timerMoreMicrosecond{ MSAPI::Timer::Create(2000, 1, 1, 0, 0, 0, 1000) };
		const auto timerMoreMillisecond{ MSAPI::Timer::Create(2000, 1, 1, 0, 0, 0, 1000000) };
		const auto timerMoreSecond{ MSAPI::Timer::Create(2000, 1, 1, 0, 0, 1) };
		const auto timerMoreMinute{ MSAPI::Timer::Create(2000, 1, 1, 0, 1) };
		const auto timerMoreHour{ MSAPI::Timer::Create(2000, 1, 1, 1) };
		const auto timerMoreDay{ MSAPI::Timer::Create(2000, 1, 2) };

		RETURN_IF_FALSE(t.Assert(
			timerMoreNanosecond - timer, MSAPI::Timer::Duration::CreateNanoseconds(1), "timerMoreNanosecond - timer"));
		RETURN_IF_FALSE(t.Assert(timerMoreMicrosecond - timer, MSAPI::Timer::Duration::CreateMicroseconds(1),
			"timerMoreMicrosecond - timer"));
		RETURN_IF_FALSE(t.Assert(timerMoreMillisecond - timer, MSAPI::Timer::Duration::CreateMilliseconds(1),
			"timerMoreMillisecond - timer"));
		RETURN_IF_FALSE(
			t.Assert(timerMoreSecond - timer, MSAPI::Timer::Duration::CreateSeconds(1), "timerMoreSecond - timer"));
		RETURN_IF_FALSE(
			t.Assert(timerMoreMinute - timer, MSAPI::Timer::Duration::CreateMinutes(1), "timerMoreMinute - timer"));
		RETURN_IF_FALSE(
			t.Assert(timerMoreHour - timer, MSAPI::Timer::Duration::CreateHours(1), "timerMoreHour - timer"));
		RETURN_IF_FALSE(t.Assert(timerMoreDay - timer, MSAPI::Timer::Duration::CreateDays(1), "timerMoreDay - timer"));

		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateDays(1)),
			MSAPI::Timer::Create(2000, 1, 2), "timer + 1 day"));
		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateHours(1)),
			MSAPI::Timer::Create(2000, 1, 2, 1), "timer + 1 hour"));
		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateMinutes(1)),
			MSAPI::Timer::Create(2000, 1, 2, 1, 1), "timer + 1 minute"));
		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateSeconds(1)),
			MSAPI::Timer::Create(2000, 1, 2, 1, 1, 1), "timer + 1 second"));
		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateMilliseconds(1)),
			MSAPI::Timer::Create(2000, 1, 2, 1, 1, 1, 1000000), "timer + 1 ms"));
		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateMicroseconds(1)),
			MSAPI::Timer::Create(2000, 1, 2, 1, 1, 1, 1001000), "timer + 1 us"));
		RETURN_IF_FALSE(t.Assert((timer = timer + MSAPI::Timer::Duration::CreateNanoseconds(1)),
			MSAPI::Timer::Create(2000, 1, 2, 1, 1, 1, 1001001), "timer + 1 ns"));

		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateDays(1)),
			MSAPI::Timer::Create(2000, 1, 1, 1, 1, 1, 1001001), "timer - 1 day"));
		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateHours(1)),
			MSAPI::Timer::Create(2000, 1, 1, 0, 1, 1, 1001001), "timer - 1 h"));
		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateMinutes(1)),
			MSAPI::Timer::Create(2000, 1, 1, 0, 0, 1, 1001001), "timer - 1 min"));
		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateSeconds(1)),
			MSAPI::Timer::Create(2000, 1, 1, 0, 0, 0, 1001001), "timer - 1 s"));
		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateMilliseconds(1)),
			MSAPI::Timer::Create(2000, 1, 1, 0, 0, 0, 1001), "timer - 1 ms"));
		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateMicroseconds(1)),
			MSAPI::Timer::Create(2000, 1, 1, 0, 0, 0, 1), "timer - 1 us"));
		RETURN_IF_FALSE(t.Assert((timer = timer - MSAPI::Timer::Duration::CreateNanoseconds(1)),
			MSAPI::Timer::Create(2000), "timer - 1 ns"));

		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Duration::Create(1, 1, 1, 1, 1001001),
			MSAPI::Timer::Duration::CreateDays(1) + MSAPI::Timer::Duration::CreateHours(1)
				+ MSAPI::Timer::Duration::CreateMinutes(1) + MSAPI::Timer::Duration::CreateSeconds(1)
				+ MSAPI::Timer::Duration::CreateMilliseconds(1) + MSAPI::Timer::Duration::CreateMicroseconds(1)
				+ MSAPI::Timer::Duration::CreateNanoseconds(1),
			"Duration create sum"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Duration::Create(0, 22, 58, 58, 998998999),
			MSAPI::Timer::Duration::CreateDays(1) - MSAPI::Timer::Duration::CreateHours(1)
				- MSAPI::Timer::Duration::CreateMinutes(1) - MSAPI::Timer::Duration::CreateSeconds(1)
				- MSAPI::Timer::Duration::CreateMilliseconds(1) - MSAPI::Timer::Duration::CreateMicroseconds(1)
				- MSAPI::Timer::Duration::CreateNanoseconds(1),
			"Duration create diff"));

		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2000) + MSAPI::Timer::Duration::Create(1, 1, 1, 1, 1001001),
			MSAPI::Timer::Create(2000, 1, 2, 1, 1, 1, 1001001), "Timer + Duration create sum"));

		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Duration{}.Empty(), true, "Duration empty"));
	}

	{
		const auto duration{ MSAPI::Timer::Duration::CreateHours(23) };
		RETURN_IF_FALSE(t.Assert(duration.GetDays(), 0, "Get days 0"));
		RETURN_IF_FALSE(t.Assert(duration.GetHours(), 23, "Get hours 23"));
		RETURN_IF_FALSE(t.Assert(duration.GetMinutes(), 1380, "Get minutes 1380"));
		RETURN_IF_FALSE(t.Assert(duration.GetSeconds(), 82800, "Get seconds 82800"));
		RETURN_IF_FALSE(t.Assert(duration.GetMilliseconds(), 82800000, "Get ms 82800000"));
		RETURN_IF_FALSE(t.Assert(duration.GetMicroseconds(), 82800000000, "Get us 82800000000"));
		RETURN_IF_FALSE(t.Assert(duration.GetNanoseconds(), 82800000000000, "Get ns 82800000000000"));
	}

	{
		const MSAPI::Timer::Duration first{ MSAPI::Timer::Duration::CreateNanoseconds(1) },
			second{ MSAPI::Timer::Duration{ 2 } };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Duration < operator: 1 < 2"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Duration <= operator: 1 <= 2"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Duration > operator: 1 > 2"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Duration >= operator: 1 >= 2"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Duration == operator: 1 == 2"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Duration != operator: 1 != 2"));
	}
	{
		const MSAPI::Timer::Duration first{ MSAPI::Timer::Duration::CreateNanoseconds(1) },
			second{ MSAPI::Timer::Duration{ 1 } };
		RETURN_IF_FALSE(t.Assert(first < second, false, "Duration < operator: 1 < 1"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Duration <= operator: 1 <= 1"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Duration > operator: 1 > 1"));
		RETURN_IF_FALSE(t.Assert(first >= second, true, "Duration >= operator: 1 >= 1"));
		RETURN_IF_FALSE(t.Assert(first == second, true, "Duration == operator: 1 == 1"));
		RETURN_IF_FALSE(t.Assert(first != second, false, "Duration != operator: 1 != 1"));
	}
	{
		const MSAPI::Timer first{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 1) },
			second{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 2) };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Timer < operator: 1 < 2"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Timer <= operator: 1 <= 2"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Timer > operator: 1 > 2"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Timer >= operator: 1 >= 2"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Timer == operator: 1 == 2"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Timer != operator: 1 != 2"));
	}
	{
		const MSAPI::Timer first{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 1) },
			second{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 1) };
		RETURN_IF_FALSE(t.Assert(first < second, false, "Timer < operator: 1 < 1"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Timer <= operator: 1 <= 1"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Timer > operator: 1 > 1"));
		RETURN_IF_FALSE(t.Assert(first >= second, true, "Timer >= operator: 1 >= 1"));
		RETURN_IF_FALSE(t.Assert(first == second, true, "Timer == operator: 1 == 1"));
		RETURN_IF_FALSE(t.Assert(first != second, false, "Timer != operator: 1 != 1"));
	}

	{
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create("1978/1/1"), MSAPI::Timer::Create(1978), "Create timer from string 1978/1/1"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("1978 1/1//.klsdfjh"), MSAPI::Timer::Create(1978),
			"Create timer from string 1978 1/1//.klsdfjh"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("2222 9 21 17"), MSAPI::Timer::Create(2222, 9, 21, 17),
			"Create timer from string 2222 9 21 17"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("2222-9-21r17"), MSAPI::Timer::Create(2222, 9, 21, 17),
			"Create timer from string 2222-9-21r17"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("2222-9-21 17:39:::"), MSAPI::Timer::Create(2222, 9, 21, 17, 39),
			"Create timer from string 2222-9-21 17:39:::"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("2222r9g21o17w39o10dm  "),
			MSAPI::Timer::Create(2222, 9, 21, 17, 39, 10), "Create timer from string 2222r9g21o17w39o10dm  "));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("2224f9,21v17p39c10n123498765"),
			MSAPI::Timer::Create(2224, 9, 21, 17, 39, 10, 123498765),
			"Create timer from string 2224f9,21v17p39c10n123498765"));

		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("1978/1/0") == MSAPI::Timer{ 0 },
			MSAPI::Timer::Create(1978, 1, 0) == MSAPI::Timer{ 0 }, "Create timer from string with invalid day"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("1978/0/1") == MSAPI::Timer{ 0 },
			MSAPI::Timer::Create(1978, 0, 1) == MSAPI::Timer{ 0 }, "Create timer from string with invalid month"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("1978/0/"), MSAPI::Timer{ 0 },
			"Create timer from string without month and invalid day"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create("1978/"), MSAPI::Timer{ 0 }, "Create timer from string without month and day"));
		RETURN_IF_FALSE(t.Assert(
			MSAPI::Timer::Create("1969/"), MSAPI::Timer{ 0 }, "Create timer from string with year before 1970"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("2222  9-21r17"), MSAPI::Timer{ 0 },
			"Create timer from string with invalid format \"2222  9-21r17\""));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create("22229-21r17"), MSAPI::Timer{ 0 },
			"Create timer from string with invalid format \"22229-21r17\""));
	}

	{
		size_t one{ 0 };
		size_t two{ 0 };
		size_t three{ 0 };
		size_t four{ 0 };

		MSAPI::Timer::Event eventOne([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&one));
		MSAPI::Timer::Event eventTwo([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&two));
		MSAPI::Timer::Event eventThree([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&three));
		MSAPI::Timer::Event eventFour([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&four));

		eventOne.Start(1);
		eventTwo.Start(1, 0, true);
		eventThree.Start(1, 1, false);
		eventFour.Start(1, 6, false);

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(one, 1, "(1) event one == 1"));
		RETURN_IF_FALSE(t.Assert(!eventOne.IsRunning(), true, "(1) event one is stopped"));
		RETURN_IF_FALSE(t.Assert(two, 2, "(1) event two == 2"));
		RETURN_IF_FALSE(t.Assert(!eventTwo.IsRunning(), true, "(1) event two is stopped"));
		RETURN_IF_FALSE(t.Assert(three > 2, true, "(1) event three > 2"));
		RETURN_IF_FALSE(t.Assert(eventThree.IsRunning(), true, "(1) event three is running"));
		RETURN_IF_FALSE(t.Assert(four, 1, "(1) event four == 1"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(1) event four is running"));

		eventThree.Stop();
		RETURN_IF_FALSE(t.Assert(!eventThree.IsRunning(), true, "(1) event three is stopped"));

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(four, 2, "(1) event four == 2"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(1) event four is running after sleep"));

		eventFour.Stop();
		RETURN_IF_FALSE(t.Assert(!eventFour.IsRunning(), true, "(1) event four is stopped"));
	}

	{
		struct EventHandler : public MSAPI::Timer::Event::IHandler {
			int64_t value{};

			void HandleEvent([[maybe_unused]] const MSAPI::Timer::Event& event) override { ++value; }
		};

		EventHandler one;
		EventHandler two;
		EventHandler three;
		EventHandler four;

		MSAPI::Timer::Event eventOne(&one);
		MSAPI::Timer::Event eventTwo(&two);
		MSAPI::Timer::Event eventThree(&three);
		MSAPI::Timer::Event eventFour(&four);

		eventOne.Start(1);
		eventTwo.Start(1, 0, true);
		eventThree.Start(1, 1, false);
		eventFour.Start(1, 6, false);

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(one.value, 1, "(2) event one == 1"));
		RETURN_IF_FALSE(t.Assert(!eventOne.IsRunning(), true, "(2) event one is stopped"));
		RETURN_IF_FALSE(t.Assert(two.value, 2, "(2) event two == 2"));
		RETURN_IF_FALSE(t.Assert(!eventTwo.IsRunning(), true, "(2) event two is stopped"));
		RETURN_IF_FALSE(t.Assert(three.value > 2, true, "(2) event three > 2"));
		RETURN_IF_FALSE(t.Assert(eventThree.IsRunning(), true, "(2) event three is running"));
		RETURN_IF_FALSE(t.Assert(four.value, 1, "(2) event four == 1"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(2) event four is running"));

		eventThree.Stop();
		RETURN_IF_FALSE(t.Assert(!eventThree.IsRunning(), true, "(2) event three is stopped"));

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(four.value, 2, "(2) event four == 2"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(2) event four is running after sleep"));

		eventFour.Stop();
		RETURN_IF_FALSE(t.Assert(!eventFour.IsRunning(), true, "(2) event four is stopped"));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_TIMER_INL