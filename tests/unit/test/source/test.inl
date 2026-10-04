/**************************
 * @file        test.inl
 * @date        2026-10-03
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

#ifndef MSAPI_UNIT_TEST_TEST_INL
#define MSAPI_UNIT_TEST_TEST_INL

#include "../../../../library/source/test/test.inl"
#include <array>
#include <atomic>
#include <barrier>
#include <cmath>
#include <optional>
#include <thread>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Test.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool Test();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool Test()
{
	LOG_INFO("MSAPI UNIT TEST Test");

	MSAPI::Test::Test t;

	{
		// An empty subject has not passed, and integer results use process exit-code semantics
		MSAPI::Test::Test subject;
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), false, "Empty subject boolean result"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<int32_t>(), 1, "Empty subject exit code"));
		RETURN_IF_FALSE(t.Assert(subject.Assert(7, 7, "Subject success"), true, "Subject assertion succeeds"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), true, "Successful subject boolean result"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<int32_t>(), 0, "Successful subject exit code"));
		RETURN_IF_FALSE(t.Assert(subject.Assert(7, 8, "Expected subject failure"), false, "Subject assertion fails"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), false, "Failure remains registered"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<int32_t>(), 1, "Failed subject exit code"));
		RETURN_IF_FALSE(t.Assert(subject.Assert(8, 8, "Success after failure"), true, "Later assertion succeeds"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), false, "Later success does not erase failure"));
	}

	{
		// Destruction without assertions exercises the empty-report path
		MSAPI::Test::Test subject;
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), false, "Empty report subject"));
	}

	class Printable {
	private:
		int32_t m_value;

	public:
		FORCE_INLINE explicit Printable(const int32_t value) noexcept
			: m_value{ value }
		{
		}

		FORCE_INLINE [[nodiscard]] bool operator==(const Printable& other) const noexcept
		{
			return m_value == other.m_value;
		}

		FORCE_INLINE [[nodiscard]] std::string ToString() const { return std::to_string(m_value); }
	};

	class Comparable {
	private:
		int32_t m_value;

	public:
		FORCE_INLINE explicit Comparable(const int32_t value) noexcept
			: m_value{ value }
		{
		}

		FORCE_INLINE [[nodiscard]] bool operator==(const Comparable& other) const noexcept
		{
			return m_value == other.m_value;
		}
	};

	enum class Value : int32_t { First, Second };

	// Exercise matching and mismatching values without adding expected failures to the outer suite
	const auto checkValues{ [&t](const auto& first, const auto& second) -> bool {
		MSAPI::Test::Test subject;
		RETURN_IF_FALSE(t.Assert(subject.Assert(first, first, "Subject equal values"), true, "Equal values pass"));
		RETURN_IF_FALSE(
			t.Assert(subject.Assert(first, second, "Expected unequal values"), false, "Unequal values fail"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), false, "Type mismatch registers failure"));
		return true;
	} };

	RETURN_IF_FALSE(checkValues(int32_t{ -7 }, int64_t{ 8 }));
	RETURN_IF_FALSE(checkValues(true, false));
	RETURN_IF_FALSE(checkValues(Value::First, Value::Second));
	RETURN_IF_FALSE(checkValues(1.f, 2.f));
	RETURN_IF_FALSE(checkValues(1., 2.));
	RETURN_IF_FALSE(checkValues(std::string{ "first" }, std::string{ "second" }));
	RETURN_IF_FALSE(checkValues(std::string_view{ "first" }, std::string_view{ "second" }));
	RETURN_IF_FALSE(checkValues(std::wstring{ L"first" }, std::wstring{ L"second" }));
	RETURN_IF_FALSE(checkValues(std::wstring_view{ L"first" }, std::wstring_view{ L"second" }));
	RETURN_IF_FALSE(checkValues(MSAPI::Timer{ 123, 4 }, MSAPI::Timer{ 124, 5 }));
	RETURN_IF_FALSE(checkValues(MSAPI::Timer::Duration{ 10 }, MSAPI::Timer::Duration{ 20 }));
	RETURN_IF_FALSE(checkValues(Printable{ 1 }, Printable{ 2 }));
	RETURN_IF_FALSE(checkValues(Comparable{ 1 }, Comparable{ 2 }));
	RETURN_IF_FALSE(checkValues(std::optional<int32_t>{ 1 }, std::optional<int32_t>{ 2 }));
	RETURN_IF_FALSE(checkValues(std::optional<double>{ 1. }, std::optional<double>{ 2. }));

	// A missing optional value must mismatch a present value in either operand order
	RETURN_IF_FALSE(checkValues(std::optional<int32_t>{ 1 }, std::optional<int32_t>{}));
	RETURN_IF_FALSE(checkValues(std::optional<int32_t>{}, std::optional<int32_t>{ 1 }));
	RETURN_IF_FALSE(checkValues(std::optional<double>{ 1. }, std::optional<double>{}));
	RETURN_IF_FALSE(checkValues(std::optional<double>{}, std::optional<double>{ 1. }));

	{
		// Floating-point comparisons honor tolerance, including when both optionals have values
		MSAPI::Test::Test subject;
		const auto adjacent{ std::nextafter(1., 2.) };
		RETURN_IF_FALSE(t.Assert(subject.Assert(adjacent, 1., "FP tolerance"), true, "Adjacent doubles pass"));
		RETURN_IF_FALSE(t.Assert(
			subject.Assert(std::optional<double>{ adjacent }, std::optional<double>{ 1. }, "Optional FP tolerance"),
			true, "Optional adjacent doubles pass"));
		RETURN_IF_FALSE(
			t.Assert(subject.Assert(std::optional<int32_t>{}, std::optional<int32_t>{}, "Empty integer optionals"),
				true, "Two empty integer optionals pass"));
		RETURN_IF_FALSE(t.Assert(subject.Assert(std::optional<double>{}, std::optional<double>{}, "Empty FP optionals"),
			true, "Two empty FP optionals pass"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), true, "Tolerance and empty optional subject passes"));
	}

	{
		// Pointer comparisons are checked through boolean assertion results in the outer suite
		MSAPI::Test::Test subject;
		int32_t first{};
		int32_t second{};
		RETURN_IF_FALSE(t.Assert(subject.Assert(&first, &first, "Same address"), true, "Same pointers pass"));
		RETURN_IF_FALSE(t.Assert(
			subject.Assert(&first, &second, "Expected different addresses"), false, "Different pointers fail"));
	}

	{
		// Polling covers immediate success, delayed success, forwarded arguments, and unlocked getters
		MSAPI::Test::Test subject;
		RETURN_IF_FALSE(t.Assert(subject.Wait(
									 0, [] { return int32_t{ 7 }; }, int32_t{ 7 }, "Immediate wait"),
			true, "Zero-budget matching wait"));

		int32_t calls{};
		RETURN_IF_FALSE(t.Assert(subject.Wait(
									 1000, [&calls] { return ++calls; }, int32_t{ 3 }, "Delayed wait"),
			true, "Delayed matching wait"));
		RETURN_IF_FALSE(t.Assert(calls, 3, "Wait registers captured matching value"));

		RETURN_IF_FALSE(
			t.Assert(subject.Wait(
						 0, [](const int32_t value) { return value; }, int32_t{ 9 }, "Forwarded getter", int32_t{ 9 }),
				true, "Wait forwards arguments"));
		RETURN_IF_FALSE(t.Assert(subject.Wait(
									 0, [&subject] { return subject.Passed<bool>(); }, true, "Getter reads subject"),
			true, "Getter executes outside Test lock"));

		std::atomic<int32_t> ready{ 1 };
		RETURN_IF_FALSE(t.Assert(subject.Wait(
									 0, [&ready] { return ready.load(); }, int32_t{ 1 }, "Atomic getter"),
			true, "Wait accepts synchronized getter"));
	}

	{
		// Timeout failures remain isolated from the passing outer suite
		MSAPI::Test::Test subject;
		RETURN_IF_FALSE(t.Assert(subject.Wait(
									 0, [] { return false; }, true, "Expected zero-budget timeout"),
			false, "Zero-budget mismatching wait"));
		RETURN_IF_FALSE(t.Assert(subject.Wait(
									 200, [] { return false; }, true, "Expected polling timeout"),
			false, "Positive-budget timeout"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<int32_t>(), 1, "Timeout registers failure"));
	}

	{
		// Concurrent mismatches must be registered without corrupting the outer suite
		MSAPI::Test::Test subject;
		std::barrier start{ 4 };
		std::array<bool, 4> succeeded{ true, true, true, true };
		std::array<std::jthread, 4> workers;

		for (int32_t worker{}; worker < 4; ++worker) {
			workers[UINT64(worker)] = std::jthread{ [&, worker] {
				start.arrive_and_wait();
				for (int32_t iteration{}; iteration < 8; ++iteration) {
					const bool expected{ iteration % 2 == 0 };
					const bool registered{ t.Assert(subject.Assert(expected, true, "Concurrent subject result"),
						expected, "Concurrent assertion result") };
					succeeded[UINT64(worker)] = registered && succeeded[UINT64(worker)];
				}
			} };
		}

		for (auto& worker : workers) {
			worker.join();
		}

		for (const bool success : succeeded) {
			RETURN_IF_FALSE(t.Assert(success, true, "Concurrent failure registration"));
		}

		RETURN_IF_FALSE(t.Assert(subject.Passed<bool>(), false, "Concurrent subject retains failures"));
		RETURN_IF_FALSE(t.Assert(subject.Passed<int32_t>(), 1, "Concurrent subject failure exit code"));
	}

	{
		// Exercise concurrent assertions, condition polling, and result inspection on one Test instance
		std::barrier start{ 4 };
		std::array<bool, 4> succeeded{ true, true, true, true };
		std::array<std::jthread, 4> workers;

		for (int32_t worker{}; worker < 4; ++worker) {
			workers[UINT64(worker)] = std::jthread{ [&, worker] {
				start.arrive_and_wait();
				for (int32_t iteration{}; iteration < 8; ++iteration) {
					const bool asserted{ t.Assert(iteration, iteration, "Concurrent Test assertion") };
					const bool waited{ t.Wait(
						0, [iteration] { return iteration; }, iteration, "Concurrent Test wait") };
					const bool passed{ t.Passed<bool>() };
					succeeded[UINT64(worker)] = asserted && waited && passed && succeeded[UINT64(worker)];
				}
			} };
		}

		for (auto& worker : workers) {
			worker.join();
		}

		for (const bool success : succeeded) {
			RETURN_IF_FALSE(t.Assert(success, true, "Concurrent Test methods"));
		}
	}

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_TEST_INL