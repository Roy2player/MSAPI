/**************************
 * @file        standard.cpp
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
 */

#include "standard.h"
#include <cstring>
#include <memory.h>
#include <sys/socket.h>

namespace MSAPI {

namespace StandardProtocol {

/*---------------------------------------------------------------------------------
Data
---------------------------------------------------------------------------------*/

Data::Data(const size_t cipher)
	: DataHeader(cipher)
{
}

Data::Data(const DataHeader& header, void* buffer)
	: DataHeader(header)
{
	size_t offset{ sizeof(size_t) * 2 };
	StandardType::Type type{ StandardType::Type::Undefined };
	size_t key{ 0 };

	while (m_bufferSize > offset) {
		memcpy(&type, &static_cast<char*>(buffer)[offset], sizeof(type));
		offset += sizeof(type);

		memcpy(&key, &static_cast<char*>(buffer)[offset], sizeof(size_t));
		offset += sizeof(size_t);

		switch (type) {
		case StandardType::Type::Int8:

#define TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(type)                                                                    \
	m_data.emplace(key, *reinterpret_cast<type*>(&static_cast<char*>(buffer)[offset]));                                \
	offset += sizeof(type);

			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int8_t);
			break;
		case StandardType::Type::Int16:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int16_t);
			break;
		case StandardType::Type::Int32:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int32_t);
			break;
		case StandardType::Type::Int64:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(int64_t);
			break;
		case StandardType::Type::Uint8:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint8_t);
			break;
		case StandardType::Type::Uint16:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint16_t);
			break;
		case StandardType::Type::Uint32:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint32_t);
			break;
		case StandardType::Type::Uint64:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(uint64_t);
			break;
		case StandardType::Type::Double:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(double);
			break;
		case StandardType::Type::Float:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(float);
			break;
		case StandardType::Type::Bool:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(bool);
			break;
		case StandardType::Type::OptionalInt8:

#define TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(type)                                                                     \
	m_data.emplace(key, std::optional<type>{ *reinterpret_cast<type*>(&static_cast<char*>(buffer)[offset]) });         \
	offset += sizeof(type);

			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(int8_t);
			break;
		case StandardType::Type::OptionalInt8Empty:
			m_data.emplace(key, std::optional<int8_t>{});
			break;
		case StandardType::Type::OptionalInt16:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(int16_t);
			break;
		case StandardType::Type::OptionalInt16Empty:
			m_data.emplace(key, std::optional<int16_t>{});
			break;
		case StandardType::Type::OptionalInt32:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(int32_t);
			break;
		case StandardType::Type::OptionalInt32Empty:
			m_data.emplace(key, std::optional<int32_t>{});
			break;
		case StandardType::Type::OptionalInt64:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(int64_t);
			break;
		case StandardType::Type::OptionalInt64Empty:
			m_data.emplace(key, std::optional<int64_t>{});
			break;
		case StandardType::Type::OptionalUint8:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(uint8_t);
			break;
		case StandardType::Type::OptionalUint8Empty:
			m_data.emplace(key, std::optional<uint8_t>{});
			break;
		case StandardType::Type::OptionalUint16:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(uint16_t);
			break;
		case StandardType::Type::OptionalUint16Empty:
			m_data.emplace(key, std::optional<uint16_t>{});
			break;
		case StandardType::Type::OptionalUint32:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(uint32_t);
			break;
		case StandardType::Type::OptionalUint32Empty:
			m_data.emplace(key, std::optional<uint32_t>{});
			break;
		case StandardType::Type::OptionalUint64:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(uint64_t);
			break;
		case StandardType::Type::OptionalUint64Empty:
			m_data.emplace(key, std::optional<uint64_t>{});
			break;
		case StandardType::Type::OptionalDouble:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(double);
			break;
		case StandardType::Type::OptionalDoubleEmpty:
			m_data.emplace(key, std::optional<double>{});
			break;
		case StandardType::Type::OptionalFloat:
			TMP_MSAPI_STANDARD_SET_OPTIONAL_DATA(float);
			break;
		case StandardType::Type::OptionalFloatEmpty:
			m_data.emplace(key, std::optional<float>{});
			break;
		case StandardType::Type::String: {
			size_t size;
			memcpy(&size, &static_cast<char*>(buffer)[offset], sizeof(size_t));
			offset += sizeof(size_t);
			m_data.emplace(key, std::string{ &static_cast<char*>(buffer)[offset], size });
			offset += size;
		} break;
		case StandardType::Type::StringEmpty:
			m_data.emplace(key, std::string{});
			break;
		case StandardType::Type::Timer:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(Timer);
			break;
		case StandardType::Type::Duration:
			TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(Timer::Duration);
			break;
		case StandardType::Type::TableData:
			m_data.emplace(key, TableData{ &static_cast<char*>(buffer)[offset] });
			offset += *reinterpret_cast<size_t*>(&static_cast<char*>(buffer)[offset]);
			break;
		default:
			LOG_ERROR("Parsing of message object encountered an error, unsupported type: "
				+ _S(static_cast<short>(type)) + ", key: " + _S(key));
			return;
		}
		m_dataTypes.emplace(key, type);
	}
}

size_t Data::GetBufferSize() const noexcept { return m_bufferSize; }

void* Data::Encode() const
{
	void* buffer{ malloc(m_bufferSize) };
	memcpy(static_cast<char*>(buffer), &m_cipher, sizeof(size_t));

	size_t offset{ sizeof(size_t) };
	memcpy(&static_cast<char*>(buffer)[offset], &m_bufferSize, sizeof(size_t));
	offset += sizeof(size_t);

	if (m_data.empty()) [[unlikely]] {
		return buffer;
	}

	for (const auto& [key, value] : m_data) {
		const auto typeIter{ m_dataTypes.find(key) };
		if (typeIter == m_dataTypes.end()) {
			LOG_ERROR("Encoding of item has been skipped, unknown type, key: " + _S(key));
			continue;
		}
		memcpy(&static_cast<char*>(buffer)[offset], &typeIter->second, sizeof(StandardType::Type));
		offset += sizeof(StandardType::Type);

		memcpy(&static_cast<char*>(buffer)[offset], &key, sizeof(size_t));
		offset += sizeof(size_t);

		std::visit(
			[this, &buffer, &offset](auto&& value) {
				using T = std::decay_t<decltype(value)>;
				if constexpr (is_standard_primitive_type<T> || std::is_same_v<T, Timer>
					|| std::is_same_v<T, Timer::Duration>) {

					memcpy(&static_cast<char*>(buffer)[offset], &value, sizeof(T));
					offset += sizeof(T);
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					if (value.empty()) {
						return;
					}
					const auto stringSize{ value.size() };
					memcpy(&static_cast<char*>(buffer)[offset], &stringSize, sizeof(size_t));
					offset += sizeof(size_t);

					const auto stringSizeBytes{ stringSize };
					memcpy(&static_cast<char*>(buffer)[offset], value.data(), stringSizeBytes);
					offset += stringSizeBytes;
				}
				else if constexpr (is_standard_primitive_type_optional<T>) {
					if (value.has_value()) {
						using S = remove_optional_t<T>;
						memcpy(&static_cast<char*>(buffer)[offset], &(value.value()), sizeof(S));
						offset += sizeof(S);
					}
				}
				else if constexpr (std::is_same_v<T, TableData>) {
					const auto tableSize{ value.GetBufferSize() };
					memcpy(&static_cast<char*>(buffer)[offset], value.GetBuffer(), tableSize);
					offset += tableSize;
				}
				else {
					static_assert(sizeof(T) + 1 == 0, "Encoding of item has been skipped, unsupported type");
				}
			},
			value);
	}

	return buffer;
}

void Data::Clear()
{
	m_data.clear();
	m_dataTypes.clear();
	m_bufferSize = sizeof(size_t) * 2;
}

std::string Data::ToString() const
{
	std::string result;
	BI(result, "Standard data:\n{{\n\tCipher : {}\n\tBuffer size : {}", m_cipher, m_bufferSize);

	for (const auto& [key, value] : m_data) {
		std::visit(
			[this, &key, &result](auto&& value) {
				const auto typeIt{ m_dataTypes.find(key) };
				if (typeIt == m_dataTypes.end()) {
					LOG_ERROR("Printing of item has been skipped, unknown type, key: " + _S(key));
					return;
				}
				BI(result, "\n\t{} ({}) : ", key, StandardType::EnumToString(typeIt->second));
				using T = std::decay_t<decltype(value)>;
				if constexpr (is_standard_simple_type<T>) {
					result += _S(value);
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					if (value.empty()) {
						return;
					}
					result += value;
				}
				else if constexpr (std::is_same_v<T, Timer> || std::is_same_v<T, Timer::Duration>
					|| std::is_same_v<T, TableData>) {

					result += value.ToString();
				}
				else {
					static_assert(sizeof(T) + 1 == 0, "Encoding of item has been skipped, unsupported type");
				}
			},
			value);
	}

	result += "\n}";
	return result;
}

const std::map<size_t, std::variant<standardTypes>>& Data::GetData() const noexcept { return m_data; }

const std::map<size_t, StandardType::Type>& Data::GetDataTypes() const noexcept { return m_dataTypes; }

/*---------------------------------------------------------------------------------
Another
---------------------------------------------------------------------------------*/

void Send(const int connection, const Data& data)
{
	LOG_PROTOCOL("Send " + data.ToString() + " to connection: " + _S(connection));
	AutoClearPtr<void> ptr{ data.Encode() };

	if (send(connection, ptr.ptr, data.GetBufferSize(), MSG_NOSIGNAL) == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			return;
		}
		LOG_ERROR("Send event failed, connection: " + _S(connection) + ", data: " + data.ToString() + ". Error №"
			+ _S(errno) + ": " + std::strerror(errno));
	}
}

#define __STANDARD_PROTOCOL_SEND                                                                                       \
	if (send(connection, &buffer, sizeof(size_t) * 2, MSG_NOSIGNAL) == -1) {                                           \
		if (errno == 104) {                                                                                            \
			LOG_DEBUG("Send returned error №104: Connection reset by peer");                                           \
			return;                                                                                                    \
		}                                                                                                              \
		LOG_ERROR("Send event failed, connection: " + _S(connection) + ". Error №" + _S(errno) + ": "                  \
			+ std::strerror(errno));                                                                                   \
	}

void SendActionPause(const int connection)
{
	static const struct Buffer {
		size_t cipher{ cipherActionPause };
		size_t bufferSize{ sizeof(size_t) * 2 };
	} buffer;
	LOG_PROTOCOL("Send action pause to connection: " + _S(connection));

	__STANDARD_PROTOCOL_SEND
}

void SendActionRun(const int connection)
{
	static const struct Buffer {
		size_t cipher{ cipherActionRun };
		size_t bufferSize{ sizeof(size_t) * 2 };
	} buffer;
	LOG_PROTOCOL("Send action run to connection: " + _S(connection));

	__STANDARD_PROTOCOL_SEND
}

void SendActionDelete(const int connection)
{
	static const struct Buffer {
		size_t cipher{ cipherActionDelete };
		size_t bufferSize{ sizeof(size_t) * 2 };
	} buffer;
	LOG_PROTOCOL("Send action delete to connection: " + _S(connection));

	__STANDARD_PROTOCOL_SEND
}

void SendActionHello(const int connection)
{
	static const struct Buffer {
		size_t cipher{ cipherActionHello };
		size_t bufferSize{ sizeof(size_t) * 2 };
	} buffer;
	LOG_PROTOCOL("Send action hello to connection: " + _S(connection));

	__STANDARD_PROTOCOL_SEND
}

void SendMetadataRequest(const int connection)
{
	static const struct Buffer {
		size_t cipher{ cipherMetadataRequest };
		size_t bufferSize{ sizeof(size_t) * 2 };
	} buffer;
	LOG_PROTOCOL("Send metadata request to connection: " + _S(connection));

	__STANDARD_PROTOCOL_SEND
}

void SendParametersRequest(const int connection)
{
	static const struct Buffer {
		size_t cipher{ cipherParametersRequest };
		size_t bufferSize{ sizeof(size_t) * 2 };
	} buffer;
	LOG_PROTOCOL("Send parameters request to connection: " + _S(connection));

	__STANDARD_PROTOCOL_SEND
}

#undef __STANDARD_PROTOCOL_SEND

}; //* namespace StandardProtocol

}; //* namespace MSAPI