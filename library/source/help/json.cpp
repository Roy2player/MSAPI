/**************************
 * @file        json.cpp
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
 */

#include "json.h"
#include "helper.h"
#include <charconv>
#include <cmath>
#include <iomanip>
#include <set>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Json
---------------------------------------------------------------------------------*/

Json::Json(const std::string_view body) { Construct(body); }

void Json::Construct(const std::string_view body)
{
	// std::cout << "body: " << body << std::endl;

	const size_t bodySize{ body.size() };
	if (bodySize < 2 || (body[0] == '{' && body[bodySize - 1] != '}')
		|| (body[0] == '[' && body[bodySize - 1] != ']')) {

		LOG_DEBUG("Body size less than 2 or root is not object or array");
		return;
	}

	bool isKey{ true };
	std::string key;
	std::string value;
	bool insideDoubleQuotes{ false };
	size_t objectDepth{ 1 };
	size_t begin{ 0 };
	size_t end{ 0 };

	size_t index{ 1 };
	if (body[0] == '[') {
		index = 0;
		isKey = false;
		key = "rootArray";
		goto startArray;
	}

	for (; index < bodySize; ++index) {
		if (isKey) {
			if (insideDoubleQuotes) {
				if (body[index] == '"') {
					isKey = false;
					insideDoubleQuotes = false;
					continue;
				}
				if (body[index] == '\\' && index + 1 < bodySize
					&& (body[index + 1] == '"' || body[index + 1] == '\\')) {

					key += '\\';
					key += body[++index];
					continue;
				}
				key += body[index];
				continue;
			}

			if (body[index] == '"') {
				insideDoubleQuotes = true;
				continue;
			}

			if (body[index] != ' ' && body[index] != '\n' && body[index] != '\t' && body[index] != ','
				&& body[index] != '{' && body[index] != '}' && body[index] != '[' && body[index] != ']') {

				LOG_DEBUG("Unexpected symbol between key and value: " + body[index]);
				return;
			}

			continue;
		}

		//* It is string value
		if (insideDoubleQuotes) {
			if (body[index] == '"') {
				isKey = true;
				insideDoubleQuotes = false;
				m_keysAndValues.emplace(std::move(key), JsonNode{ std::move(value) });
				key.clear();
				value.clear();
				continue;
			}

			if (body[index] == '\\' && index + 1 < bodySize && (body[index + 1] == '"' || body[index + 1] == '\\')) {
				value += '\\';
				value += body[++index];
				continue;
			}

			value += body[index];
			continue;
		}

		//* It will be string value
		if (body[index] == '"') {
			insideDoubleQuotes = true;
			continue;
		}

		//* It is undefined value type
		if (body[index] == ' ' || body[index] == '\n' || body[index] == '\t') {
			continue;
		}

#define TMP_MSAPI_JSON_EMPLACE_double                                                                                  \
	if (const auto mantissa{ static_cast<short>(exponentStart - dot - 2) }; exponent >= 0 && mantissa <= exponent) {   \
		if (!Helper::FloatLess(number, 0.)) {                                                                          \
			m_keysAndValues.emplace(std::move(key),                                                                    \
				JsonNode{ UINT64(std::round(number * std::pow(10, static_cast<double>(mantissa))))                     \
					* UINT64(std::pow(10, exponent - mantissa)) });                                                    \
		}                                                                                                              \
		else {                                                                                                         \
			m_keysAndValues.emplace(std::move(key),                                                                    \
				JsonNode{ INT64(std::round(number * std::pow(10, static_cast<double>(mantissa))))                      \
					* INT64(std::pow(10, exponent - mantissa)) });                                                     \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		/* Scientific string can be parsed as double by std::from_chars here and in another places, where we sure the  \
		 * result is double, but current implementation is faster ~ 10% for 01 and 03 */                               \
		m_keysAndValues.emplace(std::move(key), JsonNode{ number * std::pow(10, static_cast<double>(exponent)) });     \
	}

#define TMP_MSAPI_JSON_EMPLACE_uint64_t                                                                                \
	if (exponent >= 0 || Helper::Exponent10Of(number) >= UINT64(std::abs(exponent))) {                                 \
		m_keysAndValues.emplace(                                                                                       \
			std::move(key), JsonNode{ number * UINT64(std::pow(10, static_cast<double>(exponent))) });                 \
	}                                                                                                                  \
	else {                                                                                                             \
		m_keysAndValues.emplace(                                                                                       \
			std::move(key), JsonNode{ static_cast<double>(number) * std::pow(10, static_cast<double>(exponent)) });    \
	}

#define TMP_MSAPI_JSON_EMPLACE_int64_t                                                                                 \
	if (exponent >= 0 || Helper::Exponent10Of(number) >= INT64(std::abs(exponent))) {                                  \
		m_keysAndValues.emplace(                                                                                       \
			std::move(key), JsonNode{ number * INT64(std::pow(10, static_cast<double>(exponent))) });                  \
	}                                                                                                                  \
	else {                                                                                                             \
		m_keysAndValues.emplace(                                                                                       \
			std::move(key), JsonNode{ static_cast<double>(number) * std::pow(10, static_cast<double>(exponent)) });    \
	}

#define TMP_MSAPI_JSON_EMPLACE_NUMBER(type)                                                                            \
	if (exponentStart != 0) {                                                                                          \
		short exponent{};                                                                                              \
		error = std::from_chars(body.data() + exponentStart, body.data() + exponentEnd - exponentStart, exponent).ec;  \
		if (error != std::errc{}) {                                                                                    \
			LOG_ERROR("Cannot convert exponent string to short. Start index: " + _S(exponentStart)                     \
				+ ", end: " + _S(exponentEnd) + ". Error: " + std::make_error_code(error).message());                  \
			return;                                                                                                    \
		}                                                                                                              \
                                                                                                                       \
		TMP_MSAPI_JSON_EMPLACE_##type;                                                                                 \
	}                                                                                                                  \
	else {                                                                                                             \
		m_keysAndValues.emplace(std::move(key), JsonNode{ number });                                                   \
	}

#define TMP_MSAPI_JSON_PARSE_NUMBER(type)                                                                              \
	{                                                                                                                  \
		size_t dot{}, exponentStart{}, exponentEnd{};                                                                  \
		value += body[index];                                                                                          \
		while (++index < bodySize) {                                                                                   \
			if (std::isdigit(body[index])) {                                                                           \
				value += body[index];                                                                                  \
			}                                                                                                          \
			else if (body[index] == '.') {                                                                             \
				if (dot != 0) {                                                                                        \
					LOG_DEBUG("Unexpectedly two dots in number value");                                                \
					return;                                                                                            \
				}                                                                                                      \
				value += '.';                                                                                          \
				dot = index;                                                                                           \
			}                                                                                                          \
			else if (body[index] == 'e') {                                                                             \
				exponentStart = index + 1;                                                                             \
				while (++index < bodySize) {                                                                           \
					if (!std::isdigit(body[index]) && body[index] != '+' && body[index] != '-') {                      \
						exponentEnd = index;                                                                           \
						goto finishNumber##type;                                                                       \
					}                                                                                                  \
				}                                                                                                      \
			}                                                                                                          \
			else {                                                                                                     \
				finishNumber##type :;                                                                                  \
				isKey = true;                                                                                          \
				if (dot != 0) {                                                                                        \
					double number{};                                                                                   \
					auto error{ std::from_chars(value.data(), value.data() + value.size(), number).ec };               \
					if (error != std::errc{}) {                                                                        \
						LOG_ERROR("Cannot convert string to double: " + value                                          \
							+ ". Error: " + std::make_error_code(error).message());                                    \
						return;                                                                                        \
					}                                                                                                  \
					TMP_MSAPI_JSON_EMPLACE_NUMBER(double);                                                             \
				}                                                                                                      \
				else {                                                                                                 \
					type number{};                                                                                     \
					auto error{ std::from_chars(value.data(), value.data() + value.size(), number).ec };               \
					if (error != std::errc{}) {                                                                        \
						LOG_ERROR("Cannot convert string to " #type ": " + value                                       \
							+ ". Error: " + std::make_error_code(error).message());                                    \
						return;                                                                                        \
					}                                                                                                  \
					TMP_MSAPI_JSON_EMPLACE_NUMBER(type);                                                               \
				}                                                                                                      \
				key.clear();                                                                                           \
				value.clear();                                                                                         \
				break;                                                                                                 \
			}                                                                                                          \
		}                                                                                                              \
                                                                                                                       \
		continue;                                                                                                      \
	}

		//* It is potential number value
		if (body[index] == '-') {
			value += body[index++];
			if (index >= bodySize || !std::isdigit(body[index])) {
				LOG_DEBUG("Unexpected symbol after '-', expected is a number: " + body[index]);
				return;
			}
			goto startNegativeNumber;
		}

		//* It is number value
		if (std::isdigit(body[index])) {
			TMP_MSAPI_JSON_PARSE_NUMBER(uint64_t);
		//* Zero performance cost differentiation
		startNegativeNumber:
			TMP_MSAPI_JSON_PARSE_NUMBER(int64_t);
		}

#undef TMP_MSAPI_JSON_PARSE_NUMBER
#undef TMP_MSAPI_JSON_EMPLACE_NUMBER
#undef TMP_MSAPI_JSON_EMPLACE_int64_t
#undef TMP_MSAPI_JSON_EMPLACE_uint64_t
#undef TMP_MSAPI_JSON_EMPLACE_double

		//* It is boolean value 'true'
		if (body[index] == 't') {
			if (index + 3 < bodySize && body[index + 1] == 'r' && body[index + 2] == 'u' && body[index + 3] == 'e') {
				isKey = true;
				m_keysAndValues.emplace(std::move(key), JsonNode{ true });
				key.clear();
				index += 3;
				continue;
			}

			LOG_DEBUG("Unexpected symbol after 't', expected 'rue'");
			return;
		}

		//* It is boolean value 'false'
		if (body[index] == 'f') {
			if (index + 4 < bodySize && body[index + 1] == 'a' && body[index + 2] == 'l' && body[index + 3] == 's'
				&& body[index + 4] == 'e') {

				isKey = true;
				m_keysAndValues.emplace(std::move(key), JsonNode{ false });
				key.clear();
				index += 4;
				continue;
			}

			LOG_DEBUG("Unexpected symbol after 'f', expected 'alse'");
			return;
		}

		//* It is null value
		if (body[index] == 'n') {
			if (index + 3 < bodySize && body[index + 1] == 'u' && body[index + 2] == 'l' && body[index + 3] == 'l') {
				isKey = true;
				m_keysAndValues.emplace(std::move(key), JsonNode{ nullptr });
				key.clear();
				index += 3;
				continue;
			}

			LOG_DEBUG("Unexpected symbol after 'n', expected 'ull'");
			return;
		}

		//* It is json object
		if (body[index] == '{') {
			value += body[index];
			while (objectDepth != 0 && ++index < bodySize) {
				if (!insideDoubleQuotes) {
					if (body[index] == '{') {
						++objectDepth;
					}
					if (body[index] == '}') {
						--objectDepth;
					}
				}
				else if (body[index] == '"') {
					insideDoubleQuotes = false;
				}
				else if (body[index] == '\\' && index + 1 < bodySize
					&& (body[index + 1] == '"' || body[index + 1] == '\\')) {

					++index;
					value += '\\';
					value += body[++index];
					continue;
				}
				value += body[index];
			}

			objectDepth = 1;
			isKey = true;
			m_keysAndValues.emplace(std::move(key), JsonNode{ Json{ value } });
			key.clear();
			value.clear();
			continue;
		}

		//* It is array
		if (body[index] == '[') {
		startArray:
			begin = index;
			end = begin;
			while (objectDepth != 0 && ++end < bodySize) {
				if (!insideDoubleQuotes) {
					if (body[end] == '[') {
						++objectDepth;
					}
					if (body[end] == ']') {
						--objectDepth;
					}
				}
				else if (body[end] == '"') {
					insideDoubleQuotes = false;
				}
				else if (body[end] == '\\' && end + 1 < bodySize && (body[end + 1] == '"' || body[end + 1] == '\\')) {
					++end;
				}
			}

			index = end;

			objectDepth = 1;
			isKey = true;
			m_keysAndValues.emplace(std::move(key), JsonNode{ body, begin, end });
			key.clear();
			continue;
		}

		//* Expected symbols between value and key
		if (body[index] == ' ' || body[index] == '\n' || body[index] == '\t' || body[index] == ','
			|| body[index] == ':') {

			continue;
		}

		//* End of json object
		if ((body[index] == '}' || body[index] == ']') && index + 1 == bodySize) {
			break;
		}

		LOG_DEBUG("Unexpected symbol between value and key: " + body[index]);
		return;
	}

	if (isKey && key.empty() && value.empty() && !insideDoubleQuotes && objectDepth == 1
		&& std::all_of(
			m_keysAndValues.begin(), m_keysAndValues.end(), [](const auto& pair) { return pair.second.Valid(); })) {

		m_isValid = true;
	}
}

bool Json::Valid() const noexcept { return m_isValid; }

const std::map<std::string, JsonNode, std::less<>>& Json::GetKeysAndValues() const noexcept { return m_keysAndValues; }

const JsonNode* Json::GetValue(const std::string_view key) const noexcept
{
	if (const auto& it{ m_keysAndValues.find(key) }; it != m_keysAndValues.end()) {
		return &it->second;
	}
	return nullptr;
}

void Json::Clear()
{
	m_isValid = false;
	m_keysAndValues.clear();
}

std::string Json::ToString() const noexcept
{
	if (m_keysAndValues.empty()) {
		return "Json:\n{} <valid: " + _S(m_isValid) + ">";
	}

	size_t maxKeySize{ 0 };
	for (const auto& [key, value] : m_keysAndValues) {
		if (maxKeySize < key.size()) {
			maxKeySize = key.size();
		}
	}

	std::stringstream stream;

#define format std::left << std::setw(static_cast<int>(maxKeySize))

	stream << std::fixed << std::setprecision(16) << "Json:\n{";

	for (const auto& [key, value] : m_keysAndValues) {
		stream << "\n\t" << format << key << " : ";
		const auto& node{ value.GetValue() };
		if (std::holds_alternative<Json>(node) || std::holds_alternative<std::list<JsonNode>>(node)) {
			std::string node{ value.ToString() };
			size_t pos{ 0 };
			while ((pos = node.find('\n', pos)) != std::string::npos) {
				node.replace(pos, 1, "\n\t");
				pos += 2;
			}
			stream << node;
		}
		else {
			stream << value.ToString();
		}
	}

#undef format

	stream << "\n} <valid: " + _S(m_isValid) + ">";

	return stream.str();
}

std::string Json::ToJson() const noexcept
{
	if (m_keysAndValues.empty()) {
		return "{}";
	}
	std::stringstream stream;
	stream << std::fixed << std::setprecision(16) << "{";

	auto begin{ m_keysAndValues.begin() };
	const auto end{ m_keysAndValues.end() };
	stream << "\"" << begin->first << "\":" << begin++->second.PrepareToJson();
	for (; begin != end; ++begin) {
		stream << ",\"" << begin->first << "\":" << begin->second.PrepareToJson();
	}
	return stream.str() + "}";
}

std::string operator+(const std::string& str, Json& json) { return str + json.ToString(); }

std::ostream& operator<<(std::ostream& os, Json& json) { return os << json.ToString(); }

Json::operator std::string() { return ToString(); }

/*---------------------------------------------------------------------------------
JsonNode
---------------------------------------------------------------------------------*/

JsonNode::JsonNode(const std::string_view body, size_t begin, const size_t end)
{
	if (end - begin < 1 || body[begin] != '[' || body[end] != ']') {
		LOG_DEBUG("Body size less than 1 or begin and end of body not equal '[' and ']'");
		return;
	}

	std::list<JsonNode> array{};

	std::string value;
	bool insideDoubleQuotes{ false };
	size_t objectDepth{ 1 };
	size_t _begin{ 0 };
	size_t _end{ 0 };

	++begin;
	for (; begin < end; ++begin) {
		//* It is string value
		if (insideDoubleQuotes) {
			if (body[begin] == '"') {
				insideDoubleQuotes = false;
				array.emplace_back(JsonNode{ std::move(value) });
				value.clear();
				continue;
			}

			if (body[begin] == '\\' && begin + 1 < end && (body[begin + 1] == '"' || body[begin + 1] == '\\')) {
				value += '\\';
				value += body[++begin];
				continue;
			}

			value += body[begin];
			continue;
		}

		//* It will be string value
		if (body[begin] == '"') {
			insideDoubleQuotes = true;
			continue;
		}

		//* It is undefined value type
		if (body[begin] == ' ' || body[begin] == '\n' || body[begin] == '\t') {
			continue;
		}

#define TMP_MSAPI_JSON_EMPLACE_double                                                                                  \
	if (const auto mantissa{ static_cast<short>(exponentStart - dot - 2) }; exponent >= 0 && mantissa <= exponent) {   \
		if (!Helper::FloatLess(number, 0.)) {                                                                          \
			array.emplace_back(JsonNode{ UINT64(std::round(number * std::pow(10, static_cast<double>(mantissa))))      \
				* UINT64(std::pow(10, exponent - mantissa)) });                                                        \
		}                                                                                                              \
		else {                                                                                                         \
			array.emplace_back(JsonNode{ INT64(std::round(number * std::pow(10, static_cast<double>(mantissa))))       \
				* INT64(std::pow(10, exponent - mantissa)) });                                                         \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		array.emplace_back(JsonNode{ number * std::pow(10, static_cast<double>(exponent)) });                          \
	}

#define TMP_MSAPI_JSON_EMPLACE_uint64_t                                                                                \
	if (exponent >= 0 || Helper::Exponent10Of(number) >= UINT64(std::abs(exponent))) {                                 \
		array.emplace_back(JsonNode{ number * UINT64(std::pow(10, static_cast<double>(exponent))) });                  \
	}                                                                                                                  \
	else {                                                                                                             \
		array.emplace_back(JsonNode{ static_cast<double>(number) * std::pow(10, static_cast<double>(exponent)) });     \
	}

#define TMP_MSAPI_JSON_EMPLACE_int64_t                                                                                 \
	if (exponent >= 0 || Helper::Exponent10Of(number) >= INT64(std::abs(exponent))) {                                  \
		array.emplace_back(JsonNode{ number * INT64(std::pow(10, static_cast<double>(exponent))) });                   \
	}                                                                                                                  \
	else {                                                                                                             \
		array.emplace_back(JsonNode{ static_cast<double>(number) * std::pow(10, static_cast<double>(exponent)) });     \
	}

#define TMP_MSAPI_JSON_EMPLACE_NUMBER(type)                                                                            \
	if (exponentStart != 0) {                                                                                          \
		short exponent{};                                                                                              \
		error = std::from_chars(body.data() + exponentStart, body.data() + exponentEnd - exponentStart, exponent).ec;  \
		if (error != std::errc{}) {                                                                                    \
			LOG_ERROR("Cannot convert exponent string to short. Start index: " + _S(exponentStart)                     \
				+ ", end: " + _S(exponentEnd) + ". Error: " + std::make_error_code(error).message());                  \
			return;                                                                                                    \
		}                                                                                                              \
                                                                                                                       \
		TMP_MSAPI_JSON_EMPLACE_##type;                                                                                 \
	}                                                                                                                  \
	else {                                                                                                             \
		array.emplace_back(JsonNode{ number });                                                                        \
	}

#define TMP_MSAPI_JSON_PARSE_NUMBER(type)                                                                              \
	{                                                                                                                  \
		size_t dot{}, exponentStart{}, exponentEnd{};                                                                  \
		value += body[begin];                                                                                          \
		while (++begin <= /* end can be a trigger to emplace */ end) {                                                 \
			if (std::isdigit(body[begin])) {                                                                           \
				value += body[begin];                                                                                  \
			}                                                                                                          \
			else if (body[begin] == '.') {                                                                             \
				if (dot != 0) {                                                                                        \
					LOG_DEBUG("Unexpectedly two dots in number value");                                                \
					return;                                                                                            \
				}                                                                                                      \
				value += '.';                                                                                          \
				dot = begin;                                                                                           \
			}                                                                                                          \
			else if (body[begin] == 'e') {                                                                             \
				exponentStart = begin + 1;                                                                             \
				while (++begin <= end) {                                                                               \
					if (!std::isdigit(body[begin]) && body[begin] != '+' && body[begin] != '-') {                      \
						exponentEnd = begin;                                                                           \
						goto finishNumber##type;                                                                       \
					}                                                                                                  \
				}                                                                                                      \
			}                                                                                                          \
			else {                                                                                                     \
				finishNumber##type :;                                                                                  \
				if (dot != 0) {                                                                                        \
					double number{};                                                                                   \
					auto error{ std::from_chars(value.data(), value.data() + value.size(), number).ec };               \
					if (error != std::errc{}) {                                                                        \
						LOG_ERROR("Cannot convert string to double: " + value                                          \
							+ ". Error: " + std::make_error_code(error).message());                                    \
						return;                                                                                        \
					}                                                                                                  \
					TMP_MSAPI_JSON_EMPLACE_NUMBER(double);                                                             \
				}                                                                                                      \
				else {                                                                                                 \
					type number{};                                                                                     \
					auto error{ std::from_chars(value.data(), value.data() + value.size(), number).ec };               \
					if (error != std::errc{}) {                                                                        \
						LOG_ERROR("Cannot convert string to " #type ": " + value                                       \
							+ ". Error: " + std::make_error_code(error).message());                                    \
						return;                                                                                        \
					}                                                                                                  \
					TMP_MSAPI_JSON_EMPLACE_NUMBER(type);                                                               \
				}                                                                                                      \
				value.clear();                                                                                         \
				break;                                                                                                 \
			}                                                                                                          \
		}                                                                                                              \
                                                                                                                       \
		continue;                                                                                                      \
	}

		//* It is potential number value
		if (body[begin] == '-') {
			value += body[begin++];
			if (begin >= end || !std::isdigit(body[begin])) {
				LOG_DEBUG("Unexpected symbol after '-', expected is number");
				return;
			}
			goto startNegativeNumber;
		}

		//* It is number value
		if (std::isdigit(body[begin])) {
			TMP_MSAPI_JSON_PARSE_NUMBER(uint64_t);
		//* Zero performance cost differentiation
		startNegativeNumber:
			TMP_MSAPI_JSON_PARSE_NUMBER(int64_t);
		}

#undef TMP_MSAPI_JSON_PARSE_NUMBER
#undef TMP_MSAPI_JSON_EMPLACE_NUMBER
#undef TMP_MSAPI_JSON_EMPLACE_int64_t
#undef TMP_MSAPI_JSON_EMPLACE_uint64_t
#undef TMP_MSAPI_JSON_EMPLACE_double

		//* It is boolean value 'true'
		if (body[begin] == 't') {
			if (begin + 3 < end && body[begin + 1] == 'r' && body[begin + 2] == 'u' && body[begin + 3] == 'e') {
				array.emplace_back(JsonNode{ true });
				begin += 3;
				continue;
			}

			LOG_DEBUG("Unexpected symbol after 't', expected 'rue'");
			return;
		}

		//* It is boolean value 'false'
		if (body[begin] == 'f') {
			if (begin + 4 < end && body[begin + 1] == 'a' && body[begin + 2] == 'l' && body[begin + 3] == 's'
				&& body[begin + 4] == 'e') {

				array.emplace_back(JsonNode{ false });
				begin += 4;
				continue;
			}

			LOG_DEBUG("Unexpected symbol after 'f', expected 'alse'");
			return;
		}

		//* It is null value
		if (body[begin] == 'n') {
			if (begin + 3 < end && body[begin + 1] == 'u' && body[begin + 2] == 'l' && body[begin + 3] == 'l') {
				array.emplace_back(JsonNode{ nullptr });
				begin += 3;
				continue;
			}

			LOG_DEBUG("Unexpected symbol after 'n', expected 'ull'");
			return;
		}

		//* It is json object
		if (body[begin] == '{') {
			value += body[begin];
			while (objectDepth != 0 && ++begin < end) {
				if (!insideDoubleQuotes) {
					if (body[begin] == '{') {
						++objectDepth;
					}
					if (body[begin] == '}') {
						--objectDepth;
					}
				}
				else if (body[begin] == '"') {
					insideDoubleQuotes = false;
				}
				else if (body[begin] == '\\' && begin + 1 < end
					&& (body[begin + 1] == '"' || body[begin + 1] == '\\')) {

					value += '\\';
					value += body[++begin];
					continue;
				}
				value += body[begin];
			}

			objectDepth = 1;
			array.emplace_back(JsonNode{ Json{ value } });
			value.clear();
			continue;
		}

		//* It is array
		if (body[begin] == '[') {
			_begin = begin;
			_end = _begin;
			while (objectDepth != 0 && ++_end < end) {
				if (!insideDoubleQuotes) {
					if (body[_end] == '[') {
						++objectDepth;
					}
					if (body[_end] == ']') {
						--objectDepth;
					}
				}
				else if (body[_end] == '"') {
					insideDoubleQuotes = false;
				}
				else if (body[_end] == '\\' && _end + 1 < end && (body[_end + 1] == '"' || body[_end + 1] == '\\')) {
					++_end;
					continue;
				}
			}

			begin = _end;

			objectDepth = 1;
			array.emplace_back(JsonNode{ body, _begin, _end });
			continue;
		}

		//* Expected symbol between values
		if (body[begin] == ' ' || body[begin] == '\n' || body[begin] == '\t' || body[begin] == ',') {
			continue;
		}

		//* End of array
		if (body[begin] == ']' && begin == end) {
			break;
		}

		LOG_DEBUG("Unexpected symbol: " + body[begin]);
		return;
	}

	if (value.empty() && !insideDoubleQuotes && objectDepth == 1 && begin >= /* > in case with digit */ end
		&& std::all_of(array.begin(), array.end(), [](const auto& node) { return node.Valid(); })) {

		m_valid = true;
	}

	m_value = std::move(array);
}

bool JsonNode::Valid() const noexcept { return m_valid; }

const std::variant<JsonNodeTypes>& JsonNode::GetValue() const noexcept { return m_value; }

std::string JsonNode::ToString() const noexcept
{
	return std::forward<std::string>(std::visit(
		[this](auto&& arg) -> std::string {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, Json>) {
				return arg.ToString();
			}
			else if constexpr (std::is_same_v<T, std::list<JsonNode>>) {
				if (arg.empty()) {
					return "[] <valid: " + _S(m_valid) + ">";
				}

				std::stringstream stream;
				stream << "[\n";
				auto current{ arg.begin() };
				const auto end{ arg.end() };
				stream << "\t" << current++->ToString();

				for (; current != end; ++current) {
					stream << ",\n\t" << current->ToString();
				}
				stream << "\n] <valid: " + _S(m_valid) + ">";
				return stream.str();
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				return arg;
			}
			else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>
				|| std::is_same_v<T, bool>) {
				return _S(arg);
			}
			else if constexpr (std::is_same_v<T, std::nullptr_t>) {
				return "null";
			}
			else {
				static_assert(sizeof(T) + 1 == 0, "Unknown node type");
				return "";
			}
		},
		m_value));
}

std::string JsonNode::PrepareToJson() const noexcept
{
	return std::forward<std::string>(std::visit(
		[this](auto&& arg) -> std::string {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, Json>) {
				return arg.ToJson();
			}
			else if constexpr (std::is_same_v<T, std::list<JsonNode>>) {
				if (arg.empty()) {
					return "[]";
				}

				std::stringstream stream;
				stream << "[";
				auto current{ arg.begin() };
				const auto end{ arg.end() };
				stream << current++->PrepareToJson();

				for (; current != end; ++current) {
					stream << "," << current->PrepareToJson();
				}
				stream << "]";
				return stream.str();
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				return "\"" + arg + "\"";
			}
			else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>
				|| std::is_same_v<T, bool>) {
				return _S(arg);
			}
			else if constexpr (std::is_same_v<T, std::nullptr_t>) {
				return "null";
			}
			else {
				static_assert(sizeof(T) + 1 == 0, "Unknown node type");
				return "";
			}
		},
		m_value));
}

}; //* namespace MSAPI