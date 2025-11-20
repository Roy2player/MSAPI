/**************************
 * @file        table.cpp
 * @version     6.0
 * @date        2024-05-12
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

#include "table.h"
#include "json.h"
#include "meta.hpp"

namespace MSAPI {

/*---------------------------------------------------------------------------------
TableBase
---------------------------------------------------------------------------------*/

void TableBase::Copy([[maybe_unused]] const TableData& TableData)
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
}

std::string TableBase::ToString() const
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
	return "";
}

std::string TableBase::ToJson() const
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
	return "";
}

bool TableBase::Empty() const noexcept
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
	return false;
}

const std::list<TableBase::Column>* TableBase::GetColumns() const noexcept
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
	return nullptr;
}

size_t TableBase::GetBufferSize() const noexcept
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
	return 0;
}

void* TableBase::Encode() const
{
	LOG_ERROR("Called method from pure Table Base interface class, data is casted wrongly");
	return nullptr;
}

/*---------------------------------------------------------------------------------
Column
---------------------------------------------------------------------------------*/

TableBase::Column::Column(const size_t id, const StandardType::Type type)
	: id{ id }
	, type{ type }
{
}

/*---------------------------------------------------------------------------------
TableData
---------------------------------------------------------------------------------*/

TableData::TableData(const TableBase& table) noexcept
	: m_ownBuffer{ std::make_shared<AutoClearPtr<void>>(table.Encode()) }
	, m_bufferSize{ table.GetBufferSize() }
{
}

TableData::TableData(void* buffer)
	: m_sharedBuffer{ buffer }
{
	if (buffer != nullptr) {
		m_bufferSize = *static_cast<size_t*>(buffer);
	}
}

TableData::TableData(const std::list<JsonNode>& rows, const std::vector<StandardType::Type>& columnTypes)
{
	if (rows.empty()) {
		LOG_DEBUG("Forming table data is interrupted. Rows are empty");
		return;
	}

	size_t allocationStep{ 0 };
	for (const auto columnType : columnTypes) {
		switch (columnType) {
		case StandardType::Type::Int8:
			allocationStep += 1;
			break;
		case StandardType::Type::Int16:
			allocationStep += 2;
			break;
		case StandardType::Type::Int32:
			allocationStep += 4;
			break;
		case StandardType::Type::Int64:
			allocationStep += 8;
			break;
		case StandardType::Type::Uint8:
			allocationStep += 1;
			break;
		case StandardType::Type::Uint16:
			allocationStep += 2;
			break;
		case StandardType::Type::Uint32:
			allocationStep += 4;
			break;
		case StandardType::Type::Uint64:
			allocationStep += 8;
			break;
		case StandardType::Type::Double:
			allocationStep += 8;
			break;
		case StandardType::Type::Float:
			allocationStep += 4;
			break;
		case StandardType::Type::Bool:
			allocationStep += 1;
			break;
		case StandardType::Type::OptionalInt8:
			allocationStep += 2;
			break;
		case StandardType::Type::OptionalInt16:
			allocationStep += 3;
			break;
		case StandardType::Type::OptionalInt32:
			allocationStep += 5;
			break;
		case StandardType::Type::OptionalInt64:
			allocationStep += 9;
			break;
		case StandardType::Type::OptionalUint8:
			allocationStep += 2;
			break;
		case StandardType::Type::OptionalUint16:
			allocationStep += 3;
			break;
		case StandardType::Type::OptionalUint32:
			allocationStep += 5;
			break;
		case StandardType::Type::OptionalUint64:
			allocationStep += 9;
			break;
		case StandardType::Type::OptionalDouble:
			allocationStep += 9;
			break;
		case StandardType::Type::OptionalFloat:
			allocationStep += 5;
			break;
		case StandardType::Type::String:
			allocationStep += 72; /* 8 bytes for size and 64 bytes for content */
			break;
		case StandardType::Type::Timer:
			allocationStep += sizeof(MSAPI::Timer);
			break;
		case StandardType::Type::Duration:
			allocationStep += sizeof(MSAPI::Timer::Duration);
			break;
		default:
			LOG_ERROR_NEW("Forming table data is interrupted. Unsupported column type for table data: {}",
				StandardType::EnumToString(columnType));
			return;
		}
	}

	size_t allocatedSize{ allocationStep * rows.size() + sizeof(size_t) };
	void* buffer{ malloc(allocatedSize) };
	if (buffer == nullptr) {
		LOG_ERROR("Forming table data is interrupted. Cannot allocate memory for encoding table. Error №" + _S(errno)
			+ ": " + std::strerror(errno));
		return;
	}
	size_t offset{ sizeof(size_t) };

	for (const auto& row : rows) {
		const auto* rowArray{ std::get_if<std::list<JsonNode>>(&row.GetValue()) };
		if (rowArray == nullptr) {
			LOG_WARNING("Forming table data is interrupted. Row is not an array");
			free(buffer);
			return;
		}

		if (rowArray->size() != columnTypes.size()) {
			LOG_WARNING(
				"Forming table data is interrupted. Row has unexpected number of columns: " + _S(rowArray->size()));
			free(buffer);
			return;
		}

#define TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY(size)                                                            \
	if (offset + size > allocatedSize) {                                                                               \
		allocatedSize += std::max(allocationStep, size * 2);                                                           \
		void* newBuffer{ realloc(buffer, allocatedSize) };                                                             \
		if (newBuffer == nullptr) {                                                                                    \
			LOG_ERROR("Forming table data is interrupted. Cannot reallocate memory for encoding table. Error №"      \
				+ _S(errno) + ": " + std::strerror(errno));                                                            \
			free(buffer);                                                                                              \
			return;                                                                                                    \
		}                                                                                                              \
		buffer = newBuffer;                                                                                            \
	}

#define TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(type, jsonType)                                                         \
	if (const auto* cellValue{ std::get_if<jsonType>(cellVariant) }; cellValue != nullptr) {                           \
		TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY(sizeof(type))                                                    \
		/* static cast from uint64_t to int64_t produces a warning in case with Timer and Duration */                  \
		/* but for double that can lead to unexpected values in buffer, that can be due to the fact that FP uses */    \
		/*different registers and operations set and compiller have to generate different code to process it */        \
		if constexpr (sizeof(type) == sizeof(jsonType) && !is_float_type<type>) {                                      \
			*reinterpret_cast<jsonType*>(&reinterpret_cast<char*>(buffer)[offset]) = *cellValue;                       \
		}                                                                                                              \
		else {                                                                                                         \
			*reinterpret_cast<type*>(&reinterpret_cast<char*>(buffer)[offset]) = static_cast<type>(*cellValue);        \
		}                                                                                                              \
                                                                                                                       \
		offset += sizeof(type);                                                                                        \
		break;                                                                                                         \
	}

#define TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR                                                                           \
	LOG_ERROR_NEW("Forming table data is interrupted. Update for column index: {} has unexpected type: {}", index,     \
		StandardType::EnumToString(columnTypes[index]));                                                               \
	free(buffer);                                                                                                      \
	return;

		size_t index{ 0 };
		for (const auto& cell : *rowArray) {
			const auto* cellVariant{ &cell.GetValue() };
			switch (columnTypes[index]) {
			case StandardType::Type::Int8:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int8_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int8_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Int16:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int16_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int16_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Int32:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int32_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int32_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Int64:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int64_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(int64_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Uint8:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(uint8_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Uint16:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(uint16_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Uint32:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(uint32_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Uint64:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(uint64_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Double:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(double, double);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(double, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(double, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Float:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(float, double);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(float, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(float, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::Bool:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(bool, bool);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;

#define TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL                                                                      \
	if (std::holds_alternative<std::nullptr_t>(*cellVariant)) {                                                        \
		TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY(sizeof(bool))                                                    \
		*reinterpret_cast<bool*>(&reinterpret_cast<char*>(buffer)[offset]) = true;                                     \
		offset += sizeof(bool);                                                                                        \
		break;                                                                                                         \
	}

#define TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(type, jsonType)                                                          \
	if (const auto* cellValue{ std::get_if<jsonType>(cellVariant) }; cellValue != nullptr) {                           \
		TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY(sizeof(type) + sizeof(bool))                                     \
		*reinterpret_cast<bool*>(&reinterpret_cast<char*>(buffer)[offset]) = false;                                    \
		offset += sizeof(bool);                                                                                        \
		*reinterpret_cast<type*>(&reinterpret_cast<char*>(buffer)[offset]) = static_cast<type>(*cellValue);            \
		offset += sizeof(type);                                                                                        \
		break;                                                                                                         \
	}

			case StandardType::Type::OptionalInt8:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int8_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int8_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalInt16:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int16_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int16_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalInt32:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int32_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int32_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalInt64:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int64_t, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(int64_t, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalUint8:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(uint8_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalUint16:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(uint16_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalUint32:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(uint32_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalUint64:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(uint64_t, uint64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalDouble:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(double, double);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(double, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(double, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			case StandardType::Type::OptionalFloat:
				TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL;
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(float, double);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(float, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL(float, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;

#undef TMP_MSAPI_TABLE_TRY_SET_OPTIONAL_CELL
#undef TMP_MSAPI_TABLE_CHECK_EMPTY_OPTIONAL_CELL

			case StandardType::Type::String: {
				if (const auto* cellValue{ std::get_if<std::string>(cellVariant) }; cellValue != nullptr) {
					const auto cellValueSize{ cellValue->size() };
					TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY(sizeof(size_t) + cellValueSize)
					*reinterpret_cast<size_t*>(&reinterpret_cast<char*>(buffer)[offset]) = cellValueSize;
					offset += sizeof(size_t);
					std::memcpy(&reinterpret_cast<char*>(buffer)[offset], cellValue->c_str(), cellValueSize);
					offset += cellValueSize;
					break;
				}
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			}
			case StandardType::Type::Timer: {
				if (const auto* cellValue{ std::get_if<std::uint64_t>(cellVariant) }; cellValue != nullptr) {
					TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY(sizeof(MSAPI::Timer))
					*reinterpret_cast<MSAPI::Timer*>(&reinterpret_cast<char*>(buffer)[offset])
						= MSAPI::Timer{ INT64(*cellValue) / 1000000000, INT64(*cellValue) % 1000000000 };
					offset += sizeof(MSAPI::Timer);
					break;
				}
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;
			}
			case StandardType::Type::Duration:
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(Timer::Duration, uint64_t);
				TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL(Timer::Duration, int64_t);
				TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR;

#undef TMP_MSAPI_TABLE_TRY_SET_PRIMITIVE_CELL
#undef TMP_MSAPI_TABLE_CHECK_ALLOCATED_SIZE_VALIDITY
#undef TMP_MSAPI_TABLE_INTERRUPT_WITH_ERROR

			default:
				LOG_ERROR_NEW("Forming table data is interrupted. Unsupported column type for table data: {}",
					StandardType::EnumToString(columnTypes[index]));
				free(buffer);
				return;
			}

			++index;
		}
	}

#define suppress
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free" //* free(buffer) is safe operation, but is marked as error
	void* newBuffer{ realloc(buffer, offset) };
	if (newBuffer == nullptr) [[unlikely]] {
		LOG_ERROR("Forming table data is interrupted. Cannot reallocate memory for encoding table. Error №" + _S(errno)
			+ ": " + std::strerror(errno));
		free(buffer);
		return;
	}
#pragma GCC diagnostic pop
#undef suppress

	*reinterpret_cast<size_t*>(newBuffer) = offset;
	m_ownBuffer = std::make_unique<AutoClearPtr<void>>(newBuffer);
	m_bufferSize = offset;
}

const void* TableData::GetBuffer() const noexcept
{
	if (m_sharedBuffer == nullptr) {
		const auto* buffer{ m_ownBuffer.get() };
		if (buffer == nullptr) {
			if (m_bufferSize == sizeof(size_t)) {
				return &m_bufferSize;
			}
			return buffer;
		}

		return buffer->ptr;
	}

	return m_sharedBuffer;
}

size_t TableData::GetBufferSize() const noexcept { return m_bufferSize; }

std::string TableData::ToString() const { return std::format("Encoded table with {} bytes size", m_bufferSize); }

std::string TableData::LookUpToJson(const std::vector<StandardType::Type>& columnTypes) const
{
	const void* const buffer{ GetBuffer() };
	if (buffer == nullptr) {
		return {};
	}
	if (m_bufferSize == sizeof(size_t)) {
		return { "{\"Buffer size\":" + _S(m_bufferSize) + ",\"Rows\":[]}" };
	}

	std::string json;
	json.reserve(m_bufferSize);
	json = "{\"Buffer size\":" + _S(m_bufferSize) + ",\"Rows\":[[";

	size_t offset{ sizeof(size_t) };
	while (true) {
		for (size_t index{ 0 };; ++index) {
			switch (columnTypes[index]) {
			case StandardType::Type::Int8:

#define ADD_PRIMITIVE_TYPE(type)                                                                                       \
	json += _S(reinterpret_cast<const type*>(&reinterpret_cast<const char*>(buffer)[offset]));                         \
	offset += sizeof(type);

				ADD_PRIMITIVE_TYPE(int8_t)
				break;
			case StandardType::Type::Int16:
				ADD_PRIMITIVE_TYPE(int16_t)
				break;
			case StandardType::Type::Int32:
				ADD_PRIMITIVE_TYPE(int32_t)
				break;
			case StandardType::Type::Int64:
				ADD_PRIMITIVE_TYPE(int64_t)
				break;
			case StandardType::Type::Uint8:
				ADD_PRIMITIVE_TYPE(uint8_t)
				break;
			case StandardType::Type::Uint16:
				ADD_PRIMITIVE_TYPE(uint16_t)
				break;
			case StandardType::Type::Uint32:
				ADD_PRIMITIVE_TYPE(uint32_t)
				break;
			case StandardType::Type::Uint64:
				ADD_PRIMITIVE_TYPE(uint64_t)
				break;
			case StandardType::Type::Double:
				ADD_PRIMITIVE_TYPE(double)
				break;
			case StandardType::Type::Float:
				ADD_PRIMITIVE_TYPE(float)
				break;
			case StandardType::Type::Bool:
				ADD_PRIMITIVE_TYPE(bool)

#undef ADD_PRIMITIVE_TYPE
#define ADD_OPTIONAL_PRIMITIVE_TYPE(type)                                                                              \
	if (*reinterpret_cast<const bool*>(&reinterpret_cast<const char*>(buffer)[offset])) {                              \
		json += "null";                                                                                                \
		offset += sizeof(bool);                                                                                        \
		break;                                                                                                         \
	}                                                                                                                  \
	offset += sizeof(bool);                                                                                            \
	json += std::format("{}", *reinterpret_cast<const type*>(&reinterpret_cast<const char*>(buffer)[offset]));         \
	offset += sizeof(type);

				break;
			case StandardType::Type::OptionalInt8:
				ADD_OPTIONAL_PRIMITIVE_TYPE(int8_t)
				break;
			case StandardType::Type::OptionalInt16:
				ADD_OPTIONAL_PRIMITIVE_TYPE(int16_t)
				break;
			case StandardType::Type::OptionalInt32:
				ADD_OPTIONAL_PRIMITIVE_TYPE(int32_t)
				break;
			case StandardType::Type::OptionalInt64:
				ADD_OPTIONAL_PRIMITIVE_TYPE(int64_t)
				break;
			case StandardType::Type::OptionalUint8:
				ADD_OPTIONAL_PRIMITIVE_TYPE(uint8_t)
				break;
			case StandardType::Type::OptionalUint16:
				ADD_OPTIONAL_PRIMITIVE_TYPE(uint16_t)
				break;
			case StandardType::Type::OptionalUint32:
				ADD_OPTIONAL_PRIMITIVE_TYPE(uint32_t)
				break;
			case StandardType::Type::OptionalUint64:
				ADD_OPTIONAL_PRIMITIVE_TYPE(uint64_t)
				break;
			case StandardType::Type::OptionalDouble:
				ADD_OPTIONAL_PRIMITIVE_TYPE(double)
				break;
			case StandardType::Type::OptionalFloat:
				ADD_OPTIONAL_PRIMITIVE_TYPE(float)

#undef ADD_OPTIONAL_PRIMITIVE_TYPE

				break;
			case StandardType::Type::String: {
				size_t size;
				memcpy(&size, &reinterpret_cast<const char*>(buffer)[offset], sizeof(size_t));
				offset += sizeof(size_t);
				if (size == 0) {
					json += "\"\"";
					break;
				}
				const auto sizeBytes{ size };
				json += "\"" + std::move(std::string{ &reinterpret_cast<const char*>(buffer)[offset], sizeBytes })
					+ "\"";
				offset += sizeBytes;
			} break;
			case StandardType::Type::Timer:
				json += std::format("{}",
					reinterpret_cast<const Timer*>(&reinterpret_cast<const char*>(buffer)[offset])->GetNanoseconds());
				offset += sizeof(Timer);
				break;
			case StandardType::Type::Duration:
				json += std::format("{}",
					reinterpret_cast<const Timer::Duration*>(&reinterpret_cast<const char*>(buffer)[offset])
						->GetNanoseconds());
				offset += sizeof(Timer::Duration);
				break;
			default:
				LOG_ERROR_NEW("Unexpected column type for table: {}", StandardType::EnumToString(columnTypes[index]));
				return {};
			}

			if (index == columnTypes.size() - 1) {
				break;
			}
			json += ",";
		}

		if (offset > m_bufferSize) {
			LOG_ERROR("Buffer overflow during table data lookup: " + _S(offset) + " > " + _S(m_bufferSize));
			return {};
		}

		if (offset == m_bufferSize) {
			break;
		}

		json += "],[";
	}

	json += "]]}";
	return json;
}


}; //* namespace MSAPI