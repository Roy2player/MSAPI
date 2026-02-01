/**************************
 * @file        standard.h
 * @version     6.0
 * @date        2024-04-09
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
 * @brief Standard protocol for reserving and sending data messages which contains: all integer and float types and
 * their optional versions, std::string, bool, MSAPI::Timer, MSAPI::Timer::Duration and MSAPI::TableData. Data is
 * contained in map with property identifier as a key and pair of variant and type specifier as a value. Protocol data
 * objects contains cipher which can be used to identify messages for different purposes.
 *
 * @note Ciphers from 934875930 to 934875939 are reserved for standard protocol.
 */

#ifndef MSAPI_STANDARD_PROTOCOL_H
#define MSAPI_STANDARD_PROTOCOL_H

#include "../help/log.h"
#include "../help/standardType.hpp"
#include "../help/table.h"
#include "dataHeader.h"

namespace MSAPI {

namespace StandardProtocol {

constexpr size_t cipherActionHello{ 934875930 };
constexpr size_t cipherMetadataResponse{ 934875931 };
constexpr size_t cipherParametersResponse{ 934875932 };
constexpr size_t cipherMetadataRequest{ 934875933 };
constexpr size_t cipherParametersRequest{ 934875934 };
constexpr size_t cipherActionPause{ 934875935 };
constexpr size_t cipherActionRun{ 934875936 };
constexpr size_t cipherActionDelete{ 934875937 };
constexpr size_t cipherActionModify{ 934875938 };

/**************************
 * @brief Object for containing data of standard message.
 */
class Data : public DataHeader {
private:
	std::map<size_t, std::variant<standardTypes>> m_data;
	std::map<size_t, StandardType::Type> m_dataTypes;

public:
	/**************************
	 * @brief Constructor for creating empty data before sending, empty constructor.
	 *
	 * @note Ciphers from 934875930 to 934875939 are reserved for standard protocol.
	 *
	 * @test Has unit test.
	 */
	Data(size_t cipher);

	/**************************
	 * @brief Constructor for parsing data from buffer.
	 *
	 * @param header Header of data.
	 * @param buffer Buffer with data.
	 *
	 * @test Has unit test.
	 */
	Data(const DataHeader& header, void* buffer);

	/**************************
	 * @return Size of buffer.
	 */
	size_t GetBufferSize() const noexcept;

	/**************************
	 * @brief Set data to message.
	 *
	 * @tparam T type of data.
	 *
	 * @param key Identifier of data, should be unique. If not - data will not be set.
	 * @param value Data.
	 *
	 * @test Has unit test.
	 */
	template <typename T>
		requires(is_standard_type<std::remove_cv_t<std::remove_reference_t<T>>>
			|| std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, TableBase>
			|| std::derived_from<std::remove_cv_t<std::remove_reference_t<T>>, TableBase>)
	void SetData(const size_t key, T&& value)
	{
		using S = std::remove_cv_t<std::remove_reference_t<T>>;
		if (m_data.find(key) != m_data.end()) {
			LOG_WARNING("Setting of data has been interrupted, data with key " + _S(key) + " already exist");
			return;
		}

#define TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(type, standardType)                                                      \
	m_data.emplace(key, std::forward<T>(value));                                                                       \
	m_dataTypes.emplace(key, StandardType::Type::standardType);                                                        \
	m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t) + sizeof(type);

		if constexpr (std::is_same_v<S, int8_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int8_t, Int8)
		}
		else if constexpr (std::is_same_v<S, int16_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int16_t, Int16)
		}
		else if constexpr (std::is_same_v<S, int32_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int32_t, Int32)
		}
		else if constexpr (std::is_same_v<S, int64_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int64_t, Int64)
		}
		else if constexpr (std::is_same_v<S, uint8_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint8_t, Uint8)
		}
		else if constexpr (std::is_same_v<S, uint16_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint16_t, Uint16)
		}
		else if constexpr (std::is_same_v<S, uint32_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint32_t, Uint32)
		}
		else if constexpr (std::is_same_v<S, uint64_t>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint64_t, Uint64)
		}
		else if constexpr (std::is_same_v<S, double>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(double, Double)
		}
		else if constexpr (std::is_same_v<S, float>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(float, Float)
		}
		else if constexpr (std::is_same_v<S, bool>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(bool, Bool)

#define TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(type, standardType, emptyStandardType)                          \
	m_data.emplace(key, std::forward<T>(value));                                                                       \
	if (value.has_value()) {                                                                                           \
		m_dataTypes.emplace(key, StandardType::Type::standardType);                                                    \
		m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t) + sizeof(type);                                    \
	}                                                                                                                  \
	else {                                                                                                             \
		m_dataTypes.emplace(key, StandardType::Type::emptyStandardType);                                               \
		m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t);                                                   \
	}
		}
		else if constexpr (std::is_same_v<S, std::optional<int8_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(int8_t, OptionalInt8, OptionalInt8Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<int16_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(int16_t, OptionalInt16, OptionalInt16Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<int32_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(int32_t, OptionalInt32, OptionalInt32Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<int64_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(int64_t, OptionalInt64, OptionalInt64Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<uint8_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(uint8_t, OptionalUint8, OptionalUint8Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<uint16_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(uint16_t, OptionalUint16, OptionalUint16Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<uint32_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(uint32_t, OptionalUint32, OptionalUint32Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<uint64_t>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(uint64_t, OptionalUint64, OptionalUint64Empty)
		}
		else if constexpr (std::is_same_v<S, std::optional<double>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(double, OptionalDouble, OptionalDoubleEmpty)
		}
		else if constexpr (std::is_same_v<S, std::optional<float>>) {
			TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA(float, OptionalFloat, OptionalFloatEmpty)
#undef TMP_MSAPI_STANDARD_SET_OPTIONAL_PRIMITIVE_DATA
		}
		else if constexpr (std::is_same_v<S, std::string>) {
			if (value.empty()) {
				m_data.emplace(key, "");
				m_dataTypes.emplace(key, StandardType::Type::StringEmpty);
				m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t);
			}
			else {
				m_dataTypes.emplace(key, StandardType::Type::String);
				m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t) + sizeof(size_t) + value.size();
				m_data.emplace(key, std::forward<T>(value));
			}
		}
		else if constexpr (std::is_same_v<S, Timer>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(Timer, Timer)
		}
		else if constexpr (std::is_same_v<S, Timer::Duration>) {
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(Timer::Duration, Duration)

#undef TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA
		}
		else if constexpr (std::is_same_v<S, TableData>) {
			m_dataTypes.emplace(key, StandardType::Type::TableData);
			m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t) + value.GetBufferSize();
			m_data.emplace(key, std::forward<T>(value));
		}
		else if constexpr (std::is_same_v<S, TableBase> || std::derived_from<S, TableBase>) {
			m_dataTypes.emplace(key, StandardType::Type::TableData);
			m_bufferSize += sizeof(StandardType::Type) + sizeof(size_t) + value.GetBufferSize();
			m_data.emplace(key, TableData{ value });
		}
		else {
			static_assert(sizeof(S) + 1 == 0, "Setting of data has been interrupted, unsupported type");
		}
	}

	/**************************
	 * @brief Encode contained data to buffer. Message template is: (size_t) cipher, (size_t) buffer size, (size_t) key,
	 * (short) type specifier [(T) value, [(size_t) string size, (char) value..]]
	 *
	 * @attention Freeing up memory after using is required.
	 *
	 * @return pointer to reallocated memory.
	 *
	 * @test Has unit test.
	 */
	void* Encode() const;

	/**************************
	 * @brief Clear data containers and buffer size.
	 *
	 * @test Has unit test.
	 */
	void Clear();

	/**************************
	 * @example Standard data:
	 * {
	 * 		Cipher : 934875933
	 * 		Buffer size : 608
	 * 		1 (Int8) : 1
	 * 		2 (Int16) : 2
	 * 		3 (Int32) : 3
	 * 		4 (Int64) : 4
	 * 		5 (Uint8) : 5
	 * 		6 (Uint16) : 6
	 * 		7 (Uint32) : 7
	 * 		8 (Uint64) : 8
	 * 		9 (Float) : 9.0000000000000000
	 * 		10 (Double) : 10.0000000000000000
	 * 		11 (OptionalInt8) : 12
	 * 		12 (EmptyOptionalInt8) :
	 * 		13 (OptionalInt16) : 14
	 * 		14 (EmptyOptionalInt16) :
	 * 		15 (OptionalInt32) : 16
	 * 		16 (EmptyOptionalInt32) :
	 * 		17 (OptionalInt64) : 18
	 * 		18 (EmptyOptionalInt64) :
	 * 		19 (OptionalUint8) : 20
	 * 		20 (EmptyOptionalUint8) :
	 * 		21 (OptionalUint16) : 22
	 * 		22 (EmptyOptionalUint16) :
	 * 		23 (OptionalUint32) : 24
	 * 		24 (EmptyOptionalUint32) :
	 * 		25 (OptionalUint64) : 26
	 * 		26 (EmptyOptionalUint64) :
	 * 		27 (OptionalFloat) : 28.0000000000000000
	 * 		28 (EmptyOptionalFloat) :
	 * 		29 (OptionalDouble) : 30.0000000000000000
	 * 		30 (EmptyOptionalDouble) :
	 * 		31 (String) : 34
	 * 		32 (EmptyString) :
	 * 		33 (Timer) : 2024-04-10 21:21:09.663603851
	 * 		34 (Timer) : 1970-01-01 00:00:00.000000000
	 * 		35 (Duration) : 938445099987653 nanoseconds
	 * 		36 (Duration) : 0 nanoseconds
	 * 		47 (TableData) : Encoded table with 1690 bytes size
	 * 		48 (TableData) : Encoded table with 8 bytes size
	 * }
	 */
	std::string ToString() const;

	/**************************
	 * @return Readable reference to data.
	 */
	const std::map<size_t, std::variant<standardTypes>>& GetData() const noexcept;

	/**************************
	 * @return Readable reference to data types.
	 */
	const std::map<size_t, StandardType::Type>& GetDataTypes() const noexcept;
};

/**************************
 * @brief Send data to connection.
 *
 * @param connection Socket to send.
 * @param data Data to send.
 *
 * @test Has unit test.
 */
void Send(int connection, const Data& data);

/**************************
 * @brief Send pause message to connection.
 *
 * @param connection Socket to send.
 *
 * @test Has unit test.
 */
void SendActionPause(int connection);

/**************************
 * @brief Send run message to connection.
 *
 * @param connection Socket to send.
 *
 * @test Has unit test.
 */
void SendActionRun(int connection);

/**************************
 * @brief Send delete message to connection.
 *
 * @param connection Socket to send.
 *
 * @test Has unit test.
 */
void SendActionDelete(int connection);

/**************************
 * @brief Send hello message to connection.
 *
 * @param connection Socket to send.
 *
 * @test Has unit test.
 */
void SendActionHello(int connection);

/**************************
 * @brief Send metadata request message to connection.
 *
 * @param connection Socket to send.
 *
 * @test Has unit test.
 */
void SendMetadataRequest(int connection);

/**************************
 * @brief Send parameters request message to connection.
 *
 * @param connection Socket to send.
 *
 * @test Has unit test.
 */
void SendParametersRequest(int connection);

}; //* namespace StandardProtocol

}; //* namespace MSAPI

#endif //* MSAPI_STANDARD_PROTOCOL_H