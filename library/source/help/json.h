/**************************
 * @file        json.h
 * @version     6.0
 * @date        2023-12-09
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

#ifndef MSAPI_JSON_H
#define MSAPI_JSON_H

#include "meta.hpp"
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <variant>

namespace MSAPI {

class JsonNode;

/**************************
 * @brief Object for parsing Json data. Initialization in constructor from string, generate array pairs { key, value }.
 * If Json root is not an object but an array, key for it will be "rootArray".
 * @brief Support RFC 8259 specification.
 *
 * @note If parsed number without point it is uint64_t or int64_t. If number without point and sign it is uint64_t.
 */
class Json {
private:
	bool m_isValid{ false };
	std::map<std::string, JsonNode, std::less<>> m_keysAndValues;

public:
	/**************************
	 * @brief Construct a new Json object, parse buffer in constructor.
	 *
	 * @param body String to parse.
	 *
	 * @test Has unit test.
	 */
	Json(std::string_view body);

	/**************************
	 * @brief Construct a new empty Json object.
	 *
	 * @test Has unit test.
	 */
	Json() = default;

	/**************************
	 * @brief Construct a new Json object, empty constructor.
	 *
	 * @param body String to parse.
	 *
	 * @test Has unit test.
	 */
	void Construct(std::string_view body);

	/**************************
	 * @return True if Json is valid.
	 *
	 * @test Has unit test.
	 */
	bool Valid() const noexcept;

	/**************************
	 * @return Readable reference on keys and values of Json.
	 *
	 * @test Has unit test.
	 */
	const std::map<std::string, JsonNode, std::less<>>& GetKeysAndValues() const noexcept;

	/**************************
	 * @return Readable pointer on value of key, nullptr if does not exist.
	 *
	 * @test Has unit test.
	 */
	const JsonNode* GetValue(std::string_view key) const noexcept;

	/**************************
	 * @brief Clear all data and set invalid flag.
	 *
	 * @test Has unit test.
	 */
	void Clear();

	/**************************
	 * @example Json:
	 * {
	 *		information : Json:
	 *		{
	 *			balance1 : 123.0000000000000000
	 *			balance2 : 321
	 *			email    : 22@2.ru
	 *		} <valid: true>
	 *		logs        : [
	 *			1Tue Jun 21 13:01:20.106297 2022: Get account information is true,
	 *			2Tue Jun 21 13:01:20.106297 2022: Get account information is true,
	 *			3Tue Jun 21 13:01:20.106297 2022: Get account information is true
	 *		] <valid: true>
	 *		type        : true
	 *		type2       : true2
	 *  	type3	    : null
	 * } <valid: true>
	 *
	 * @test Has unit test.
	 */
	std::string ToString() const noexcept;

	/**************************
	 * @example {"information":{"balance1":123.0000000000000000,"balance2":"321","email":"22@2.ru"},"logs":["1Tue Jun 21
	 * 13:01:20.106297 2022: Get account information is true","2Tue Jun 21 13:01:20.106297 2022: Get account information
	 * is true","3Tue Jun 21 13:01:20.106297 2022: Get account information is true"],"type":"true","type2":"true2"}
	 *
	 * @test Has unit test.
	 */
	std::string ToJson() const noexcept;

	/**************************
	 * @brief Call ToString() inside.
	 */
	operator std::string();

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::string operator+(const std::string& str, Json& json);

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& os, Json& json);
};

#define JsonNodeTypes                                                                                                  \
	MSAPI::Json, std::list<MSAPI::JsonNode>, std::string, double, int64_t, uint64_t, bool, std::nullptr_t

template <typename T> constexpr bool is_json_node_type = is_any_of<T, JsonNodeTypes>;

/**************************
 * @brief Variant with possible node types. Exists to provide a way to store Json data in a tree structure.
 * @brief Supported types are: Json (object), std::list<JsonNode> (array), std::string, double (float),
 * int64_t (any positive integral), uint64_t (any negative integral), bool, std::nullptr_t (empty).
 */
class JsonNode {
private:
	std::variant<JsonNodeTypes> m_value;
	bool m_valid{ false };

public:
	/**************************
	 * @brief Construct a new Json Node Json object, empty constructor.
	 *
	 * @tparam T Type of value, except Json or array of JsonNode.
	 *
	 * @test Has unit test.
	 */
	template <typename T>
		requires((is_json_node_type<std::decay_t<T>> || is_any_constructible_from<T, JsonNodeTypes>)
					&& !std::is_same_v<std::decay_t<T>, Json> && !std::is_same_v<std::decay_t<T>, std::list<JsonNode>>)
	JsonNode(T&& value) noexcept
		: m_value{ std::forward<T>(value) }
		, m_valid{ true }
	{
	}

	/**************************
	 * @brief Construct a new Json Node Json object, empty constructor.
	 *
	 * @param json Json value.
	 *
	 * @test Has unit test.
	 */
	template <typename T>
		requires std::is_same_v<std::decay_t<T>, Json>
	JsonNode(T&& json) noexcept
		: m_value{ std::forward<T>(json) }
		, m_valid{ std::get_if<std::decay_t<T>>(&m_value)->Valid() }
	{
	}

	/**************************
	 * @brief Construct a new Json Node Array object, parse array's items inside.
	 *
	 * @param body Body of json.
	 * @param begin Start position of array.
	 * @param end End position of array.
	 *
	 * @test Has unit test.
	 */
	JsonNode(std::string_view body, size_t begin, size_t end);

	/**************************
	 * @brief Construct a new Json Node Array object, parse array's items inside.
	 *
	 * @param array List of JsonNode.
	 *
	 * @test Has unit test.
	 */
	template <typename T>
		requires std::is_same_v<std::decay_t<T>, std::list<JsonNode>>
	JsonNode(T&& array)
		: m_value{ std::forward<T>(array) }
		, m_valid{ std::all_of(std::get_if<std::decay_t<T>>(&m_value)->begin(),
			  std::get_if<std::decay_t<T>>(&m_value)->end(), [](const auto& node) { return node.Valid(); }) }
	{
	}

	/**************************
	 * @brief Guaranties that node has a value.
	 *
	 * @return True if node is valid.
	 *
	 * @test Has unit test.
	 */
	bool Valid() const noexcept;

	/**************************
	 * @return Readable reference on value of node.
	 *
	 * @test Has unit test.
	 */
	const std::variant<JsonNodeTypes>& GetValue() const noexcept;

	/**************************
	 * @return std::string representation of node.
	 *
	 * @test Has unit test.
	 */
	std::string ToString() const noexcept;

private:
	/**************************
	 * @brief Prepare json node value for ToJson() in Json class.
	 *
	 * @test Has unit test.
	 */
	std::string PrepareToJson() const noexcept;

	/**************************
	 * @brief Call PrepareToJson() inside.
	 */
	friend std::string Json::ToJson() const noexcept;
};

}; //* namespace MSAPI

#endif //* MSAPI_JSON_H