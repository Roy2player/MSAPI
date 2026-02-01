/**************************
 * @file        test.h
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

#ifndef MSAPI_TEST_H
#define MSAPI_TEST_H

#include "../help/helper.h"
#include "../help/log.h"
#include "../help/table.h"
#include <vector>

#define RETURN_IF_FALSE(x)                                                                                             \
	if (!x) [[unlikely]] {                                                                                             \
		LOG_ERROR("Test is going to fail");                                                                            \
		return false;                                                                                                  \
	}

#define LOG_INFO_UNITTEST(name) LOG_INFO_NEW("UNITTEST  : {}", name);

namespace MSAPI {

template <typename T, typename S>
concept comparable = requires(T t, S s) {
	{ t == s } -> std::same_as<bool>;
};

template <typename T>
concept has_to_string = requires(T t) {
	{ t.ToString() } -> std::same_as<std::string>;
};

/**************************
 * @brief Class for registration compare tests, save their results and whole test time duration from creating Test
 * object to destroying it. Results will be printed in INFO level logs and in standard output after removing Test
 * object. Standard output message format: "Passed/Failed, elapsed wall time: X ns".
 *
 * @todo Currently that class cannot be used for performance check as here are a lot of memory allocation which
 * alway will impact a result. So, when log level greater than INFO, no allocation must exist.
 */
class Test {
private:
	int32_t m_counter{};
	int32_t m_passedCounter{};
	Timer m_timer;
	Timer m_totalTimer;

	static constexpr std::string_view m_patternPassed{ "\033[0;32mPASSED: \033[0m{}. {} ns" };
	static constexpr std::string_view m_patternFailed{ "\033[0;31mFAILED: \033[0m{}. Actual: {}. Expected: {}. {} ns" };

public:
	/**************************
	 * @brief Print in INFO level logs results of all tests.
	 */
	~Test();

	/**************************
	 * @tparam T Boolean or integer type.
	 *
	 * @return 0 (true) if there is at least one test and all tests passed successfully, 1 (false) in another way.
	 */
	template <typename T>
		requires(std::is_same_v<T, bool> || MSAPI::is_integer_type<T>)
	FORCE_INLINE T Passed() const
	{
		if constexpr (std::is_same_v<T, bool>) {
			return m_counter == m_passedCounter && m_counter > 0;
		}
		else {
			return m_counter == m_passedCounter && m_counter > 0 ? 0 : 1;
		}
	}

	/**************************
	 * @brief Registers the assertion of couple values and save result.
	 *
	 * @tparam T Any standard type, wstring, string/wstring view or type with "string ToString()" method.
	 * @tparam S Any standard type, wstring, string/wstring view or type with "string ToString()" method and comparable
	 * with T.
	 *
	 * @param actual Actual value.
	 * @param expected Expected value.
	 * @param name Assertion name.
	 *
	 * @return True if assertion passed successfully, false in another way.
	 */
	template <typename T, typename S>
		requires comparable<T, S>
	FORCE_INLINE bool Assert(T&& actual, S&& expected, const std::string_view name)
	{
		++m_counter;

		using N = std::decay_t<T>;
		using Z = remove_optional_t<N>;

		using G = std::conditional_t<is_greater_type<Z, remove_optional_t<std::decay_t<S>>>, safe_underlying_type_t<Z>,
			safe_underlying_type_t<remove_optional_t<std::decay_t<S>>>>;

		if constexpr (is_integer_type<N> || std::is_same_v<N, bool> || std::is_enum_v<N> || std::is_enum_v<Z>) {
			if (static_cast<G>(actual) == static_cast<G>(expected)) [[likely]] {

#define TMP_MSAPI_TEST_ASSERT_SUCCESS                                                                                  \
	LOG_INFO_NEW(m_patternPassed, name, Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());                        \
	m_timer.Reset();                                                                                                   \
	++m_passedCounter;                                                                                                 \
	return true;

				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(
				m_patternFailed, name, _S(actual), _S(expected), Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (is_integer_type_optional<N>) {
			if (const bool valuesPresented{ actual.has_value() && expected.has_value() };
				!valuesPresented || static_cast<G>(actual.value()) == static_cast<G>(expected.value())) [[likely]] {

				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(
				m_patternFailed, name, _S(actual), _S(expected), Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (is_float_type<N>) {
			if (MSAPI::Helper::FloatEqual(static_cast<G>(actual), static_cast<G>(expected))) [[likely]] {
				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(
				m_patternFailed, name, _S(actual), _S(expected), Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (is_float_type_optional<N>) {
			if (const bool valuesPresented{ actual.has_value() && expected.has_value() }; !valuesPresented
				|| MSAPI::Helper::FloatEqual(static_cast<G>(actual.value()), static_cast<G>(expected.value())))
				[[likely]] {

				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(
				m_patternFailed, name, _S(actual), _S(expected), Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (std::is_same_v<N, std::string> || std::is_same_v<N, std::string_view>) {
			if (actual == expected) [[likely]] {
				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(
				m_patternFailed, name, actual, expected, Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (std::is_same_v<N, std::wstring> || std::is_same_v<N, std::wstring_view>) {
			if (actual == expected) [[likely]] {
				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}

			std::string actualString;
			if constexpr (std::is_same_v<std::decay_t<N>, std::wstring>) {
				actualString = Helper::WstringToString(actual.c_str());
			}
			else {
				actualString = Helper::WstringToString(std::wstring{ actual }.c_str());
			}
			std::string expectedString;
			if constexpr (std::is_same_v<std::decay_t<S>, std::wstring>) {
				expectedString = Helper::WstringToString(expected.c_str());
			}
			else {
				expectedString = Helper::WstringToString(std::wstring{ expected }.c_str());
			}
			LOG_INFO_NEW(m_patternFailed, name, actualString, expectedString,
				Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (std::is_same_v<N, MSAPI::Timer> || std::is_same_v<N, MSAPI::Timer::Duration>) {
			if (actual == expected) [[likely]] {
				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(m_patternFailed, name, actual.ToString(), expected.ToString(),
				Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else if constexpr (has_to_string<T> && has_to_string<S>) {
			if (actual == expected) [[likely]] {
				TMP_MSAPI_TEST_ASSERT_SUCCESS;
			}
			LOG_INFO_NEW(m_patternFailed, name, actual.ToString(), expected.ToString(),
				Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}
		else {
			if (actual == expected) [[likely]] {
				TMP_MSAPI_TEST_ASSERT_SUCCESS;
#undef TMP_MSAPI_TEST_ASSERT_SUCCESS
			}
			LOG_INFO_NEW(m_patternFailed, name, "<unprintable>", "<unprintable>",
				Timer::Duration{ Timer{} - m_timer }.GetNanoseconds());
		}

		m_timer.Reset();
		return false;
	}

	/**************************
	 * @brief Wait for particular condition.
	 *
	 * @param waitTime Maximum amount of time to wait predicate in microseconds.
	 * @param predicate Condition to wait.
	 */
	static void Wait(size_t waitTime, const std::function<bool()>& predicate);
};

}; //* namespace MSAPI

#endif //* MSAPI_TEST_H