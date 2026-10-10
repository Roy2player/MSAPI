/**************************
 * @file        standard.cpp
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
#include "../server/connection.inl"
#include <cstring>
#include <limits>
#include <memory.h>
#include <sys/socket.h>

namespace MSAPI {

namespace Protocol {

namespace Standard {

/*---------------------------------------------------------------------------------
Data
---------------------------------------------------------------------------------*/

Data::Data(const uint64_t cipher)
	: DataHeader(cipher)
{
}

Data::Data(const DataHeader& header, const void* buffer)
	: DataHeader(header)
{
	uint64_t offset{ DataHeader::HEADER_SIZE };
	StandardType::Type type [[indeterminate]];
	uint64_t key{};

	while (m_bufferSize > offset) {
		memcpy(&type, &static_cast<const char*>(buffer)[offset], sizeof(type));
		offset += sizeof(type);

		memcpy(&key, &static_cast<const char*>(buffer)[offset], sizeof(uint64_t));
		offset += sizeof(uint64_t);

		switch (type) {
		case StandardType::Type::Int8:

#define TMP_MSAPI_STANDARD_SET_PRIMITIVE_DATA(type)                                                                    \
	m_data.emplace(key, *reinterpret_cast<const type*>(&static_cast<const char*>(buffer)[offset]));                    \
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
	m_data.emplace(                                                                                                    \
		key, std::optional<type>{ *reinterpret_cast<const type*>(&static_cast<const char*>(buffer)[offset]) });        \
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
			if (offset > m_bufferSize || m_bufferSize - offset < sizeof(uint64_t)) [[unlikely]] {
				LOG_ERROR_NEW("Incomplete string size in standard message, key: {}, offset: {}, buffer size: {}, "
							  "required size: {}",
					key, offset, m_bufferSize, sizeof(uint64_t));
				return;
			}

			uint64_t stringSize [[indeterminate]];
			memcpy(&stringSize, &static_cast<const char*>(buffer)[offset], sizeof(uint64_t));
			offset += sizeof(uint64_t);

			if constexpr (sizeof(size_t) < sizeof(uint64_t)) {
				if (stringSize > std::numeric_limits<size_t>::max()) [[unlikely]] {
					LOG_ERROR_NEW("String size exceeds addressable size in standard message, key: {}, string size: {}, "
								  "maximum size: {}",
						key, stringSize, std::numeric_limits<size_t>::max());
					return;
				}
			}

			if (offset > m_bufferSize || stringSize > m_bufferSize - offset) [[unlikely]] {
				LOG_ERROR_NEW(
					"String exceeds standard message buffer, key: {}, string size: {}, offset: {}, buffer size: {}",
					key, stringSize, offset, m_bufferSize);
				return;
			}

			m_data.emplace(
				key, std::string{ &static_cast<const char*>(buffer)[offset], static_cast<size_t>(stringSize) });
			offset += stringSize;
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
		case StandardType::Type::TableData: {
			if (offset > m_bufferSize || m_bufferSize - offset < sizeof(size_t)) [[unlikely]] {
				LOG_ERROR_NEW("Incomplete table size in standard message, key: {}, offset: {}, buffer size: {}, "
							  "required size: {}",
					key, offset, m_bufferSize, sizeof(size_t));
				return;
			}

			size_t tableSize [[indeterminate]];
			memcpy(&tableSize, &static_cast<const char*>(buffer)[offset], sizeof(size_t));

			if (tableSize < sizeof(size_t) || tableSize > m_bufferSize - offset) [[unlikely]] {
				LOG_ERROR_NEW(
					"Invalid table size in standard message, key: {}, table size: {}, offset: {}, buffer size: {}", key,
					tableSize, offset, m_bufferSize);
				return;
			}

			m_data.emplace(key, TableData{ &static_cast<const char*>(buffer)[offset] });
			offset += tableSize;
		} break;
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
	if (buffer == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Cannot allocate memory for encoding data. Error №{}: {}", errno, std::strerror(errno));
		return nullptr;
	}

	memcpy(static_cast<char*>(buffer), &m_cipher, sizeof(uint64_t));

	size_t offset{ sizeof(uint64_t) };
	memcpy(&static_cast<char*>(buffer)[offset], &m_bufferSize, sizeof(uint64_t));
	offset += sizeof(uint64_t);

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

		memcpy(&static_cast<char*>(buffer)[offset], &key, sizeof(uint64_t));
		offset += sizeof(uint64_t);

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
					const uint64_t stringSize{ value.size() };
					memcpy(&static_cast<char*>(buffer)[offset], &stringSize, sizeof(uint64_t));
					offset += sizeof(uint64_t);

					memcpy(&static_cast<char*>(buffer)[offset], value.data(), stringSize);
					offset += stringSize;
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
	m_bufferSize = DataHeader::HEADER_SIZE;
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

const std::map<uint64_t, std::variant<standardTypes>>& Data::GetData() const noexcept { return m_data; }

const std::map<uint64_t, StandardType::Type>& Data::GetDataTypes() const noexcept { return m_dataTypes; }

/*---------------------------------------------------------------------------------
Global
---------------------------------------------------------------------------------*/

void Send(Connection& connection, const Data& data)
{
	LOG_PROTOCOL_NEW("Send {} to connection id: {}", data.ToString(), connection.GetId());
	AutoClearPtr<void> ptr{ data.Encode() };
	if (ptr.Get() == nullptr) [[unlikely]] {
		return;
	}

	(void)connection.Send(ptr.Get(), data.GetBufferSize(), MSG_NOSIGNAL);
}

void SendActionPause(Connection& connection)
{
	static const struct Buffer {
		uint64_t cipher{ CIPHER_ACTION_PAUSE };
		uint64_t bufferSize{ DataHeader::HEADER_SIZE };
	} buffer;
	LOG_PROTOCOL_NEW("Send action pause to connection id: {}", connection.GetId());

	(void)connection.Send(&buffer, DataHeader::HEADER_SIZE, MSG_NOSIGNAL);
}

void SendActionRun(Connection& connection)
{
	static const struct Buffer {
		uint64_t cipher{ CIPHER_ACTION_RUN };
		uint64_t bufferSize{ DataHeader::HEADER_SIZE };
	} buffer;
	LOG_PROTOCOL_NEW("Send action run to connection id: {}", connection.GetId());

	(void)connection.Send(&buffer, DataHeader::HEADER_SIZE, MSG_NOSIGNAL);
}

void SendActionDelete(Connection& connection)
{
	static const struct Buffer {
		uint64_t cipher{ CIPHER_ACTION_DELETE };
		uint64_t bufferSize{ DataHeader::HEADER_SIZE };
	} buffer;
	LOG_PROTOCOL_NEW("Send action delete to connection id: {}", connection.GetId());

	(void)connection.Send(&buffer, DataHeader::HEADER_SIZE, MSG_NOSIGNAL);
}

void SendActionHello(Connection& connection)
{
	static const struct Buffer {
		uint64_t cipher{ CIPHER_ACTION_HELLO };
		uint64_t bufferSize{ DataHeader::HEADER_SIZE };
	} buffer;
	LOG_PROTOCOL_NEW("Send action hello to connection id: {}", connection.GetId());

	(void)connection.Send(&buffer, DataHeader::HEADER_SIZE, MSG_NOSIGNAL);
}

void SendMetadataRequest(Connection& connection)
{
	static const struct Buffer {
		uint64_t cipher{ CIPHER_METADATA_REQUEST };
		uint64_t bufferSize{ DataHeader::HEADER_SIZE };
	} buffer;
	LOG_PROTOCOL_NEW("Send metadata request to connection id: {}", connection.GetId());

	(void)connection.Send(&buffer, DataHeader::HEADER_SIZE, MSG_NOSIGNAL);
}

void SendParametersRequest(Connection& connection)
{
	static const struct Buffer {
		uint64_t cipher{ CIPHER_PARAMETERS_REQUEST };
		uint64_t bufferSize{ DataHeader::HEADER_SIZE };
	} buffer;
	LOG_PROTOCOL_NEW("Send parameters request to connection id: {}", connection.GetId());

	(void)connection.Send(&buffer, DataHeader::HEADER_SIZE, MSG_NOSIGNAL);
}

} // namespace Standard

} // namespace Protocol

} // namespace MSAPI