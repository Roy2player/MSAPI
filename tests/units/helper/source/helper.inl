/**************************
 * @file        helper.inl
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

#ifndef MSAPI_UNIT_TEST_HELPER_INL
#define MSAPI_UNIT_TEST_HELPER_INL

#include "../../../../library/source/help/helper.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Helper.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Helper();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Helper()
{
	static_assert(MSAPI::Helper::caseSensitive, "caseSensitive should be true");
	static_assert(!MSAPI::Helper::caseInsensitive, "caseInsensitive should be false");

	LOG_INFO_UNITTEST("MSAPI Helper");
	MSAPI::Test t;

	{
		const auto compareFloats{ [&t]<typename T> [[nodiscard]] (const T a, const T b, const int expected) {
			RETURN_IF_FALSE(t.Assert(
				MSAPI::Helper::CompareFloats(a, b), expected, std::format("Compare floats, {} and {}", _S(a), _S(b))));

			if (expected == 0) {
				return t.Assert(
					MSAPI::Helper::FloatEqual(a, b), true, std::format("Float equal, {} and {}", _S(a), _S(b)));
			}
			if (expected == 1) {
				return t.Assert(
					MSAPI::Helper::FloatGreater(a, b), true, std::format("Float greater, {} and {}", _S(a), _S(b)));
			}
			return t.Assert(MSAPI::Helper::FloatLess(a, b), true, std::format("Float less, {} and {}", _S(a), _S(b)));
		} };

		RETURN_IF_FALSE(compareFloats(1.0f, 1.0f, 0));
		RETURN_IF_FALSE(compareFloats(1.0, 1.0, 0));
		RETURN_IF_FALSE(compareFloats(1.0L, 1.0L, 0));

		RETURN_IF_FALSE(compareFloats(1.0f + 1.0E-7f, 1.0f, 0));
		RETURN_IF_FALSE(compareFloats(1.0 + 1.0E-11, 1.0, 0));
		RETURN_IF_FALSE(compareFloats(1.0L + 1.0E-21L, 1.0L, 0));

		RETURN_IF_FALSE(compareFloats(1.0f - 1.0E-7f, 1.0f, 0));
		RETURN_IF_FALSE(compareFloats(1.0 - 1.0E-11, 1.0, 0));
		RETURN_IF_FALSE(compareFloats(1.0L - 1.0E-21L, 1.0L, 0));

		RETURN_IF_FALSE(compareFloats(1.0f + 1.0E-5f, 1.0f, 1));
		RETURN_IF_FALSE(compareFloats(1.0 + 1.0E-9, 1.0, 1));
		RETURN_IF_FALSE(compareFloats(1.0L + 1.0E-19L, 1.0L, 1));

		RETURN_IF_FALSE(compareFloats(1.0f - 1.0E-5f, 1.0f, -1));
		RETURN_IF_FALSE(compareFloats(1.0 - 1.0E-9, 1.0, -1));
		RETURN_IF_FALSE(compareFloats(1.0L - 1.0E-19L, 1.0L, -1));
	}

	{
#define TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(a, b, expected, epsilon)                                   \
	{                                                                                                                  \
		constexpr auto e = epsilon;                                                                                    \
		RETURN_IF_FALSE(t.Assert(MSAPI::Helper::CompareFloats<decltype(a), e>(a, b), expected,                         \
			std::format("Compare floats, {} and {}. Epsilon: {}", _S(a), _S(b), _S(epsilon))));                        \
                                                                                                                       \
		if (expected == 0) {                                                                                           \
			return t.Assert(MSAPI::Helper::FloatEqual<decltype(a), e>(a, b), true,                                     \
				std::format("Float equal, {} and {}. Epsilon: {}", _S(a), _S(b), _S(epsilon)));                        \
		}                                                                                                              \
		if (expected == 1) {                                                                                           \
			return t.Assert(MSAPI::Helper::FloatGreater<decltype(a), e>(a, b), true,                                   \
				std::format("Float greater, {} and {}. Epsilon: {}", _S(a), _S(b), _S(epsilon)));                      \
		}                                                                                                              \
		return t.Assert(MSAPI::Helper::FloatLess<decltype(a), e>(a, b), true,                                          \
			std::format("Float less, {} and {}. Epsilon: {}", _S(a), _S(b), _S(epsilon)));                             \
	}

		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0f + 1.0E-5f, 1.0f, 0, 1.0E-4f);
		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0 + 1.0E-9, 1.0, 0, 1.0E-8);
		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0L + 1.0E-19L, 1.0L, 0, 1.0E-18L);

		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0f + 1.0E-5f, 1.0f, 1, 1.0E-6f);
		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0 + 1.0E-9, 1.0, 1, 1.0E-12);
		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0L + 1.0E-19L, 1.0L, 1, 1.0E-22L);

		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0f + 1.0E-5f, 1.0f, 1, 1.0E-5f);
		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0 + 1.0E-9, 1.0, 1, 1.0E-9);
		TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON(1.0L + 1.0E-19L, 1.0L, 1, 1.0E-19L);

#undef TMP_MSAPI_HELPER_COMPARE_FLOATS_WITH_CUSTOM_EPSILON
	}

	{
		const auto roundDouble{ [&t] [[nodiscard]] (double value, double tick, double result) {
			return t.Assert(
				MSAPI::Helper::Round(value, tick), result, std::format("Round double for tick {}", _S(tick)));
		} };
		const auto roundInt32t{ [&t] [[nodiscard]] (int32_t value, double tick, int32_t result) {
			return t.Assert(
				MSAPI::Helper::Round(value, tick), result, std::format("Round int32_t for tick {}", _S(tick)));
		} };
		const auto roundUp{ [&t] [[nodiscard]] (double value, size_t accuracy, double result) {
			return t.Assert(
				MSAPI::Helper::RoundUp(value, accuracy), result, std::format("RoundUp for accuracy {}", _S(accuracy)));
		} };
		const auto roundDown{ [&t] [[nodiscard]] (double value, size_t accuracy, double result) {
			return t.Assert(MSAPI::Helper::RoundDown(value, accuracy), result,
				std::format("RoundDown for accuracy {}", _S(accuracy)));
		} };

		RETURN_IF_FALSE(roundDouble(1.001, 0.001, 1.001));
		RETURN_IF_FALSE(roundDouble(1.00101, 0.001, 1.001));
		RETURN_IF_FALSE(roundDouble(100.0, 0.001, 100));
		RETURN_IF_FALSE(roundDouble(100.001911, 0.001, 100.001));

		RETURN_IF_FALSE(roundInt32t(129999999, 0.01, 130000000));
		RETURN_IF_FALSE(roundInt32t(750000000, 0.01, 750000000));
		RETURN_IF_FALSE(roundInt32t(750004320, 0.01, 750000000));
		RETURN_IF_FALSE(roundInt32t(751000000, 0.01, 750000000));
		RETURN_IF_FALSE(roundInt32t(749000000, 0.01, 750000000));
		RETURN_IF_FALSE(roundInt32t(749999999, 0.01, 750000000));
		RETURN_IF_FALSE(roundInt32t(880910000, 0.0001, 880900000));

		RETURN_IF_FALSE(roundDown(0.002029, 2, 0));
		RETURN_IF_FALSE(roundDown(0.449999, 2, 0.44));
		RETURN_IF_FALSE(roundDown(0.045999, 2, 0.04));
		RETURN_IF_FALSE(roundDown(0.099999, 2, 0.09));
		RETURN_IF_FALSE(roundDown(0.999999, 0, 0));
		RETURN_IF_FALSE(roundDown(1.099999, 0, 1));

		RETURN_IF_FALSE(roundUp(0.002029, 2, 0.01));
		RETURN_IF_FALSE(roundUp(0.449999, 2, 0.45));
		RETURN_IF_FALSE(roundUp(0.045999, 2, 0.05));
		RETURN_IF_FALSE(roundUp(0.099999, 2, 0.1));
		RETURN_IF_FALSE(roundUp(0.999999, 0, 1));
		RETURN_IF_FALSE(roundUp(1.099999, 0, 2));
	}

	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(1, 1, 2, 2, 1, 2), 1, "WhereIsPoint test 1"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(1, 1, 2, 2, 2, 1), -1, "WhereIsPoint test 2"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(1, 1, 2, 2, 3, 3), 0, "WhereIsPoint test 3"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 1, 1, 2), 1, "WhereIsPoint test 4"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 1, 2, 1), -1, "WhereIsPoint test 5"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 1, 3, 3), 0, "WhereIsPoint test 6"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(-1, -1, -2, -2, -1, -2), -1, "WhereIsPoint test 7"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(-1, -1, -2, -2, -2, -1), 1, "WhereIsPoint test 8"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(-1, -1, -2, -2, -3, -3), 0, "WhereIsPoint test 9"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(-2, -2, -1, -1, -1, -2), -1, "WhereIsPoint test 10"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(-2, -2, -1, -1, -2, -1), 1, "WhereIsPoint test 11"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(-2, -2, -1, -1, -3, -3), 0, "WhereIsPoint test 12"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(1, 1, 2, 2, -1, -2), -1, "WhereIsPoint test 13"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(1, 1, 2, 2, -2, -1), 1, "WhereIsPoint test 14"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(1, 1, 2, 2, -3, -3), 0, "WhereIsPoint test 15"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 1, -1, -2), -1, "WhereIsPoint test 16"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 1, -2, -1), 1, "WhereIsPoint test 17"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 1, -3, -3), 0, "WhereIsPoint test 18"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 2, 1, -2, -1), -2, "WhereIsPoint test 19"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Helper::WhereIsPoint(2, 2, 1, 2, -3, -3), -2, "WhereIsPoint test 20"));

	{
		const auto checkEmail{ [&t] [[nodiscard]] (const std::string_view email, const bool expected) {
			return t.Assert(MSAPI::Helper::ValidateEmail(email), expected, std::format("Validate email \"{}\"", email));
		} };

		RETURN_IF_FALSE(checkEmail("t@m.", false));
		RETURN_IF_FALSE(checkEmail("t@m", false));
		RETURN_IF_FALSE(checkEmail("t@m.c", false));
		RETURN_IF_FALSE(checkEmail("t@m.ce", true));
		RETURN_IF_FALSE(checkEmail("t@.c", false));
		RETURN_IF_FALSE(checkEmail("t.c@m.c", false));
		RETURN_IF_FALSE(checkEmail("t@m.c.", false));
		RETURN_IF_FALSE(checkEmail("@m.c", false));
		RETURN_IF_FALSE(checkEmail("t@m.c@", false));
		RETURN_IF_FALSE(checkEmail(".c@", false));
		RETURN_IF_FALSE(checkEmail(".@", false));
		RETURN_IF_FALSE(checkEmail("@.", false));
		RETURN_IF_FALSE(checkEmail("@.c", false));
		RETURN_IF_FALSE(checkEmail("2.3@3.ce", true));
		RETURN_IF_FALSE(checkEmail("2.3@3..ce", false));
		RETURN_IF_FALSE(checkEmail("2..3@3.ce", false));
		RETURN_IF_FALSE(checkEmail("simple@example.com", true));
		RETURN_IF_FALSE(checkEmail("very.common@example.com", true));
		RETURN_IF_FALSE(checkEmail("disposable.style.email.with+symbol@example.com", false));
		RETURN_IF_FALSE(checkEmail("other.email-with-hyphen@example.com", true));
		RETURN_IF_FALSE(checkEmail("fully-qualified-domain@example.com", true));
		RETURN_IF_FALSE(checkEmail("user.name+tag+sorting@example.com", false));
		RETURN_IF_FALSE(checkEmail("x@example.com", true));
		RETURN_IF_FALSE(checkEmail("example-indeed@strange-example.com", true));
		RETURN_IF_FALSE(checkEmail("admin@mailserver1", false));
		RETURN_IF_FALSE(checkEmail("mailhost!username@example.org", false));
		RETURN_IF_FALSE(checkEmail("user%example.com@example.org", false));
		RETURN_IF_FALSE(checkEmail("plainaddress", false));
		RETURN_IF_FALSE(checkEmail("@missingusername.com", false));
		RETURN_IF_FALSE(checkEmail("username@.com", false));
		RETURN_IF_FALSE(checkEmail("username@.com.", false));
		RETURN_IF_FALSE(checkEmail("username@.com..com", false));
		RETURN_IF_FALSE(checkEmail("username@.com.-com", false));
		RETURN_IF_FALSE(checkEmail(".username@example.com", false));
		RETURN_IF_FALSE(checkEmail("username@example.com.", false));
		RETURN_IF_FALSE(checkEmail("username@example.com..com", false));
		RETURN_IF_FALSE(checkEmail("username@-example.com", false));
		RETURN_IF_FALSE(checkEmail("username@111.222.333.44444", false));
		RETURN_IF_FALSE(checkEmail("username@example..com", false));
		RETURN_IF_FALSE(checkEmail("username@.com", false));
		RETURN_IF_FALSE(checkEmail("username@-example.com", false));
		RETURN_IF_FALSE(checkEmail("username@example.com (Joe Smith)", false));
		RETURN_IF_FALSE(checkEmail("username@example@example.com", false));
		RETURN_IF_FALSE(checkEmail("username@example..com", false));
		RETURN_IF_FALSE(checkEmail("username@example.c", false));
		RETURN_IF_FALSE(checkEmail("username@example.toolongtld", true));
		RETURN_IF_FALSE(checkEmail("username@.com.my", false));
		RETURN_IF_FALSE(checkEmail("username@.com.com", false));
		RETURN_IF_FALSE(checkEmail("username@..com.com", false));
		RETURN_IF_FALSE(checkEmail("username@-example.com", false));
		RETURN_IF_FALSE(checkEmail("username@111.222.333.44444", false));
		RETURN_IF_FALSE(checkEmail("username@example.com.1a", false));
		RETURN_IF_FALSE(checkEmail("username@example.com.1", false));
		RETURN_IF_FALSE(checkEmail("username@..com", false));
		RETURN_IF_FALSE(checkEmail("username@example@example.com", false));
		RETURN_IF_FALSE(checkEmail("username@example@domain.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com@domain.com", false));
		RETURN_IF_FALSE(checkEmail("username@.domain.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain..com", false));
		RETURN_IF_FALSE(checkEmail("username@.domain..com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com.", false));
		RETURN_IF_FALSE(checkEmail("username@-domain.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain-.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.c", false));
		RETURN_IF_FALSE(checkEmail("username@domain.co1", false));
		RETURN_IF_FALSE(checkEmail("username@domain.c1", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com.", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com..", false));
		RETURN_IF_FALSE(checkEmail("username@domain..com", false));
		RETURN_IF_FALSE(checkEmail("username@..domain.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com.com", true));
		RETURN_IF_FALSE(checkEmail("username@domain..com.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com..com", false));
		RETURN_IF_FALSE(checkEmail("username@domain..com.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com.-com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com.-com.com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com..com", false));
		RETURN_IF_FALSE(checkEmail("username@domain.com.-com.com", false));
	}

	{
		const auto checkUtf8AndWstring{ [&t] [[nodiscard]] (const char* cstr, const wchar_t* wcstr) {
			const auto wsresult{ MSAPI::Helper::StringToWstring(cstr) };
			RETURN_IF_FALSE(t.Assert(std::wstring_view{ wsresult }, std::wstring_view{ wcstr },
				"Transformation from UTF-8 (char) to wstring"))
			const auto sresult{ MSAPI::Helper::WstringToString(wcstr) };
			return t.Assert(
				std::string_view{ sresult }, std::string_view{ cstr }, "Transformation from wstring to UTF-8 (char)");
		} };

		RETURN_IF_FALSE(checkUtf8AndWstring("Hello, world!", L"Hello, world!"));

		RETURN_IF_FALSE(
			t.Assert(MSAPI::Helper::StringToWstring(nullptr), std::wstring_view{ L"" }, "nullptr string to wstring"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Helper::StringToWstring("\0"), std::wstring_view{ L"" }, "\\0 string to wstring"));

		RETURN_IF_FALSE(
			t.Assert(MSAPI::Helper::WstringToString(nullptr), std::string_view{ "" }, "nullptr wstring to string"));
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Helper::WstringToString(L"\0"), std::string_view{ "" }, "\\0 wstring to string"));
	}

	{
		const auto checkNormalizeOctalEscapedUtf8{ [&t] [[nodiscard]] (
													   const char* sequence, std::string_view expected) {
			const auto result{ MSAPI::Helper::NormalizeOctalEscapedUtf8(sequence) };
			return t.Assert(std::string_view{ result }, expected, "Normalize octal-escaped UTF-8 sequence");
		} };

		RETURN_IF_FALSE(checkNormalizeOctalEscapedUtf8("Pilgrim\'s Pride Corp", "Pilgrim's Pride Corp"));
		RETURN_IF_FALSE(checkNormalizeOctalEscapedUtf8(
			"\320\241\320\276\320\265\320\264\320\270\320\275\320\265\320\275\320\275\321\213\320\265 "
			"\320\250\321\202\320\260\321\202\321\213 \320\220\320\274\320\265\321\200\320\270\320\272\320\270",
			"Соединенные Штаты Америки"));
		RETURN_IF_FALSE(
			checkNormalizeOctalEscapedUtf8("\320\220\320\224\320\240 Koninklijke Philips", "АДР Koninklijke Philips"));
	}

	{
		struct Exponent10DataInt {
			int64_t value;
			int64_t result;
		};

		std::vector<Exponent10DataInt> testData{ { -1777777777777777777, 18 }, { -1000000000000000000, 18 },
			{ -999999999999999999, 17 }, { -100000000000000000, 17 }, { -99999999999999999, 16 },
			{ -10000000000000000, 16 }, { -9999999999999999, 15 }, { -1000000000000000, 15 }, { -999999999999999, 14 },
			{ -100000000000000, 14 }, { -99999999999999, 13 }, { -10000000000000, 13 }, { -9999999999999, 12 },
			{ -1000000000000, 12 }, { -999999999999, 11 }, { -100000000000, 11 }, { -99999999999, 10 },
			{ -10000000000, 10 }, { -9999999999, 9 }, { -1000000000, 9 }, { -999999999, 8 }, { -100000000, 8 },
			{ -99999999, 7 }, { -10000000, 7 }, { -9999999, 6 }, { -1000000, 6 }, { -999999, 5 }, { -100000, 5 },
			{ -99999, 4 }, { -10000, 4 }, { -9999, 3 }, { -1000, 3 }, { -999, 2 }, { -100, 2 }, { -99, 1 }, { -10, 1 },
			{ -9, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 }, { 9, 0 }, { 10, 1 }, { 99, 1 }, { 100, 2 }, { 999, 2 },
			{ 1000, 3 }, { 9999, 3 }, { 10000, 4 }, { 99999, 4 }, { 100000, 5 }, { 999999, 5 }, { 1000000, 6 },
			{ 9999999, 6 }, { 10000000, 7 }, { 99999999, 7 }, { 100000000, 8 }, { 999999999, 8 }, { 1000000000, 9 },
			{ 9999999999, 9 }, { 10000000000, 10 }, { 99999999999, 10 }, { 100000000000, 11 }, { 999999999999, 11 },
			{ 1000000000000, 12 }, { 9999999999999, 12 }, { 10000000000000, 13 }, { 99999999999999, 13 },
			{ 100000000000000, 14 }, { 999999999999999, 14 }, { 1000000000000000, 15 }, { 9999999999999999, 15 },
			{ 10000000000000000, 16 }, { 99999999999999999, 16 }, { 100000000000000000, 17 },
			{ 999999999999999999, 17 }, { 1000000000000000000, 18 }, { 1777777777777777777, 18 } };

		for (const auto& data : testData) {
			RETURN_IF_FALSE(t.Assert(MSAPI::Helper::Exponent10Of(data.value), data.result,
				std::format("Exponent10Of for {}", _S(data.value))));
		}
	}

	return true;
};

}; // namespace UnitTest

}; // namespace MSAPI

#endif // MSAPI_UNIT_TEST_HELPER_INL