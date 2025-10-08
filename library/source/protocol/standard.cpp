/**************************
 * @file        standard.cpp
 * @version     6.0
 * @date        2024-04-09
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

#include "standard.h"
#include "../test/test.h"
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

bool Data::UNITTEST()
{
	LOG_INFO_UNITTEST("MSAPI Standard Data");
	MSAPI::Test t;

	const auto checkEmpty{ [&t] [[nodiscard]] (const Data& data) {
		RETURN_IF_FALSE(t.Assert(data.ToString(), "Standard data:\n{\n\tCipher : 934875933\n\tBuffer size : 16\n}",
			"Standard Data ToString empty"));
		RETURN_IF_FALSE(t.Assert(data.GetData().empty(), true, "Standard Data GetData empty"));
		RETURN_IF_FALSE(t.Assert(data.GetBufferSize(), 16, "Standard Data buffer size is expected"));
		RETURN_IF_FALSE(t.Assert(data.GetDataTypes().empty(), true, "Standard Data GetDataTypes empty"));
		return true;
	} };

	Data data(934875933);

	RETURN_IF_FALSE(checkEmpty(data));

	const int8_t dataItem1{ 1 };
	data.SetData(1, dataItem1);
	const int16_t dataItem2{ 2 };
	data.SetData(2, dataItem2);
	const int32_t dataItem3{ 3 };
	data.SetData(3, dataItem3);
	const int64_t dataItem4{ 4 };
	data.SetData(4, dataItem4);
	const uint8_t dataItem5{ 5 };
	data.SetData(5, dataItem5);
	const uint16_t dataItem6{ 6 };
	data.SetData(6, dataItem6);
	const uint32_t dataItem7{ 7 };
	data.SetData(7, dataItem7);
	const uint64_t dataItem8{ 8 };
	data.SetData(8, dataItem8);
	const float dataItem9{ 9.0f };
	data.SetData(9, dataItem9);
	const double dataItem10{ 10.0 };
	data.SetData(10, dataItem10);
	const double dataItem11{ 11.0 };
	data.SetData(11, dataItem11);
	const std::optional<int8_t> dataItem12{ 12 };
	data.SetData(12, dataItem12);
	const std::optional<int8_t> dataItem13;
	data.SetData(13, dataItem13);
	const std::optional<int16_t> dataItem14{ 14 };
	data.SetData(14, dataItem14);
	const std::optional<int16_t> dataItem15;
	data.SetData(15, dataItem15);
	const std::optional<int32_t> dataItem16{ 16 };
	data.SetData(16, dataItem16);
	const std::optional<int32_t> dataItem17;
	data.SetData(17, dataItem17);
	const std::optional<int64_t> dataItem18{ 18 };
	data.SetData(18, dataItem18);
	const std::optional<int64_t> dataItem19;
	data.SetData(19, dataItem19);
	const std::optional<uint8_t> dataItem20{ 20 };
	data.SetData(20, dataItem20);
	const std::optional<uint8_t> dataItem21;
	data.SetData(21, dataItem21);
	const std::optional<uint16_t> dataItem22{ 22 };
	data.SetData(22, dataItem22);
	const std::optional<uint16_t> dataItem23;
	data.SetData(23, dataItem23);
	const std::optional<uint32_t> dataItem24{ 24 };
	data.SetData(24, dataItem24);
	const std::optional<uint32_t> dataItem25;
	data.SetData(25, dataItem25);
	const std::optional<uint64_t> dataItem26{ 26 };
	data.SetData(26, dataItem26);
	const std::optional<uint64_t> dataItem27;
	data.SetData(27, dataItem27);
	const std::optional<float> dataItem28{ 28.0f };
	data.SetData(28, dataItem28);
	const std::optional<float> dataItem29;
	data.SetData(29, dataItem29);
	const std::optional<double> dataItem30{ 30.0 };
	data.SetData(30, dataItem30);
	const std::optional<double> dataItem31;
	data.SetData(31, dataItem31);
	const std::optional<double> dataItem32{ 32.0 };
	data.SetData(32, dataItem32);
	const std::optional<double> dataItem33;
	data.SetData(33, dataItem33);
	const std::string dataItem34{ "34" };
	data.SetData(34, dataItem34);
	const std::string dataItem35;
	data.SetData(35, dataItem35);
	const MSAPI::Timer dataItem36{ 1756075436, 163998971 };
	data.SetData(36, dataItem36);
	const MSAPI::Timer dataItem37{ 0 };
	data.SetData(37, dataItem37);
	const MSAPI::Timer::Duration dataItem38{ MSAPI::Timer::Duration::Create(10, 20, 40, 45, 99987653) };
	data.SetData(38, dataItem38);
	const MSAPI::Timer::Duration dataItem39;
	data.SetData(39, dataItem39);

	Table<std::optional<uint64_t>, Timer, std::string, Timer::Duration, double> table{ 111111, 222222, 333333, 444444,
		555555 };

	struct CustomTableData : public Table<std::optional<uint64_t>, Timer, std::string, Timer::Duration, double> {
		int m_customDataInt{ 0 };
		double m_customDataDouble{ 0.0 };
	} customTable;

	{
		std::array<std::optional<uint64_t>, 20> bufferOptionalUint64{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<Timer, 20> bufferTimer{ Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			Timer::Create(2023, 11, 27, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			Timer::Create(2023, 11, 27, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0) };
		std::array<std::string, 20> bufferString{ "0 Some random string here", "Some -1 random string here",
			"Some random --2 string here", "Some random string ---3 here", "Some random string here ----4",
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here ",
			"Some random string --------8 here", "Some random string here---------9", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here ", "Some random string --------8 here",
			"Some random string here---------9" };
		std::array<Timer::Duration, 20> bufferTimerDuration{ Timer::Duration::CreateNanoseconds(7929342421),
			Timer::Duration::CreateMicroseconds(348238), Timer::Duration::CreateMicroseconds(348225223423438),
			Timer::Duration::CreateMicroseconds(343248238), Timer::Duration::CreateMilliseconds(234234),
			Timer::Duration::CreateSeconds(28434), Timer::Duration::CreateMinutes(23453),
			Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441), Timer::Duration::CreateMinutes(0),
			Timer::Duration{ 7929342421 }, Timer::Duration::CreateMicroseconds(348238),
			Timer::Duration::CreateMicroseconds(348225223423438), Timer::Duration::CreateMicroseconds(343248238),
			Timer::Duration::CreateMilliseconds(234234), Timer::Duration::CreateSeconds(28434),
			Timer::Duration::CreateMinutes(23453), Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441),
			Timer::Duration::CreateMinutes(0) };
		std::array<double, 20> bufferDouble{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001,
			-7.89123456, 8.91234567, -9.12345678, 10.23456789, -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235,
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789 };

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferOptionalUint64[row], bufferTimer[row], bufferString[row], bufferTimerDuration[row],
				bufferDouble[row]);
			customTable.AddRow(bufferOptionalUint64[row], bufferTimer[row], bufferString[row], bufferTimerDuration[row],
				bufferDouble[row]);
		}
	}

	data.SetData(40, table);
	const Table<MSAPI::Timer, std::string> table2{ 1997, 2024 };
	data.SetData(41, table2);
	data.SetData(42, customTable);
	TableData tableData{ customTable };
	data.SetData(43, tableData);

	AutoClearPtr<void> buffer{ data.Encode() };
	DataHeader header{ buffer.ptr };
	Data copyData{ header, buffer.ptr };

	RETURN_IF_FALSE(
		t.Assert(data.ToString(), copyData.ToString(), "For standard data from buffer ToString is as on source data"));
	RETURN_IF_FALSE(t.Assert(data.GetDataTypes() == copyData.GetDataTypes(), true,
		"For standard data from buffer GetDataTypes is as on source data"));
	RETURN_IF_FALSE(t.Assert(
		data.GetData() == copyData.GetData(), true, "For standard data from buffer GetData is as on source data"));

	RETURN_IF_FALSE(t.Assert(data.GetBufferSize(), 4663, "Data buffer size is correct for huge object"));
	RETURN_IF_FALSE(t.Assert(data.ToString(),
		std ::string_view{ "Standard data:"
						   "\n{"
						   "\n\tCipher : 934875933"
						   "\n\tBuffer size : 4663"
						   "\n\t1 (Int8) : 1"
						   "\n\t2 (Int16) : 2"
						   "\n\t3 (Int32) : 3"
						   "\n\t4 (Int64) : 4"
						   "\n\t5 (Uint8) : 5"
						   "\n\t6 (Uint16) : 6"
						   "\n\t7 (Uint32) : 7"
						   "\n\t8 (Uint64) : 8"
						   "\n\t9 (Float) : 9.000000000"
						   "\n\t10 (Double) : 10.00000000000000000"
						   "\n\t11 (Double) : 11.00000000000000000"
						   "\n\t12 (OptionalInt8) : 12"
						   "\n\t13 (OptionalInt8Empty) : "
						   "\n\t14 (OptionalInt16) : 14"
						   "\n\t15 (OptionalInt16Empty) : "
						   "\n\t16 (OptionalInt32) : 16"
						   "\n\t17 (OptionalInt32Empty) : "
						   "\n\t18 (OptionalInt64) : 18"
						   "\n\t19 (OptionalInt64Empty) : "
						   "\n\t20 (OptionalUint8) : 20"
						   "\n\t21 (OptionalUint8Empty) : "
						   "\n\t22 (OptionalUint16) : 22"
						   "\n\t23 (OptionalUint16Empty) : "
						   "\n\t24 (OptionalUint32) : 24"
						   "\n\t25 (OptionalUint32Empty) : "
						   "\n\t26 (OptionalUint64) : 26"
						   "\n\t27 (OptionalUint64Empty) : "
						   "\n\t28 (OptionalFloat) : 28.000000000"
						   "\n\t29 (OptionalFloatEmpty) : "
						   "\n\t30 (OptionalDouble) : 30.00000000000000000"
						   "\n\t31 (OptionalDoubleEmpty) : "
						   "\n\t32 (OptionalDouble) : 32.00000000000000000"
						   "\n\t33 (OptionalDoubleEmpty) : "
						   "\n\t34 (String) : 34"
						   "\n\t35 (StringEmpty) : "
						   "\n\t36 (Timer) : 2025-08-24 22:43:56.163998971"
						   "\n\t37 (Timer) : 1970-01-01 00:00:00.000000000"
						   "\n\t38 (Duration) : 938445099987653 nanoseconds"
						   "\n\t39 (Duration) : 0 nanoseconds"
						   "\n\t40 (TableData) : Encoded table with 1370 bytes size"
						   "\n\t41 (TableData) : Encoded table with 8 bytes size"
						   "\n\t42 (TableData) : Encoded table with 1370 bytes size"
						   "\n\t43 (TableData) : Encoded table with 1370 bytes size"
						   "\n}" },
		"Data to string is correct for huge object"));

	RETURN_IF_FALSE(t.Assert(data.GetDataTypes()
			== std::map<size_t, StandardType::Type>{ { 1, StandardType::Type::Int8 }, { 2, StandardType::Type::Int16 },
				{ 3, StandardType::Type::Int32 }, { 4, StandardType::Type::Int64 }, { 5, StandardType::Type::Uint8 },
				{ 6, StandardType::Type::Uint16 }, { 7, StandardType::Type::Uint32 }, { 8, StandardType::Type::Uint64 },
				{ 9, StandardType::Type::Float }, { 10, StandardType::Type::Double },
				{ 11, StandardType::Type::Double }, { 12, StandardType::Type::OptionalInt8 },
				{ 13, StandardType::Type::OptionalInt8Empty }, { 14, StandardType::Type::OptionalInt16 },
				{ 15, StandardType::Type::OptionalInt16Empty }, { 16, StandardType::Type::OptionalInt32 },
				{ 17, StandardType::Type::OptionalInt32Empty }, { 18, StandardType::Type::OptionalInt64 },
				{ 19, StandardType::Type::OptionalInt64Empty }, { 20, StandardType::Type::OptionalUint8 },
				{ 21, StandardType::Type::OptionalUint8Empty }, { 22, StandardType::Type::OptionalUint16 },
				{ 23, StandardType::Type::OptionalUint16Empty }, { 24, StandardType::Type::OptionalUint32 },
				{ 25, StandardType::Type::OptionalUint32Empty }, { 26, StandardType::Type::OptionalUint64 },
				{ 27, StandardType::Type::OptionalUint64Empty }, { 28, StandardType::Type::OptionalFloat },
				{ 29, StandardType::Type::OptionalFloatEmpty }, { 30, StandardType::Type::OptionalDouble },
				{ 31, StandardType::Type::OptionalDoubleEmpty }, { 32, StandardType::Type::OptionalDouble },
				{ 33, StandardType::Type::OptionalDoubleEmpty }, { 34, StandardType::Type::String },
				{ 35, StandardType::Type::StringEmpty }, { 36, StandardType::Type::Timer },
				{ 37, StandardType::Type::Timer }, { 38, StandardType::Type::Duration },
				{ 39, StandardType::Type::Duration }, { 40, StandardType::Type::TableData },
				{ 41, StandardType::Type::TableData }, { 42, StandardType::Type::TableData },
				{ 43, StandardType::Type::TableData } },
		true, "Data types are expected for huge object"));

	RETURN_IF_FALSE(t.Assert(data.GetData()
			== std::map<size_t, std::variant<standardTypes>>{ { 1, dataItem1 }, { 2, dataItem2 }, { 3, dataItem3 },
				{ 4, dataItem4 }, { 5, dataItem5 }, { 6, dataItem6 }, { 7, dataItem7 }, { 8, dataItem8 },
				{ 9, dataItem9 }, { 10, dataItem10 }, { 11, dataItem11 }, { 12, dataItem12 }, { 13, dataItem13 },
				{ 14, dataItem14 }, { 15, dataItem15 }, { 16, dataItem16 }, { 17, dataItem17 }, { 18, dataItem18 },
				{ 19, dataItem19 }, { 20, dataItem20 }, { 21, dataItem21 }, { 22, dataItem22 }, { 23, dataItem23 },
				{ 24, dataItem24 }, { 25, dataItem25 }, { 26, dataItem26 }, { 27, dataItem27 }, { 28, dataItem28 },
				{ 29, dataItem29 }, { 30, dataItem30 }, { 31, dataItem31 }, { 32, dataItem32 }, { 33, dataItem33 },
				{ 34, dataItem34 }, { 35, dataItem35 }, { 36, dataItem36 }, { 37, dataItem37 }, { 38, dataItem38 },
				{ 39, dataItem39 }, { 40, table }, { 41, table2 }, { 42, customTable }, { 43, tableData } },
		true, "Data is expected for huge object"));

	data.Clear();
	RETURN_IF_FALSE(checkEmpty(data));

	return true;
}

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