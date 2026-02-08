/**************************
 * @file        helper.h
 * @version     6.0
 * @date        2023-09-24
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
#include <cstring>
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

constexpr bool caseSensitive = true;
constexpr bool caseInsensitive = false;

/**************************
 * @brief Check if string contains substring.
 *
 * @tparam CaseSensitive - If true, then case sensitive, otherwise not.
 *
 * @param base Base string for checking.
 * @param sub Substring for checking.
 *
 * @return True if base contains sub, false otherwise.
 *
 * @todo Add unit tests.
 */
template <bool CaseSensitive> FORCE_INLINE bool ContainsStr(const std::string& base, const std::string& sub)
{
	size_t index{ 0 };
	const auto size{ sub.size() };
	for (const auto& symbol : base) {
		if constexpr (CaseSensitive) {
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
 * @attention Does not validate several invalid UTF-8 forms (overlong encodings, surrogate code points U+D800–U+DFFF,
 * and 4-byte lead bytes 0xF5–0xF7 producing code points > U+10FFFF).
 *
 * @return Wstring constructed from UTF-8 string.
 *
 * @test Has unit tests.
 */
FORCE_INLINE std::wstring StringToWstring(const char* cstr)
{
	if (!cstr || *cstr == '\0') {
		return {};
	}

	std::wstring result;
	result.reserve(std::strlen(cstr));

	const auto* ptr{ reinterpret_cast<const unsigned char*>(cstr) };
	while (*ptr != 0) {
		uint32_t codePoint{};
		unsigned char ch{ *ptr };
		uint32_t extraBytes{};

		if (ch < 0x80) {
			codePoint = ch;
			extraBytes = 0;
		}
		else if ((ch & 0xE0) == 0xC0) {
			codePoint = static_cast<uint32_t>(ch & 0x1F);
			extraBytes = 1;
		}
		else if ((ch & 0xF0) == 0xE0) {
			codePoint = static_cast<uint32_t>(ch & 0x0F);
			extraBytes = 2;
		}
		else if ((ch & 0xF8) == 0xF0) {
			codePoint = static_cast<uint32_t>(ch & 0x07);
			extraBytes = 3;
		}
		else {
			LOG_WARNING_NEW("Invalid UTF-8 lead byte in input: {}", cstr);
			return {};
		}

		++ptr;
		for (uint32_t index{}; index < extraBytes; ++index) {
			unsigned char next{ *ptr };
			if ((next & 0xC0) != 0x80) {
				LOG_WARNING_NEW("Invalid UTF-8 continuation byte in input: {}", cstr);
				return {};
			}
			codePoint = codePoint << 6 | static_cast<uint32_t>(next & 0x3F);
			++ptr;
		}

		result.push_back(static_cast<wchar_t>(codePoint));
	}

	return result;
}

/**************************
 * @brief Check on nullptr and if first character is null terminator.
 *
 * @return String which is constructed from wstring encoded as UTF-8.
 *
 * @test Has unit tests.
 */
FORCE_INLINE std::string WstringToString(const wchar_t* wcstr)
{
	if (!wcstr || *wcstr == L'\0') {
		return {};
	}

	std::string result;
	const wchar_t* ptr{ wcstr };

	while (*ptr != L'\0') {
		uint32_t codePoint{ static_cast<uint32_t>(*ptr++) };

		if (codePoint <= 0x7F) {
			result.push_back(static_cast<char>(codePoint));
		}
		else if (codePoint <= 0x7FF) {
			result.push_back(static_cast<char>((codePoint >> 6) | 0xC0));
			result.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
		}
		else if (codePoint <= 0xFFFF) {
			result.push_back(static_cast<char>((codePoint >> 12) | 0xE0));
			result.push_back(static_cast<char>(((codePoint >> 6) & 0x3F) | 0x80));
			result.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
		}
		else if (codePoint <= 0x10FFFF) {
			result.push_back(static_cast<char>((codePoint >> 18) | 0xF0));
			result.push_back(static_cast<char>(((codePoint >> 12) & 0x3F) | 0x80));
			result.push_back(static_cast<char>(((codePoint >> 6) & 0x3F) | 0x80));
			result.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
		}
		else {
			LOG_WARNING("Invalid Unicode code point in WstringToString");
			return {};
		}
	}

	return result;
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

/**
 * @brief Encode data to Base64 format.
 *
 * @tparam T Type of the data elements.
 *
 * @param data Span of data to encode.
 * @param buffer Span for storing the encoded string. Must have a size of at least (data.size() + 2) / 3 * 4.
 *
 * @return String view on the encoded data inside buffer.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires(sizeof(T) == 1 && std::is_integral_v<T>)
FORCE_INLINE [[nodiscard]] std::string_view Base64Encode(const std::span<T> data, const std::span<char> buffer)
{
	static const char B64_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	if (const auto required{ (data.size() + 2) / 3 * 4 }; buffer.size() < required) [[unlikely]] {
		LOG_WARNING_NEW(
			"Buffer size is insufficient for Base64 encoding. Required: {}, Provided: {}", required, buffer.size());
		return {};
	}

	size_t bufferIndex{};
	for (size_t index{}; index < data.size(); index += 3) {
		uint32_t triple{};
		const auto rem{ data.size() - index };

		triple |= static_cast<uint32_t>(data[index]) << 16;
		if (rem > 1) {
			triple |= static_cast<uint32_t>(data[index + 1]) << 8;
		}
		if (rem > 2) {
			triple |= static_cast<uint32_t>(data[index + 2]);
		}

		buffer[bufferIndex++] = B64_ALPHABET[(triple >> 18) & 0x3F];
		buffer[bufferIndex++] = B64_ALPHABET[(triple >> 12) & 0x3F];

		if (rem > 1) {
			buffer[bufferIndex++] = B64_ALPHABET[(triple >> 6) & 0x3F];
		}
		else {
			buffer[bufferIndex++] = '=';
		}

		if (rem > 2) {
			buffer[bufferIndex++] = B64_ALPHABET[triple & 0x3F];
		}
		else {
			buffer[bufferIndex++] = '=';
		}
	}

	return std::string_view{ buffer.data(), bufferIndex };
}

/**
 * @brief Decode Base64 fully properly encoded string.
 *
 * @attention String to decode must be multiply of 4 in size and buffer must have a size of at least data.size() / 4 * 3
 * - padding, where padding is the number of '=' characters at the end of the string. Padding positions must be correct.
 *
 * @tparam T Type of the data elements.
 *
 * @param data Base64 encoded string.
 * @param buffer Span for storing the decoded data.
 *
 * @return Span of decoded data inside buffer.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires(sizeof(T) == 1 && std::is_integral_v<T>)
FORCE_INLINE [[nodiscard]] std::span<const T> Base64Decode(const std::string_view data, const std::span<char> buffer)
{
	static const int8_t B64_LUT[256] = { /* initialize all to -1 */
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59,
		60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
		43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
		/* rest are -1 */
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};

	const auto size{ data.size() };
	if (size % 4 != 0 || size == 0) [[unlikely]] {
		LOG_WARNING_NEW("Invalid Base64 size {}: {}", size, data);
		return {};
	}

	const auto firstPadding{ (data[size - 2] == '=' ? 1 : 0) };
	const auto secondPadding{ (data[size - 1] == '=' ? 1 : 0) };
	if (firstPadding && !secondPadding) [[unlikely]] {
		LOG_WARNING_NEW("Invalid Base64 padding pattern in input: {}", data);
		return {};
	}

	if (const auto required{ size / 4 * 3 - static_cast<size_t>(firstPadding) - static_cast<size_t>(secondPadding) };
		buffer.size() < required) [[unlikely]] {
		LOG_WARNING_NEW(
			"Buffer size is insufficient for Base64 decoding. Required: {}, Provided: {}", required, buffer.size());
		return {};
	}

	size_t bufferIndex{};
	const size_t lastIndex{ size - 4 };

	for (size_t index{}; index < lastIndex; index += 4) {
		const auto v1{ B64_LUT[static_cast<uint8_t>(data[index])] };
		const auto v2{ B64_LUT[static_cast<uint8_t>(data[index + 1])] };
		const auto v3{ B64_LUT[static_cast<uint8_t>(data[index + 2])] };
		const auto v4{ B64_LUT[static_cast<uint8_t>(data[index + 3])] };
		if (v1 < 0 || v2 < 0 || v3 < 0 || v4 < 0) [[unlikely]] {
			LOG_WARNING_NEW("Invalid Base64 character in input: {}", data);
			return {};
		}

		const uint32_t triple{ (static_cast<uint32_t>(v1) << 18) | (static_cast<uint32_t>(v2) << 12)
			| (static_cast<uint32_t>(v3) << 6) | static_cast<uint32_t>(v4) };

		buffer[bufferIndex++] = static_cast<char>((triple >> 16) & 0xFF);
		buffer[bufferIndex++] = static_cast<char>((triple >> 8) & 0xFF);
		buffer[bufferIndex++] = static_cast<char>(triple & 0xFF);
	}

	const auto v1{ B64_LUT[static_cast<uint8_t>(data[lastIndex])] };
	const auto v2{ B64_LUT[static_cast<uint8_t>(data[lastIndex + 1])] };
	const auto v3{ B64_LUT[static_cast<uint8_t>(data[lastIndex + 2])] };
	const auto v4{ B64_LUT[static_cast<uint8_t>(data[lastIndex + 3])] };
	if (v1 < 0 || v2 < 0) [[unlikely]] {
		LOG_WARNING_NEW("Invalid Base64 character in input: {}", data);
		return {};
	}

	const uint32_t triple{ (static_cast<uint32_t>(v1) << 18) | (static_cast<uint32_t>(v2) << 12)
		| ((firstPadding ? 0 : static_cast<uint32_t>(v3)) << 6) | (secondPadding ? 0 : static_cast<uint32_t>(v4)) };

	buffer[bufferIndex++] = static_cast<char>((triple >> 16) & 0xFF);
	if (!firstPadding) {
		buffer[bufferIndex++] = static_cast<char>((triple >> 8) & 0xFF);
		if (!secondPadding) {
			buffer[bufferIndex++] = static_cast<char>(triple & 0xFF);
		}
	}

	return { reinterpret_cast<const T*>(buffer.data()), bufferIndex };
}

}; //* namespace Helper

}; //* namespace MSAPI

#endif //* MSAPI_HELPER_H