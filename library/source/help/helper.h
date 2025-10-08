/**************************
 * @file        helper.h
 * @version     6.0
 * @date        2023-09-24
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
 *
 * @brief Common helper functions.
 */

#ifndef MSAPI_HELPER_H
#define MSAPI_HELPER_H

#include "log.h"
#include "meta.hpp"
#include <arpa/inet.h>
#include <codecvt>
#include <concepts>
#include <locale>
#include <set>
#include <string>
#include <vector>

namespace MSAPI {

namespace Helper {

constexpr std::hash<std::string_view> stringHasher;

template <std::floating_point T> struct Epsilon { };

template <> struct Epsilon<float> {
	static constexpr float value = 1.0E-6f;
};

template <> struct Epsilon<double> {
	static constexpr double value = 1.0E-10;
};

template <> struct Epsilon<long double> {
	static constexpr long double value = 1.0E-20L;
};

/**************************
 * @brief Check if string is email.
 *
 * @param email String for checking.
 *
 * @return True if string is email, false otherwise.
 *
 * @test Has unit tests.
 */
bool ValidateEmail(std::string_view email);

/**************************
 * @brief Check if string is valid IPv4.
 *
 * @param ip String for checking.
 *
 * @return True if string is ip, false otherwise.
 */
bool ValidateIpv4(const char* ip);

/**************************
 * @brief Check if string is valid IPv6.
 *
 * @param ip String for checking.
 *
 * @return True if string is ip, false otherwise.
 */
bool ValidateIpv6(const char* ip);

/**************************
 * @brief Get path of executable directory with \ at the end and print error if failed.
 *
 * @param path Path for saving. Maximum size of read is the path capacity. If failed, path will be empty.
 */
void GetExecutableDir(std::string& path);

/**************************
 * @brief Transform domain to ip.
 *
 * @param domain Domain name.
 */
std::string DomainToIp(const char* domain);

/**************************
 * @brief Separate string by symbol.
 *
 * @param container Container for saving separated strings.
 * @param str String for separating.
 * @param symbol Symbol for separating.
 */
void Separating(std::vector<std::string>& container, const std::string& str, char symbol);

/**************************
 * @brief Separate string by symbol.
 *
 * @param container Container for saving separated strings.
 * @param str String for separating.
 * @param symbol Symbol for separating.
 */
void Separating(std::set<std::string>& container, const std::string& str, char symbol);

/**************************
 * @brief Check if string contains substring.
 *
 * @tparam caseSensetive - If true, then case sensetive, otherwise not.
 *
 * @param base Base string for checking.
 * @param sub Substring for checking.
 *
 * @return True if base contains sub, false otherwise.
 *
 * @todo Add unit tests.
 */
template <bool caseSensetive> FORCE_INLINE bool ContainsStr(const std::string& base, const std::string& sub)
{
	size_t index{ 0 };
	const auto size{ sub.size() };
	for (const auto& symbol : base) {
		if constexpr (caseSensetive) {
			if (symbol == sub[index]) {
				if (++index == size) {
					return true;
				}
				continue;
			}
			index = 0;
		}
		else {
			if (std::tolower(symbol) == std::tolower(sub[index])) {
				if (++index == size) {
					return true;
				}
				continue;
			}
			index = 0;
		}
	}
	return false;
}

/**************************
 * @brief Transform hex string to decimal int.
 *
 * @note Register is not important.
 *
 * @param str Hex string.
 *
 * @return Decimal int if str is hex string, std::nullopt otherwise.
 */
std::optional<int> HexStrToDecimal(const std::string& str);

/**************************
 * @return Accumulated units and nano in double.
 */
double ToDouble(int64_t units, int32_t nano);

/**************************
 * @brief Round nano to accuracy from tick.
 *
 * @example Round(129999999, 0.01) = 130000000.
 * @example Round(750000000, 0.01) = 750000000.
 * @example Round(750004320, 0.01) = 750000000.
 * @example Round(751000000, 0.01) = 750000000.
 * @example Round(749000000, 0.01) = 750000000.
 * @example Round(749999999, 0.01) = 750000000.
 * @example Round(880910000, 0.0001) = 880900000.
 *
 * @note Has unit test.
 *
 * @todo Way if nano or tick == 0.
 */
int32_t Round(int32_t nano, double tick);

/**************************
 * @brief Round price to accuracy from tick.
 *
 * @example F(1.001, 0.001) = 1.001.
 * @example F(1.00101, 0.001) = 1.001.
 * @example F(100.0, 0.001) = 100.
 * @example F(100.001911, 0.001) = 100.001.
 *
 * @note Has unit test.
 *
 * @todo If price or tick == 0.
 */
double Round(double price, double tick);

/**************************
 * @brief Replace all "from" to "to" in string.
 *
 * @param str String for replacing.
 * @param from Symbol for replacing from.
 * @param to Symbol for replacing to.
 *
 * @return String with replaced symbols.
 */
template <typename T>
	requires(std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
FORCE_INLINE std::string Replace(T&& str, const char from, const char to)
{
	if constexpr (std::is_same_v<T, std::string>) {
		for (size_t index{ 0 }; index < str.size(); ++index) {
			if (str[index] == from) {
				str[index] = to;
			}
		}
		return str;
	}
	else if constexpr (std::is_same_v<T, std::string_view>) {
		std::string result;
		result.reserve(str.size());
		for (const auto& symbol : str) {
			if (symbol == from) {
				result += to;
			}
			else {
				result += symbol;
			}
		}
		return result;
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Unsupported type for Replace function");
		return {};
	}
}

/**************************
 * @brief Compare floating point values. Default epsilons are: 1.0E-6 for float, 1.0E-10 for double, 1.0E-20 for
 * long double.
 *
 * @tparam T floating point type.
 * @tparam Epsilon floating point epsilon for comparing based on T type.
 *
 * @param first Left argument.
 * @param second Right argument.
 *
 * @return 0 if equal, 1 if first > second, -1 if first < second.
 *
 * @test Has unit tests.
 */
template <std::floating_point T, T epsilon = Epsilon<T>::value>
FORCE_INLINE int CompareFloats(const T first, const T second)
{
	return (std::abs(first - second) >= epsilon) * ((first > second) - (first < second));
}

/**************************
 * @brief Compare floating point values. Default epsilons are: 1.0E-6 for float, 1.0E-10 for double, 1.0E-20 for
 * long double.
 *
 * @tparam T floating point type.
 * @tparam Epsilon floating point epsilon for comparing based on T type.
 *
 * @param first Left argument.
 * @param second Right argument.
 *
 * @return True if first < second, false otherwise.
 *
 * @test Has unit tests.
 */
template <std::floating_point T, T epsilon = Epsilon<T>::value>
FORCE_INLINE bool FloatLess(const T first, const T second)
{
	return second - first > epsilon;
}

/**************************
 * @brief Compare floating point values. Default epsilons are: 1.0E-6 for float, 1.0E-10 for double, 1.0E-20 for
 * long double.
 *
 * @tparam T floating point type.
 * @tparam Epsilon floating point epsilon for comparing based on T type.
 *
 * @param first Left argument.
 * @param second Right argument.
 *
 * @return True if first > second, false otherwise.
 *
 * @test Has unit tests.
 */
template <std::floating_point T, T epsilon = Epsilon<T>::value>
FORCE_INLINE bool FloatGreater(const T first, const T second)
{
	return first - second > epsilon;
}

/**************************
 * @brief Compare floating point values. Default epsilons are: 1.0E-6 for float, 1.0E-10 for double, 1.0E-20 for
 * long double.
 *
 * @tparam T floating point type.
 * @tparam Epsilon floating point epsilon for comparing based on T type.
 *
 * @param first Left argument.
 * @param second Right argument.
 *
 * @return True if first == second, false otherwise.
 *
 * @test Has unit tests.
 */
template <std::floating_point T, T epsilon = Epsilon<T>::value>
FORCE_INLINE bool FloatEqual(const T first, const T second)
{
	return std::abs(first - second) < epsilon;
}

/**************************
 * @brief Round value up to "accuracy" numbers after comma.
 *
 * @example F(0.002029, 2) = 0.01.
 * @example F(0.449999, 2) = 0.45.
 * @example F(0.045999, 2) = 0.05.
 * @example F(0.099999, 2) = 0.1.
 * @example F(0.999999, 0) = 1.
 * @example F(1.099999, 0) = 2.
 *
 * @note Has unit test.
 *
 * @todo Way if accuracy == 0.
 */
double RoundUp(double value, size_t accuracy);

/**************************
 * @brief Round value down to "accuracy" numbers after comma.
 *
 * @example F(0.002029, 2) = 0.
 * @example F(0.449999, 2) = 0.44.
 * @example F(0.045999, 2) = 0.04.
 * @example F(0.099999, 2) = 0.09.
 * @example F(0.999999, 0) = 0.
 * @example F(1.099999, 0) = 1.
 *
 * @note Has unit test.
 *
 * @todo Check is accuracy == 0.
 */
double RoundDown(double value, size_t accuracy);

/**************************
 * @brief Find a position for { x3, y3 } relatively straight { { x1, y1 }, { x2, y2 } }.
 *
 * @note Has unit test.
 *
 * @return 0 if at line, 1 if over, -1 if under, -2 if x1 is equal for x2, or y1 is equal for y2.
 */
int WhereIsPoint(double x1, double y1, double x2, double y2, double x3, double y3);

/**************************
 * @brief Check on nullptr and if first character is null terminator.
 *
 * @return Wstring constructed from string.
 *
 * @test Has unit tests.
 */
FORCE_INLINE std::wstring StringToWstring(const char* cstr)
{
	if (!cstr || *cstr == '\0') {
		return {};
	}

	size_t size_needed{ mbstowcs(nullptr, cstr, 0) };
	if (size_needed == static_cast<size_t>(-1)) {
		LOG_WARNING_NEW("Invalid multibyte sequence in input: {}", cstr);
		return {};
	}

	std::wstring wstr(size_needed, L'\0');
	size_t converted{ mbstowcs(&wstr[0], cstr, size_needed) };
	if (converted == static_cast<size_t>(-1)) {
		LOG_WARNING_NEW("Conversion error during mbstowcs for input: {}", cstr);
		return {};
	}

	return wstr;
}

/**************************
 * @brief Check on nullptr and if first character is null terminator.
 *
 * @return String which is constructed from wstring.
 *
 * @test Has unit tests.
 */
FORCE_INLINE std::string WstringToString(const wchar_t* wcstr)
{
	if (!wcstr || *wcstr == L'\0') {
		return {};
	}

	size_t size_needed{ wcstombs(nullptr, wcstr, 0) };
	if (size_needed == static_cast<size_t>(-1)) {
		LOG_WARNING("Invalid wide char sequence in input");
		return {};
	}

	std::string str(size_needed, '\0');
	size_t converted{ wcstombs(&str[0], wcstr, size_needed) };
	if (converted == static_cast<size_t>(-1)) {
		LOG_WARNING("Conversion error during wcstombs");
		return {};
	}

	return str;
}

/**************************
 * @brief Transform octal-escaped UTF-8 sequence to usual string. Transform chars to wchars and back.
 *
 * @param cstr Octal-escaped UTF-8 sequence.
 *
 * @return Normalized string.
 *
 * @test Has unit tests.
 */
FORCE_INLINE std::string NormalizeOctalEscapedUtf8(const char* cstr)
{
	return WstringToString(StringToWstring(cstr).c_str());
}

/**************************
 * @tparam T Integral type.
 *
 * @param value Value for checking.
 *
 * @return Return exponent of 10 for value, 0 if value is 0, as for |x| <= 10.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires std::is_integral_v<T>
FORCE_INLINE T Exponent10Of(T value)
{
	if (value == 0) {
		return 0;
	}

	T tmp{};
	if constexpr (std::is_signed_v<T>) {
		if (value < 0) {
			value = -value;
		}
	}

	while (value >= 10) {
		value /= 10;
		++tmp;
	}

	return tmp;
}

/**************************
 * @return String IP by sockaddr structure, empty if IP is unknown.
 */
std::string GetStringIp(sockaddr_in addr);

/**************************
 * @return True if all units tests passed, false otherwise.
 */
bool UNITTEST();

}; //* namespace Helper

}; //* namespace MSAPI

#endif //* MSAPI_HELPER_H