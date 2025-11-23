/**************************
 * @file        standardData.inl
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

#ifndef MSAPI_TEST_STANDARD_DATA_INL
#define MSAPI_TEST_STANDARD_DATA_INL

#include "../../../../library/source/protocol/standard.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for StandardData.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool StandardData();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool StandardData()
{
	LOG_INFO_UNITTEST("MSAPI Standard Data");
	MSAPI::Test t;

	const auto checkEmpty{ [&t] [[nodiscard]] (const MSAPI::StandardProtocol::Data& data) {
		RETURN_IF_FALSE(t.Assert(data.ToString(), "Standard data:\n{\n\tCipher : 934875933\n\tBuffer size : 16\n}",
			"Standard Data ToString empty"));
		RETURN_IF_FALSE(t.Assert(data.GetData().empty(), true, "Standard Data GetData empty"));
		RETURN_IF_FALSE(t.Assert(data.GetBufferSize(), 16, "Standard Data buffer size is expected"));
		RETURN_IF_FALSE(t.Assert(data.GetDataTypes().empty(), true, "Standard Data GetDataTypes empty"));
		return true;
	} };

	MSAPI::StandardProtocol::Data data(934875933);

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

	MSAPI::Table<std::optional<uint64_t>, MSAPI::Timer, std::string, MSAPI::Timer::Duration, double> table{ 111111,
		222222, 333333, 444444, 555555 };

	struct CustomTableData
		: public MSAPI::Table<std::optional<uint64_t>, MSAPI::Timer, std::string, MSAPI::Timer::Duration, double> {
		int m_customDataInt{ 0 };
		double m_customDataDouble{ 0.0 };
	} customTable;

	{
		std::array<std::optional<uint64_t>, 20> bufferOptionalUint64{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<MSAPI::Timer, 20> bufferTimer{ MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0) };
		std::array<std::string, 20> bufferString{ "0 Some random string here", "Some -1 random string here",
			"Some random --2 string here", "Some random string ---3 here", "Some random string here ----4",
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here ",
			"Some random string --------8 here", "Some random string here---------9", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here ", "Some random string --------8 here",
			"Some random string here---------9" };
		std::array<MSAPI::Timer::Duration, 20> bufferTimerDuration{
			MSAPI::Timer::Duration::CreateNanoseconds(7929342421), MSAPI::Timer::Duration::CreateMicroseconds(348238),
			MSAPI::Timer::Duration::CreateMicroseconds(348225223423438),
			MSAPI::Timer::Duration::CreateMicroseconds(343248238), MSAPI::Timer::Duration::CreateMilliseconds(234234),
			MSAPI::Timer::Duration::CreateSeconds(28434), MSAPI::Timer::Duration::CreateMinutes(23453),
			MSAPI::Timer::Duration::CreateHours(264), MSAPI::Timer::Duration::CreateDays(6441),
			MSAPI::Timer::Duration::CreateMinutes(0), MSAPI::Timer::Duration{ 7929342421 },
			MSAPI::Timer::Duration::CreateMicroseconds(348238),
			MSAPI::Timer::Duration::CreateMicroseconds(348225223423438),
			MSAPI::Timer::Duration::CreateMicroseconds(343248238), MSAPI::Timer::Duration::CreateMilliseconds(234234),
			MSAPI::Timer::Duration::CreateSeconds(28434), MSAPI::Timer::Duration::CreateMinutes(23453),
			MSAPI::Timer::Duration::CreateHours(264), MSAPI::Timer::Duration::CreateDays(6441),
			MSAPI::Timer::Duration::CreateMinutes(0)
		};
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
	const MSAPI::Table<MSAPI::Timer, std::string> table2{ 1997, 2024 };
	data.SetData(41, table2);
	data.SetData(42, customTable);
	MSAPI::TableData tableData{ customTable };
	data.SetData(43, tableData);

	MSAPI::AutoClearPtr<void> buffer{ data.Encode() };
	MSAPI::DataHeader header{ buffer.ptr };
	MSAPI::StandardProtocol::Data copyData{ header, buffer.ptr };

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
			== std::map<size_t, MSAPI::StandardType::Type>{ { 1, MSAPI::StandardType::Type::Int8 },
				{ 2, MSAPI::StandardType::Type::Int16 }, { 3, MSAPI::StandardType::Type::Int32 },
				{ 4, MSAPI::StandardType::Type::Int64 }, { 5, MSAPI::StandardType::Type::Uint8 },
				{ 6, MSAPI::StandardType::Type::Uint16 }, { 7, MSAPI::StandardType::Type::Uint32 },
				{ 8, MSAPI::StandardType::Type::Uint64 }, { 9, MSAPI::StandardType::Type::Float },
				{ 10, MSAPI::StandardType::Type::Double }, { 11, MSAPI::StandardType::Type::Double },
				{ 12, MSAPI::StandardType::Type::OptionalInt8 }, { 13, MSAPI::StandardType::Type::OptionalInt8Empty },
				{ 14, MSAPI::StandardType::Type::OptionalInt16 }, { 15, MSAPI::StandardType::Type::OptionalInt16Empty },
				{ 16, MSAPI::StandardType::Type::OptionalInt32 }, { 17, MSAPI::StandardType::Type::OptionalInt32Empty },
				{ 18, MSAPI::StandardType::Type::OptionalInt64 }, { 19, MSAPI::StandardType::Type::OptionalInt64Empty },
				{ 20, MSAPI::StandardType::Type::OptionalUint8 }, { 21, MSAPI::StandardType::Type::OptionalUint8Empty },
				{ 22, MSAPI::StandardType::Type::OptionalUint16 },
				{ 23, MSAPI::StandardType::Type::OptionalUint16Empty },
				{ 24, MSAPI::StandardType::Type::OptionalUint32 },
				{ 25, MSAPI::StandardType::Type::OptionalUint32Empty },
				{ 26, MSAPI::StandardType::Type::OptionalUint64 },
				{ 27, MSAPI::StandardType::Type::OptionalUint64Empty },
				{ 28, MSAPI::StandardType::Type::OptionalFloat }, { 29, MSAPI::StandardType::Type::OptionalFloatEmpty },
				{ 30, MSAPI::StandardType::Type::OptionalDouble },
				{ 31, MSAPI::StandardType::Type::OptionalDoubleEmpty },
				{ 32, MSAPI::StandardType::Type::OptionalDouble },
				{ 33, MSAPI::StandardType::Type::OptionalDoubleEmpty }, { 34, MSAPI::StandardType::Type::String },
				{ 35, MSAPI::StandardType::Type::StringEmpty }, { 36, MSAPI::StandardType::Type::Timer },
				{ 37, MSAPI::StandardType::Type::Timer }, { 38, MSAPI::StandardType::Type::Duration },
				{ 39, MSAPI::StandardType::Type::Duration }, { 40, MSAPI::StandardType::Type::TableData },
				{ 41, MSAPI::StandardType::Type::TableData }, { 42, MSAPI::StandardType::Type::TableData },
				{ 43, MSAPI::StandardType::Type::TableData } },
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

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_TEST_STANDARD_DATA_INL