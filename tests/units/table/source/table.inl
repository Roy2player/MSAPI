/**************************
 * @file        table.inl
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

#ifndef MSAPI_UNIT_TEST_TABLE_INL
#define MSAPI_UNIT_TEST_TABLE_INL

#include "../../../../library/source/help/json.h"
#include "../../../../library/source/help/table.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Table.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Table();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Table()
{
	LOG_INFO_UNITTEST("MSAPI Table");
	MSAPI::Test t;

	const auto basicTableCheck{ [&t]<class... Ts> [[nodiscard]] (const MSAPI::Table<Ts...>& table, const size_t rows,
									const std::vector<size_t>& ids) {
		const bool isEmpty{ rows == 0 };
		RETURN_IF_FALSE(t.Assert(table.Empty(), isEmpty, "Table empty state"));
		RETURN_IF_FALSE(t.Assert(table.GetColumnsSize(), ids.size(), "Table columns count"));
		RETURN_IF_FALSE(t.Assert(table.GetRowsSize(), rows, "Table rows count"));
		for (int64_t index{ 0 }; const auto& column : *table.GetColumns()) {
			RETURN_IF_FALSE(t.Assert(column.id, ids[UINT64(index)], std::format("Table column id at index {}", index)));
			++index;
		}

		if (isEmpty) {
			RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), sizeof(size_t), "Table buffer size for empty table"));
			MSAPI::AutoClearPtr<void> tableBuffer{ table.Encode() };
			RETURN_IF_FALSE(
				t.Assert(*static_cast<size_t*>(tableBuffer.ptr), sizeof(size_t), "Table buffer empty size content"));
		}

		return true;
	} };

	const auto checkTableElement{ [&t]<typename T, size_t bufferSize, class... Ts> [[nodiscard]] (
									  const MSAPI::Table<Ts...>& table, const size_t column, const size_t row,
									  const std::array<T, bufferSize>& buffer) {
		const auto* element{ table.GetCell(column, row) };
		RETURN_IF_FALSE(
			t.Assert(element != nullptr, true, std::format("Table element [{}, {}] is not nullptr", column, row)));
		RETURN_IF_FALSE(t.Assert(*reinterpret_cast<const T*>(element), buffer[row],
			std::format("Table element [{}, {}] is correct", column, row)));
		return true;
	} };

	const auto checkTableData{ [&t]<class... Ts> [[nodiscard]] (const MSAPI::Table<Ts...>& table) {
		MSAPI::TableData tableData1{ table };
		RETURN_IF_FALSE(t.Assert(tableData1.GetBuffer() != nullptr, true, "TableData(Table) buffer is not nullptr"));
		RETURN_IF_FALSE(t.Assert(tableData1.GetBufferSize(), table.GetBufferSize(), "TableData(Table) buffer size"));

		MSAPI::AutoClearPtr<void> buffer{ table.Encode() };
		MSAPI::TableData tableData2{ buffer.ptr };
		RETURN_IF_FALSE(t.Assert(tableData2.GetBuffer() != nullptr, true, "TableData(buffer) buffer is not nullptr"));
		RETURN_IF_FALSE(t.Assert(tableData2.GetBufferSize(), table.GetBufferSize(), "TableData(buffer) buffer size"));
		RETURN_IF_FALSE(t.Assert(tableData1, tableData2, "Tables data are equal, operator =="));
		RETURN_IF_FALSE(t.Assert(tableData1 != tableData2, false, "Tables data are equal, operator !="));

		const std::string expectedString{ "Encoded table with " + _S(tableData1.GetBufferSize()) + " bytes size" };
		RETURN_IF_FALSE(t.Assert(tableData1.ToString(), expectedString, "TableData1 ToString"));
		RETURN_IF_FALSE(t.Assert(tableData2.ToString(), expectedString, "TableData2 ToString"));
		return true;
	} };

	const auto checkCopy{ [&t]<class... Ts> [[nodiscard]] (const MSAPI::Table<Ts...>& table) {
		auto tableCopy{ table };
		RETURN_IF_FALSE(t.Assert(table == tableCopy, true, "Table is equal to its copy, operator =="));
		RETURN_IF_FALSE(t.Assert(!(table != tableCopy), true, "Table is equal to its copy, operator !="));
		if (!table.Empty()) {
			tableCopy.Clear();
			RETURN_IF_FALSE(t.Assert(table != tableCopy, true, "Table is not equal to its cleared copy, operator !="));
			RETURN_IF_FALSE(
				t.Assert(!(table == tableCopy), true, "Table is not equal to its cleared copy, operator =="));
		}
		return true;
	} };

	{
		MSAPI::Table<bool, bool, bool> table{ 111111, 111112, 111113 };
		RETURN_IF_FALSE(checkCopy(table));
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 111112, 111113 }));
		RETURN_IF_FALSE(checkTableData(table));

		const auto checkEmptyPrints{ [&table, &t] [[nodiscard]] () {
			const std::string expectedString1{ "Table:\n{\n\tBuffer size: " + _S(sizeof(size_t))
				+ "\n\tColumns:\n\t{\n\t\t[0] 111111 Bool\n\t\t[1] 111112 Bool\n\t\t[2] 111113 Bool\n\t}\n}" };
			RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

			const std::string expectedString2{ "{\"Buffer size\":" + _S(sizeof(size_t))
				+ ",\"Columns\":[{\"id\":111111,\"type\":\"Bool\"},{\"id\":111112,\"type\":\"Bool\"},{\"id\":111113,"
				  "\"type\":\"Bool\"}],\"Rows\":[]}" };
			RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

			return true;
		} };

		RETURN_IF_FALSE(checkEmptyPrints());

		std::array<bool, 20> bufferBool{ true, true, false, true, false, false, false, true, true, false, true, true,
			true, false, true, false, true, true, true, false };

		for (const auto item : bufferBool) {
			table.AddRow(item, item, item);
		}

		RETURN_IF_FALSE(basicTableCheck(table, 20, { 111111, 111112, 111113 }));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, bool>(table, 0, row, bufferBool)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, bool>(table, 1, row, bufferBool)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, bool>(table, 2, row, bufferBool)));
		}

		const std::string_view expectedString1{
			"Table:\n{\n\tBuffer size: 68\n\tColumns:\n\t{\n\t\t[0] 111111 Bool\n\t\t[1] 111112 Bool\n\t\t[2] "
			"111113 Bool\n\t}\n\tRows:\n\t{\n\t\t[0, 0] true [1| true [2| true\n\t\t[0, 1] true [1| true [2| "
			"true\n\t\t[0, 2] false [1| false [2| false\n\t\t[0, 3] true [1| true [2| true\n\t\t[0, 4] false [1| "
			"false [2| false\n\t\t[0, 5] false [1| false [2| false\n\t\t[0, 6] false [1| false [2| false\n\t\t[0, "
			"7] true [1| true [2| true\n\t\t[0, 8] true [1| true [2| true\n\t\t[0, 9] false [1| false [2| "
			"false\n\t\t[0, 10] true [1| true [2| true\n\t\t[0, 11] true [1| true [2| true\n\t\t[0, 12] true [1| "
			"true [2| true\n\t\t[0, 13] false [1| false [2| false\n\t\t[0, 14] true [1| true [2| true\n\t\t[0, 15] "
			"false [1| false [2| false\n\t\t[0, 16] true [1| true [2| true\n\t\t[0, 17] true [1| true [2| "
			"true\n\t\t[0, 18] true [1| true [2| true\n\t\t[0, 19] false [1| false [2| false\n\t}\n}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

		const std::string_view expectedString2{
			"{\"Buffer "
			"size\":68,\"Columns\":[{\"id\":111111,\"type\":\"Bool\"},{\"id\":111112,\"type\":\"Bool\"},{\"id\":"
			"111113,\"type\":\"Bool\"}],\"Rows\":[[true,true,true],[true,true,true],[false,false,false],[true,true,"
			"true],[false,false,false],[false,false,false],[false,false,false],[true,true,true],[true,true,true],["
			"false,false,false],[true,true,true],[true,true,true],[true,true,true],[false,false,false],[true,true,"
			"true],[false,false,false],[true,true,true],[true,true,true],[true,true,true],[false,false,false]]}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

		MSAPI::AutoClearPtr<void> buffer{ table.Encode() };
		const auto bufferSize{ table.GetBufferSize() };

		const size_t expectedBufferSize1{ sizeof(bool) * 3 * 20 + sizeof(size_t) };
		RETURN_IF_FALSE(t.Assert(bufferSize, expectedBufferSize1, "Table buffer size"));

		table.Clear();

		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 111112, 111113 }));

		MSAPI::TableData tableData{ buffer.ptr };
		table.Copy(tableData);
		RETURN_IF_FALSE(basicTableCheck(table, 20, { 111111, 111112, 111113 }));

		RETURN_IF_FALSE(t.Assert(bufferSize, expectedBufferSize1, "Table buffer size after copy"));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, bool>(table, 0, row, bufferBool)));
		}

		std::array<bool, 40> bufferBool2{ true, true, false, true, false, false, false, true, true, false, true, true,
			true, false, true, false, true, true, true, false, true, true, false, true, false, false, false, true, true,
			false, true, true, true, false, true, false, true, true, true, false };

		for (const auto item : bufferBool) {
			table.AddRow(item, item, item);
		}

		const auto check{ [&] [[nodiscard]] () {
			RETURN_IF_FALSE(basicTableCheck(table, 40, { 111111, 111112, 111113 }));

			const size_t expectedBufferSize{ sizeof(bool) * 3 * 40 + sizeof(size_t) };
			RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size for 40 rows"));

			for (size_t row{ 0 }; row < 40; ++row) {
				RETURN_IF_FALSE((checkTableElement.operator()<bool, 40, bool>(table, 0, row, bufferBool2)));
				RETURN_IF_FALSE((checkTableElement.operator()<bool, 40, bool>(table, 1, row, bufferBool2)));
				RETURN_IF_FALSE((checkTableElement.operator()<bool, 40, bool>(table, 2, row, bufferBool2)));
			}

			return true;
		} };

		RETURN_IF_FALSE(check());

		std::reverse(bufferBool2.begin(), bufferBool2.end());

		for (size_t row{ 0 }; row < 40; ++row) {
			table.UpdateCell(0, row, bufferBool2[row]);
			table.UpdateCell(1, row, bufferBool2[row]);
			table.UpdateCell(2, row, bufferBool2[row]);
		}

		RETURN_IF_FALSE(checkTableData(table));

		RETURN_IF_FALSE(check());

		RETURN_IF_FALSE(checkCopy(table));
		table.Clear();
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 111112, 111113 }));

		RETURN_IF_FALSE(checkEmptyPrints());
	}

	{
		MSAPI::Table<uint64_t, bool, double> table{ 111111, 222222, 333333 };
		RETURN_IF_FALSE(checkCopy(table));
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 222222, 333333 }));

		RETURN_IF_FALSE(checkTableData(table));

		const auto checkEmptyPrints{ [&table, &t] [[nodiscard]] () {
			const std::string expectedString1{ "Table:\n{\n\tBuffer size: " + _S(sizeof(size_t))
				+ "\n\tColumns:\n\t{\n\t\t[0] 111111 Uint64\n\t\t[1] 222222 Bool\n\t\t[2] 333333 Double\n\t}\n}" };
			RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

			const std::string expectedString2{ "{\"Buffer size\":" + _S(sizeof(size_t))
				+ ",\"Columns\":[{\"id\":111111,\"type\":\"Uint64\"},{\"id\":222222,\"type\":\"Bool\"},{\"id\":333333,"
				  "\"type\":\"Double\"}],\"Rows\":[]}" };
			RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

			return true;
		} };

		RETURN_IF_FALSE(checkEmptyPrints());

		std::array<uint64_t, 20> bufferUint64{ 0, 121, 242, 363, 484, 505, 626, 747, 868, 989, 1010, 1131, 1252, 1373,
			1494, 1515, 1636, 1757, 1878, 1999 };
		std::array<bool, 20> bufferBool{ true, true, false, true, false, false, false, true, true, false, true, true,
			true, false, true, false, true, true, true, false };
		std::array<double, 20> bufferDouble{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001,
			-7.89123456, 8.91234567, -9.12345678, 10.23456789, -11.34567891, 12.45678912, -13.56789123, 14.67891234,
			-15.78912345, 16.89123456, -17.91234567, 18.12345678, -19.23456789, 20.34567891 };

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferUint64[row], bufferBool[row], bufferDouble[row]);
		}

		RETURN_IF_FALSE(basicTableCheck(table, 20, { 111111, 222222, 333333 }));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE(
				(checkTableElement.operator()<uint64_t, 20, uint64_t, bool, double>(table, 0, row, bufferUint64)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<bool, 20, uint64_t, bool, double>(table, 1, row, bufferBool)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<double, 20, uint64_t, bool, double>(table, 2, row, bufferDouble)));
		}

		const std::string_view expectedString1{
			"Table:\n{\n\tBuffer size: 348\n\tColumns:\n\t{\n\t\t[0] 111111 Uint64\n\t\t[1] 222222 Bool\n\t\t[2] "
			"333333 Double\n\t}\n\tRows:\n\t{\n\t\t[0, 0] 0 [1| true [2| -0.84291000000000005\n\t\t[0, 1] 121 [1| "
			"true [2| 0.00000000000000000\n\t\t[0, 2] 242 [1| false [2| 23492.43582999999853200\n\t\t[0, 3] 363 "
			"[1| true [2| -0.00002342340000000\n\t\t[0, 4] 484 [1| false [2| 4583045.00234999973326921\n\t\t[0, 5] "
			"505 [1| false [2| -2342234.23482001014053822\n\t\t[0, 6] 626 [1| false [2| "
			"-7.89123456000000001\n\t\t[0, 7] 747 [1| true [2| 8.91234567000000055\n\t\t[0, 8] 868 [1| true [2| "
			"-9.12345677999999971\n\t\t[0, 9] 989 [1| false [2| 10.23456788999999922\n\t\t[0, 10] 1010 [1| true "
			"[2| -11.34567891000000017\n\t\t[0, 11] 1131 [1| true [2| 12.45678911999999983\n\t\t[0, 12] 1252 [1| "
			"true [2| -13.56789123000000075\n\t\t[0, 13] 1373 [1| false [2| 14.67891234000000011\n\t\t[0, 14] 1494 "
			"[1| true [2| -15.78912344999999995\n\t\t[0, 15] 1515 [1| false [2| 16.89123456000000090\n\t\t[0, 16] "
			"1636 [1| true [2| -17.91234567000000055\n\t\t[0, 17] 1757 [1| true [2| 18.12345678000000149\n\t\t[0, "
			"18] 1878 [1| true [2| -19.23456789000000100\n\t\t[0, 19] 1999 [1| false [2| "
			"20.34567891000000017\n\t}\n}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

		const std::string_view expectedString2{
			"{\"Buffer "
			"size\":348,\"Columns\":[{\"id\":111111,\"type\":\"Uint64\"},{\"id\":222222,\"type\":\"Bool\"},{\"id\":"
			"333333,\"type\":\"Double\"}],\"Rows\":[[0,true,-0.84291000000000005],[121,true,0.00000000000000000],["
			"242,false,23492.43582999999853200],[363,true,-0.00002342340000000],[484,false,4583045."
			"00234999973326921],[505,false,-2342234.23482001014053822],[626,false,-7.89123456000000001],[747,true,"
			"8.91234567000000055],[868,true,-9.12345677999999971],[989,false,10.23456788999999922],[1010,true,-11."
			"34567891000000017],[1131,true,12.45678911999999983],[1252,true,-13.56789123000000075],[1373,false,14."
			"67891234000000011],[1494,true,-15.78912344999999995],[1515,false,16.89123456000000090],[1636,true,-17."
			"91234567000000055],[1757,true,18.12345678000000149],[1878,true,-19.23456789000000100],[1999,false,20."
			"34567891000000017]]}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

		MSAPI::AutoClearPtr<void> buffer{ table.Encode() };

		const size_t expectedBufferSize{ (sizeof(uint64_t) + sizeof(bool) + sizeof(double)) * 20 + sizeof(size_t) };
		RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size"));

		table.Clear();

		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 222222, 333333 }));

		MSAPI::TableData tableData{ buffer.ptr };
		table.Copy(tableData);
		RETURN_IF_FALSE(basicTableCheck(table, 20, { 111111, 222222, 333333 }));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE(
				(checkTableElement.operator()<uint64_t, 20, size_t, bool, double>(table, 0, row, bufferUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, size_t, bool, double>(table, 1, row, bufferBool)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<double, 20, size_t, bool, double>(table, 2, row, bufferDouble)));
		}

		std::array<uint64_t, 40> bufferUint642{ 0, 121, 242, 363, 484, 505, 626, 747, 868, 989, 1010, 1131, 1252, 1373,
			1494, 1515, 1636, 1757, 1878, 1999, 0, 121, 242, 363, 484, 505, 626, 747, 868, 989, 1010, 1131, 1252, 1373,
			1494, 1515, 1636, 1757, 1878, 1999 };
		std::array<bool, 40> bufferBool2{ true, true, false, true, false, false, false, true, true, false, true, true,
			true, false, true, false, true, true, true, false, true, true, false, true, false, false, false, true, true,
			false, true, true, true, false, true, false, true, true, true, false };
		std::array<double, 40> bufferDouble2{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001,
			-7.89123456, 8.91234567, -9.12345678, 10.23456789, -11.34567891, 12.45678912, -13.56789123, 14.67891234,
			-15.78912345, 16.89123456, -17.91234567, 18.12345678, -19.23456789, 20.34567891, -0.84291, 0, 23492.43583,
			-0.0000234234, 4583045.00235, -2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789,
			-11.34567891, 12.45678912, -13.56789123, 14.67891234, -15.78912345, 16.89123456, -17.91234567, 18.12345678,
			-19.23456789, 20.34567891 };

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferUint64[row], bufferBool[row], bufferDouble[row]);
		}

		const auto check{ [&] [[nodiscard]] () {
			RETURN_IF_FALSE(basicTableCheck(table, 40, { 111111, 222222, 333333 }));

			const size_t expectedBufferSize{ (sizeof(uint64_t) + sizeof(bool) + sizeof(double)) * 40 + sizeof(size_t) };
			RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size for 40 rows"));

			for (size_t row{ 0 }; row < 40; ++row) {
				RETURN_IF_FALSE(
					(checkTableElement.operator()<uint64_t, 40, uint64_t, bool, double>(table, 0, row, bufferUint642)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<bool, 40, uint64_t, bool, double>(table, 1, row, bufferBool2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<double, 40, uint64_t, bool, double>(table, 2, row, bufferDouble2)));
			}

			return true;
		} };

		RETURN_IF_FALSE(check());

		std::reverse(bufferUint642.begin(), bufferUint642.end());
		std::reverse(bufferBool2.begin(), bufferBool2.end());
		std::reverse(bufferDouble2.begin(), bufferDouble2.end());

		for (size_t row{ 0 }; row < 40; ++row) {
			table.UpdateCell(0, row, bufferUint642[row]);
			table.UpdateCell(1, row, bufferBool2[row]);
			table.UpdateCell(2, row, bufferDouble2[row]);
		}

		RETURN_IF_FALSE(checkTableData(table));

		RETURN_IF_FALSE(check());

		RETURN_IF_FALSE(checkCopy(table));
		table.Clear();
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 222222, 333333 }));

		RETURN_IF_FALSE(checkEmptyPrints());
	}

	{
#define __TableTypes std::optional<uint64_t>, Timer, std::string, std::string, Timer::Duration, double, std::string

		MSAPI::Table<__TableTypes> table{ 111111, 222222, 333333, 444444, 555555, 666666, 777777 };
		RETURN_IF_FALSE(checkCopy(table));
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 222222, 333333, 444444, 555555, 666666, 777777 }));

		RETURN_IF_FALSE(checkTableData(table));

		const auto checkEmptyPrints{ [&table, &t] [[nodiscard]] () {
			const std::string expectedString1{
				"Table:\n{\n\tBuffer size: 8\n\tColumns:\n\t{\n\t\t[0] 111111 OptionalUint64\n\t\t[1] 222222 "
				"Timer\n\t\t[2] 333333 String\n\t\t[3] 444444 String\n\t\t[4] 555555 Duration\n\t\t[5] 666666 "
				"Double\n\t\t[6] 777777 String\n\t}\n}"
			};
			RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

			const std::string expectedString2{
				"{\"Buffer "
				"size\":8,\"Columns\":[{\"id\":111111,\"type\":\"OptionalUint64\"},{\"id\":222222,\"type\":"
				"\"Timer\"},{\"id\":333333,\"type\":\"String\"},{\"id\":444444,"
				"\"type\":\"String\"},{\"id\":555555,\"type\":\"Duration\"},{\"id\":666666,\"type\":\"Double\"},{"
				"\"id\":777777,\"type\":\"String\"}],\"Rows\":[]}"
			};
			RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

			return true;
		} };

		RETURN_IF_FALSE(checkEmptyPrints());

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
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here",
			"Some random string --------8 here", "Some random string here ---------9", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here", "Some random string --------8 here",
			"Some random string here ---------9" };
		const size_t sizeofBufferString{ [&bufferString]() {
			size_t size{ 0 };
			for (const auto& item : bufferString) {
				size += item.size() + sizeof(size_t);
			}
			return size;
		}() };
		std::array<Timer::Duration, 20> bufferTimerDuration{ Timer::Duration{ 7929342421 },
			Timer::Duration::CreateMicroseconds(348238), Timer::Duration::CreateMicroseconds(348225223423438),
			Timer::Duration::CreateMicroseconds(343248238), Timer::Duration::CreateMilliseconds(234234),
			Timer::Duration::CreateSeconds(28434), Timer::Duration::CreateMinutes(23453),
			Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441), Timer::Duration::CreateMinutes(0),
			Timer::Duration::CreateNanoseconds(7929342421), Timer::Duration::CreateMicroseconds(348238),
			Timer::Duration::CreateMicroseconds(348225223423438), Timer::Duration::CreateMicroseconds(343248238),
			Timer::Duration::CreateMilliseconds(234234), Timer::Duration::CreateSeconds(28434),
			Timer::Duration::CreateMinutes(23453), Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441),
			Timer::Duration::CreateMinutes(0) };
		std::array<double, 20> bufferLongDouble{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235,
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789, -0.84291, 0, 23492.43583,
			-0.0000234234, 4583045.00235, -2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789 };

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferOptionalUint64[row], bufferTimer[row], bufferString[row], bufferString[row],
				bufferTimerDuration[row], bufferLongDouble[row], bufferString[row]);
		}

		RETURN_IF_FALSE(basicTableCheck(table, 20, { 111111, 222222, 333333, 444444, 555555, 666666, 777777 }));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE(
				(checkTableElement.operator()<std::optional<uint64_t>, 20, std::optional<uint64_t>, Timer, std::string,
					std::string, Timer::Duration, double, std::string>(table, 0, row, bufferOptionalUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<Timer, 20, __TableTypes>(table, 1, row, bufferTimer)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 2, row, bufferString)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 3, row, bufferString)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<Timer::Duration, 20, __TableTypes>(table, 4, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<double, 20, __TableTypes>(table, 5, row, bufferLongDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 6, row, bufferString)));
		}

		const std::string_view expectedString1{
			"Table:\n{\n\tBuffer size: 2870\n\tColumns:\n\t{\n\t\t[0] 111111 OptionalUint64\n\t\t[1] 222222 "
			"Timer\n\t\t[2] 333333 String\n\t\t[3] 444444 String\n\t\t[4] 555555 Duration\n\t\t[5] 666666 "
			"Double\n\t\t[6] 777777 String\n\t}\n\tRows:\n\t{\n\t\t[0, 0] 287918237 [1| 1999-03-04 "
			"12:44:23.000746384 [2| 0 Some random string here [3| 0 Some random string here [4| 7929342421 "
			"nanoseconds [5| -0.84291000000000005 [6| 0 Some random string here\n\t\t[0, 1]  [1| 2023-11-27 "
			"00:00:00.000000000 [2| Some -1 random string here [3| Some -1 random string here [4| 348238000 "
			"nanoseconds [5| 0.00000000000000000 [6| Some -1 random string here\n\t\t[0, 2] 9098345 [1| 2024-05-12 "
			"00:00:00.000000000 [2| Some random --2 string here [3| Some random --2 string here [4| "
			"348225223423438000 nanoseconds [5| 23492.43582999999853200 [6| Some random --2 string here\n\t\t[0, "
			"3]  [1| 1970-01-01 00:00:00.000000000 [2| Some random string ---3 here [3| Some random string ---3 "
			"here [4| 343248238000 nanoseconds [5| -0.00002342340000000 [6| Some random string ---3 here\n\t\t[0, "
			"4] 209348023 [1| 2024-05-12 00:00:00.000000000 [2| Some random string here ----4 [3| Some random "
			"string here ----4 [4| 234234000000 nanoseconds [5| 4583045.00234999973326921 [6| Some random string "
			"here ----4\n\t\t[0, 5] 99938498234 [1| 1970-01-01 00:00:00.000000000 [2| -----5 Some random string "
			"here [3| -----5 Some random string here [4| 28434000000000 nanoseconds [5| -2342234.23482001014053822 "
			"[6| -----5 Some random string here\n\t\t[0, 6] 0 [1| 2024-05-12 00:00:00.000000000 [2| Some ------6 "
			"random string here [3| Some ------6 random string here [4| 1407180000000000 nanoseconds [5| "
			"-7.89123456000000001 [6| Some ------6 random string here\n\t\t[0, 7]  [1| 2024-05-12 "
			"00:00:00.000000000 [2| Some random -------7 string here [3| Some random -------7 string here [4| "
			"950400000000000 nanoseconds [5| 8.91234567000000055 [6| Some random -------7 string here\n\t\t[0, 8] "
			"238472934729834 [1| 2024-05-12 00:00:00.000000000 [2| Some random string --------8 here [3| Some "
			"random string --------8 here [4| 556502400000000000 nanoseconds [5| -9.12345677999999971 [6| Some "
			"random string --------8 here\n\t\t[0, 9] 27346277 [1| 2024-05-12 00:00:00.000000000 [2| Some random "
			"string here ---------9 [3| Some random string here ---------9 [4| 0 nanoseconds [5| "
			"10.23456788999999922 [6| Some random string here ---------9\n\t\t[0, 10] 287918237 [1| 1999-03-04 "
			"12:44:23.000746384 [2| 0 Some random string here [3| 0 Some random string here [4| 7929342421 "
			"nanoseconds [5| -0.84291000000000005 [6| 0 Some random string here\n\t\t[0, 11]  [1| 2023-11-27 "
			"00:00:00.000000000 [2| Some -1 random string here [3| Some -1 random string here [4| 348238000 "
			"nanoseconds [5| 0.00000000000000000 [6| Some -1 random string here\n\t\t[0, 12] 9098345 [1| "
			"2024-05-12 00:00:00.000000000 [2| Some random --2 string here [3| Some random --2 string here [4| "
			"348225223423438000 nanoseconds [5| 23492.43582999999853200 [6| Some random --2 string here\n\t\t[0, "
			"13]  [1| 1970-01-01 00:00:00.000000000 [2| Some random string ---3 here [3| Some random string ---3 "
			"here [4| 343248238000 nanoseconds [5| -0.00002342340000000 [6| Some random string ---3 here\n\t\t[0, "
			"14] 209348023 [1| 2024-05-12 00:00:00.000000000 [2| Some random string here ----4 [3| Some random "
			"string here ----4 [4| 234234000000 nanoseconds [5| 4583045.00234999973326921 [6| Some random string "
			"here ----4\n\t\t[0, 15] 99938498234 [1| 1970-01-01 00:00:00.000000000 [2| -----5 Some random string "
			"here [3| -----5 Some random string here [4| 28434000000000 nanoseconds [5| -2342234.23482001014053822 "
			"[6| -----5 Some random string here\n\t\t[0, 16] 0 [1| 2024-05-12 00:00:00.000000000 [2| Some ------6 "
			"random string here [3| Some ------6 random string here [4| 1407180000000000 nanoseconds [5| "
			"-7.89123456000000001 [6| Some ------6 random string here\n\t\t[0, 17]  [1| 2024-05-12 "
			"00:00:00.000000000 [2| Some random -------7 string here [3| Some random -------7 string here [4| "
			"950400000000000 nanoseconds [5| 8.91234567000000055 [6| Some random -------7 string here\n\t\t[0, 18] "
			"238472934729834 [1| 2024-05-12 00:00:00.000000000 [2| Some random string --------8 here [3| Some "
			"random string --------8 here [4| 556502400000000000 nanoseconds [5| -9.12345677999999971 [6| Some "
			"random string --------8 here\n\t\t[0, 19] 27346277 [1| 2024-05-12 00:00:00.000000000 [2| Some random "
			"string here ---------9 [3| Some random string here ---------9 [4| 0 nanoseconds [5| "
			"10.23456788999999922 [6| Some random string here ---------9\n\t}\n}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

		const std::string_view expectedString2{
			"{\"Buffer "
			"size\":2870,\"Columns\":[{\"id\":111111,\"type\":\"OptionalUint64\"},{\"id\":222222,\"type\":"
			"\"Timer\"},{\"id\":333333,\"type\":\"String\"},{\"id\":444444,\"type\":\"String\"},{\"id\":555555,"
			"\"type\":\"Duration\"},{\"id\":666666,\"type\":\"Double\"},{\"id\":777777,\"type\":\"String\"}],"
			"\"Rows\":[[287918237,\"1999-03-04 12:44:23.000746384\",\"0 Some random string here\",\"0 Some random "
			"string here\",\"7929342421 nanoseconds\",-0.84291000000000005,\"0 Some random string "
			"here\"],[null,\"2023-11-27 00:00:00.000000000\",\"Some -1 random string here\",\"Some -1 random "
			"string here\",\"348238000 nanoseconds\",0.00000000000000000,\"Some -1 random string "
			"here\"],[9098345,\"2024-05-12 00:00:00.000000000\",\"Some random --2 string here\",\"Some random --2 "
			"string here\",\"348225223423438000 nanoseconds\",23492.43582999999853200,\"Some random --2 string "
			"here\"],[null,\"1970-01-01 00:00:00.000000000\",\"Some random string ---3 here\",\"Some random string "
			"---3 here\",\"343248238000 nanoseconds\",-0.00002342340000000,\"Some random string ---3 "
			"here\"],[209348023,\"2024-05-12 00:00:00.000000000\",\"Some random string here ----4\",\"Some random "
			"string here ----4\",\"234234000000 nanoseconds\",4583045.00234999973326921,\"Some random string here "
			"----4\"],[99938498234,\"1970-01-01 00:00:00.000000000\",\"-----5 Some random string here\",\"-----5 "
			"Some random string here\",\"28434000000000 nanoseconds\",-2342234.23482001014053822,\"-----5 Some "
			"random string here\"],[0,\"2024-05-12 00:00:00.000000000\",\"Some ------6 random string here\",\"Some "
			"------6 random string here\",\"1407180000000000 nanoseconds\",-7.89123456000000001,\"Some ------6 "
			"random string here\"],[null,\"2024-05-12 00:00:00.000000000\",\"Some random -------7 string "
			"here\",\"Some random -------7 string here\",\"950400000000000 "
			"nanoseconds\",8.91234567000000055,\"Some random -------7 string here\"],[238472934729834,\"2024-05-12 "
			"00:00:00.000000000\",\"Some random string --------8 here\",\"Some random string --------8 "
			"here\",\"556502400000000000 nanoseconds\",-9.12345677999999971,\"Some random string --------8 "
			"here\"],[27346277,\"2024-05-12 00:00:00.000000000\",\"Some random string here ---------9\",\"Some "
			"random string here ---------9\",\"0 nanoseconds\",10.23456788999999922,\"Some random string here "
			"---------9\"],[287918237,\"1999-03-04 12:44:23.000746384\",\"0 Some random string here\",\"0 Some "
			"random string here\",\"7929342421 nanoseconds\",-0.84291000000000005,\"0 Some random string "
			"here\"],[null,\"2023-11-27 00:00:00.000000000\",\"Some -1 random string here\",\"Some -1 random "
			"string here\",\"348238000 nanoseconds\",0.00000000000000000,\"Some -1 random string "
			"here\"],[9098345,\"2024-05-12 00:00:00.000000000\",\"Some random --2 string here\",\"Some random --2 "
			"string here\",\"348225223423438000 nanoseconds\",23492.43582999999853200,\"Some random --2 string "
			"here\"],[null,\"1970-01-01 00:00:00.000000000\",\"Some random string ---3 here\",\"Some random string "
			"---3 here\",\"343248238000 nanoseconds\",-0.00002342340000000,\"Some random string ---3 "
			"here\"],[209348023,\"2024-05-12 00:00:00.000000000\",\"Some random string here ----4\",\"Some random "
			"string here ----4\",\"234234000000 nanoseconds\",4583045.00234999973326921,\"Some random string here "
			"----4\"],[99938498234,\"1970-01-01 00:00:00.000000000\",\"-----5 Some random string here\",\"-----5 "
			"Some random string here\",\"28434000000000 nanoseconds\",-2342234.23482001014053822,\"-----5 Some "
			"random string here\"],[0,\"2024-05-12 00:00:00.000000000\",\"Some ------6 random string here\",\"Some "
			"------6 random string here\",\"1407180000000000 nanoseconds\",-7.89123456000000001,\"Some ------6 "
			"random string here\"],[null,\"2024-05-12 00:00:00.000000000\",\"Some random -------7 string "
			"here\",\"Some random -------7 string here\",\"950400000000000 "
			"nanoseconds\",8.91234567000000055,\"Some random -------7 string here\"],[238472934729834,\"2024-05-12 "
			"00:00:00.000000000\",\"Some random string --------8 here\",\"Some random string --------8 "
			"here\",\"556502400000000000 nanoseconds\",-9.12345677999999971,\"Some random string --------8 "
			"here\"],[27346277,\"2024-05-12 00:00:00.000000000\",\"Some random string here ---------9\",\"Some "
			"random string here ---------9\",\"0 nanoseconds\",10.23456788999999922,\"Some random string here "
			"---------9\"]]}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

		MSAPI::AutoClearPtr<void> buffer{ table.Encode() };
		const auto bufferSize{ table.GetBufferSize() };

		const size_t expectedBufferSize{ (sizeof(bool) + sizeof(Timer) + sizeof(Timer::Duration) + sizeof(double)) * 20
			+ sizeof(size_t) + sizeof(uint64_t) * 14 + sizeofBufferString * 3 };
		RETURN_IF_FALSE(t.Assert(bufferSize, expectedBufferSize, "Table buffer size for 20 rows"));

		table.Clear();

		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 222222, 333333, 444444, 555555, 666666, 777777 }));

		MSAPI::TableData tableData{ buffer.ptr };
		table.Copy(tableData);
		RETURN_IF_FALSE(basicTableCheck(table, 20, { 111111, 222222, 333333, 444444, 555555, 666666, 777777 }));

		RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size for 20 rows"));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint64_t>, 20, __TableTypes>(
				table, 0, row, bufferOptionalUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<Timer, 20, __TableTypes>(table, 1, row, bufferTimer)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 2, row, bufferString)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 3, row, bufferString)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<Timer::Duration, 20, __TableTypes>(table, 4, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<double, 20, __TableTypes>(table, 5, row, bufferLongDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 6, row, bufferString)));
		}

		std::array<std::optional<uint64_t>, 40> bufferOptionalUint642{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834,
			27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834, 27346277 };
		std::array<Timer, 40> bufferTimer2{ Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			Timer::Create(2023, 11, 27, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			Timer::Create(2023, 11, 27, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			Timer::Create(2023, 11, 27, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			Timer::Create(2023, 11, 27, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0), Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			Timer::Create(2024, 5, 12, 0, 0, 0, 0) };
		std::array<std::string, 40> bufferString2{ "0 Some random string here", "Some -1 random string here",
			"Some random --2 string here", "Some random string ---3 here", "Some random string here ----4",
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here",
			"Some random string --------8 here", "Some random string here ---------9", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here", "Some random string --------8 here",
			"Some random string here ---------9", "0 Some random string here", "Some -1 random string here",
			"Some random --2 string here", "Some random string ---3 here", "Some random string here ----4",
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here",
			"Some random string --------8 here", "Some random string here ---------9", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here", "Some random string --------8 here",
			"Some random string here ---------9" };
		std::array<Timer::Duration, 40> bufferTimerDuration2{ Timer::Duration{ 7929342421 },
			Timer::Duration::CreateMicroseconds(348238), Timer::Duration::CreateMicroseconds(348225223423438),
			Timer::Duration::CreateMicroseconds(343248238), Timer::Duration::CreateMilliseconds(234234),
			Timer::Duration::CreateSeconds(28434), Timer::Duration::CreateMinutes(23453),
			Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441), Timer::Duration::CreateMinutes(0),
			Timer::Duration::CreateNanoseconds(7929342421), Timer::Duration::CreateMicroseconds(348238),
			Timer::Duration::CreateMicroseconds(348225223423438), Timer::Duration::CreateMicroseconds(343248238),
			Timer::Duration::CreateMilliseconds(234234), Timer::Duration::CreateSeconds(28434),
			Timer::Duration::CreateMinutes(23453), Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441),
			Timer::Duration::CreateMinutes(0), Timer::Duration{ 7929342421 },
			Timer::Duration::CreateMicroseconds(348238), Timer::Duration::CreateMicroseconds(348225223423438),
			Timer::Duration::CreateMicroseconds(343248238), Timer::Duration::CreateMilliseconds(234234),
			Timer::Duration::CreateSeconds(28434), Timer::Duration::CreateMinutes(23453),
			Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441), Timer::Duration::CreateMinutes(0),
			Timer::Duration::CreateNanoseconds(7929342421), Timer::Duration::CreateMicroseconds(348238),
			Timer::Duration::CreateMicroseconds(348225223423438), Timer::Duration::CreateMicroseconds(343248238),
			Timer::Duration::CreateMilliseconds(234234), Timer::Duration::CreateSeconds(28434),
			Timer::Duration::CreateMinutes(23453), Timer::Duration::CreateHours(264), Timer::Duration::CreateDays(6441),
			Timer::Duration::CreateMinutes(0) };
		std::array<double, 40> bufferLongDouble2{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235,
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789, -0.84291, 0, 23492.43583,
			-0.0000234234, 4583045.00235, -2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789,
			-0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001, -7.89123456, 8.91234567,
			-9.12345678, 10.23456789, -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001,
			-7.89123456, 8.91234567, -9.12345678, 10.23456789 };

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferOptionalUint642[row], bufferTimer[row], bufferString[row], bufferString[row],
				bufferTimerDuration[row], bufferLongDouble[row], bufferString[row]);
		}

		const auto check{ [&] [[nodiscard]] () {
			RETURN_IF_FALSE(basicTableCheck(table, 40, { 111111, 222222, 333333, 444444, 555555, 666666, 777777 }));

			const size_t expectedBufferSize{ (sizeof(bool) + sizeof(Timer) + sizeof(Timer::Duration) + sizeof(double))
					* 40
				+ sizeof(size_t) + sizeof(uint64_t) * 28 + sizeofBufferString * 6 };
			RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size for 40 rows"));

			for (size_t row{ 0 }; row < 40; ++row) {
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint64_t>, 40, __TableTypes>(
					table, 0, row, bufferOptionalUint642)));
				RETURN_IF_FALSE((checkTableElement.operator()<Timer, 40, __TableTypes>(table, 1, row, bufferTimer2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<std::string, 40, __TableTypes>(table, 2, row, bufferString2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<std::string, 40, __TableTypes>(table, 3, row, bufferString2)));
				RETURN_IF_FALSE((checkTableElement.operator()<Timer::Duration, 40, __TableTypes>(
					table, 4, row, bufferTimerDuration2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<double, 40, __TableTypes>(table, 5, row, bufferLongDouble2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<std::string, 40, __TableTypes>(table, 6, row, bufferString2)));
#undef __TableTypes
			}

			return true;
		} };

		RETURN_IF_FALSE(check());

		std::reverse(bufferOptionalUint642.begin(), bufferOptionalUint642.end());
		std::reverse(bufferTimer2.begin(), bufferTimer2.end());
		std::reverse(bufferString2.begin(), bufferString2.end());
		std::reverse(bufferTimerDuration2.begin(), bufferTimerDuration2.end());
		std::reverse(bufferLongDouble2.begin(), bufferLongDouble2.end());

		for (size_t row{ 0 }; row < 40; ++row) {
			table.UpdateCell(0, row, bufferOptionalUint642[row]);
			table.UpdateCell(1, row, bufferTimer2[row]);
			table.UpdateCell(2, row, bufferString2[row]);
			table.UpdateCell(3, row, bufferString2[row]);
			table.UpdateCell(4, row, bufferTimerDuration2[row]);
			table.UpdateCell(5, row, bufferLongDouble2[row]);
			table.UpdateCell(6, row, bufferString2[row]);
		}

		RETURN_IF_FALSE(checkTableData(table));

		RETURN_IF_FALSE(check());

		RETURN_IF_FALSE(checkCopy(table));
		table.Clear();
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 111111, 222222, 333333, 444444, 555555, 666666, 777777 }));

		RETURN_IF_FALSE(checkEmptyPrints());
	}

	{
#define __TableTypes                                                                                                   \
	std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::Timer::Duration, MSAPI::Timer::Duration,                 \
		standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, bool, std::string,                     \
		std::optional<double>, std::optional<double>

		MSAPI::Table<__TableTypes> table{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010,
			1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919,
			2020202020, 2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
			2929292929, 3030303030, 3131313131, 3232323232, 3333333333 };
		RETURN_IF_FALSE(checkCopy(table));
		RETURN_IF_FALSE(basicTableCheck(table, 0,
			{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111, 1212121212,
				1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
				2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
				2929292929, 3030303030, 3131313131, 3232323232, 3333333333 }));

		RETURN_IF_FALSE(checkTableData(table));

		const auto checkEmptyPrints{ [&table, &t] [[nodiscard]] () {
			const std::string expectedString1{
				"Table:\n{\n\tBuffer size: 8\n\tColumns:\n\t{\n\t\t[0] 11111 String\n\t\t[1] 22222 Timer\n\t\t[2] "
				"33333 Duration\n\t\t[3] 44444 Duration\n\t\t[4] 55555 Duration\n\t\t[5] 66666 Int8\n\t\t[6] 77777 "
				"Int16\n\t\t[7] 88888 Int32\n\t\t[8] 99999 Int64\n\t\t[9] 1010101010 Uint8\n\t\t[10] "
				"1111111111 Uint16\n\t\t[11] 1212121212 Uint32\n\t\t[12] 1313131313 Uint64\n\t\t[13] 1414141414 "
				"Double\n\t\t[14] 1515151515 Float\n\t\t[15] 1616161616 Bool\n\t\t[16] 1717171717 "
				"OptionalInt8\n\t\t[17] 1818181818 OptionalInt16\n\t\t[18] 1919191919 OptionalInt32\n\t\t[19] "
				"2020202020 OptionalInt64\n\t\t[20] 2121212121 OptionalUint8\n\t\t[21] 2222222222 "
				"OptionalUint16\n\t\t[22] 2323232323 OptionalUint32\n\t\t[23] 2424242424 OptionalUint64\n\t\t[24] "
				"2525252525 OptionalDouble\n\t\t[25] 2626262626 OptionalFloat\n\t\t[26] 2727272727 "
				"String\n\t\t[27] 2828282828 Timer\n\t\t[28] 2929292929 Duration\n\t\t[29] 3030303030 "
				"Bool\n\t\t[30] 3131313131 String\n\t\t[31] 3232323232 OptionalDouble\n\t\t[32] 3333333333 "
				"OptionalDouble\n\t}\n}"
			};
			RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString1, "Table to string"));

			const std::string expectedString2{
				"{\"Buffer "
				"size\":8,\"Columns\":[{\"id\":11111,\"type\":\"String\"},{\"id\":22222,\"type\":\"Timer\"},{\"id\":"
				"33333,\"type\":\"Duration\"},{\"id\":44444,\"type\":\"Duration\"},{\"id\":55555,\"type\":\"Duration\"}"
				",{\"id\":66666,\"type\":\"Int8\"},{\"id\":77777,\"type\":\"Int16\"},{\"id\":88888,\"type\":\"Int32\"},"
				"{\"id\":99999,\"type\":\"Int64\"},{\"id\":1010101010,\"type\":\"Uint8\"},{\"id\":1111111111,\"type\":"
				"\"Uint16\"},{\"id\":1212121212,\"type\":\"Uint32\"},{\"id\":1313131313,\"type\":\"Uint64\"},{\"id\":"
				"1414141414,\"type\":\"Double\"},{\"id\":1515151515,\"type\":\"Float\"},{\"id\":1616161616,\"type\":"
				"\"Bool\"},{\"id\":1717171717,\"type\":\"OptionalInt8\"},{\"id\":1818181818,\"type\":\"OptionalInt16\"}"
				",{\"id\":1919191919,\"type\":\"OptionalInt32\"},{\"id\":2020202020,\"type\":\"OptionalInt64\"},{"
				"\"id\":2121212121,\"type\":\"OptionalUint8\"},{\"id\":2222222222,\"type\":\"OptionalUint16\"},{\"id\":"
				"2323232323,\"type\":\"OptionalUint32\"},{\"id\":2424242424,\"type\":\"OptionalUint64\"},{\"id\":"
				"2525252525,\"type\":\"OptionalDouble\"},{\"id\":2626262626,\"type\":\"OptionalFloat\"},{\"id\":"
				"2727272727,\"type\":\"String\"},{\"id\":2828282828,\"type\":\"Timer\"},{\"id\":2929292929,\"type\":"
				"\"Duration\"},{\"id\":3030303030,\"type\":\"Bool\"},{\"id\":3131313131,\"type\":\"String\"},{\"id\":"
				"3232323232,\"type\":\"OptionalDouble\"},{\"id\":3333333333,\"type\":\"OptionalDouble\"}],\"Rows\":[]}"
			};
			RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

			return true;
		} };

		RETURN_IF_FALSE(checkEmptyPrints());

		std::array<int8_t, 20> bufferInt8{ -128, -64, -32, -16, -8, -4, -2, -1, 0, 1, 2, 4, 8, 16, 32, 64, 100, 112,
			120, 127 };
		std::array<int16_t, 20> bufferInt16{ -100, 200, 30, 0, -41, 52, 63, 74, 85, 96, 107, 118, -129, 140, 151, -162,
			173, 184, 195, -206 };
		std::array<int32_t, 20> bufferInt32{ 11111, 22222, -33333, 44444, 55555, 66666, -77777, 88888, 99999,
			-1010101010, 1111111111, 1212121212, 1313131313, -1414141414, 1515151515, 1616161616, 1717171717,
			-1818181818, -1919191919, 2020202020 };
		std::array<int64_t, 20> bufferInt64{ 11111, 22222, -33333, 44444, 55555, 66666, -77777, 88888, 99999,
			-1010101010, 1111111111, 1212121212, 1313131313, -1414141414, 1515151515, 1616161616, 1717171717,
			-1818181818, -1919191919, 2020202020 };
		std::array<uint8_t, 20> bufferUint8{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 100, 127, 128, 200, 220, 254,
			255 };
		std::array<uint16_t, 20> bufferUint16{ 100, 200, 30, 0, 41, 52, 63, 74, 85, 96, 107, 118, 129, 140, 151, 162,
			173, 184, 195, 206 };
		std::array<uint32_t, 20> bufferUint32{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999,
			1010101010, 1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818,
			1919191919, 2020202020 };
		std::array<uint64_t, 20> bufferUint64{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999,
			1010101010, 1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818,
			1919191919, 2020202020 };
		std::array<double, 20> bufferDouble{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001,
			-7.89123456, 8.91234567, -9.12345678, 10.23456789, -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235,
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789 };
		std::array<float, 20> bufferFloat{ -0.84291f, 0.f, 23492.43583f, -0.0000234234f, 4583045.00235f,
			-2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, 10.23456789f, -0.84291f, 0.f, 23492.43583f,
			-0.0000234234f, 4583045.00235f, -2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, 10.23456789f };
		std::array<bool, 20> bufferBool{ true, true, false, true, false, false, false, true, true, false, true, true,
			true, false, true, false, true, true, true, false };
		std::array<std::optional<int8_t>, 20> bufferOptionalInt8{ -128, -64, {}, -16, -8, -4, -2, -1, {}, {}, 2, 4, 8,
			16, 32, 64, 100, 112, 120, {} };
		std::array<std::optional<int16_t>, 20> bufferOptionalInt16{ std::optional<int16_t>{}, {}, {}, {}, {}, {}, {},
			{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} };
		std::array<std::optional<int32_t>, 20> bufferOptionalInt32{ 287918237, {}, 9098345, {}, 209348023, 99938498234,
			0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<std::optional<int64_t>, 20> bufferOptionalInt64{ 287918237, {}, 9098345, {}, 209348023, 99938498234,
			0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<std::optional<uint8_t>, 20> bufferOptionalUint8{ 0, 1, {}, 3, 4, 5, 6, 7, {}, {}, 10, 11, 12, 100,
			127, 128, 200, 220, 254, {} };
		std::array<std::optional<uint16_t>, 20> bufferOptionalUint16{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<std::optional<uint32_t>, 20> bufferOptionalUint32{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<std::optional<uint64_t>, 20> bufferOptionalUint64{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277 };
		std::array<std::optional<double>, 20> bufferOptionalDouble{ -0.84291, 0, 23492.43583, -0.0000234234, {},
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, {}, -0.84291, 0, 23492.43583, -0.0000234234,
			4583045.00235, -2342234.23482001, {}, 8.91234567, -9.12345678, {} };
		std::array<std::optional<float>, 20> bufferOptionalFloat{ -0.84291f, 0.f, 23492.43583f, -0.0000234234f, {},
			-2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, {}, -0.84291f, 0.f, 23492.43583f,
			-0.0000234234f, 4583045.00235f, -2342234.23482001f, {}, 8.91234567f, -9.12345678f, {} };
		std::array<std::string, 20> bufferString{ "", "", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here", "Some random string --------8 here", "", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here", "",
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here",
			"Some random string --------8 here", "Some random string here---------9" };
		const size_t sizeofBufferString{ [&bufferString]() {
			size_t size{ 0 };
			for (const auto& item : bufferString) {
				size += item.size() + sizeof(size_t);
			}
			return size;
		}() };
		std::array<MSAPI::Timer, 20> bufferTimer{ MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0) };
		std::array<Timer::Duration, 20> bufferTimerDuration{ MSAPI::Timer::Duration::CreateNanoseconds(7929342421),
			MSAPI::Timer::Duration::CreateMicroseconds(348238),
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
			MSAPI::Timer::Duration::CreateMinutes(0) };

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferString[row], bufferTimer[row], bufferTimerDuration[row], bufferTimerDuration[row],
				bufferTimerDuration[row], bufferInt8[row], bufferInt16[row], bufferInt32[row], bufferInt64[row],
				bufferUint8[row], bufferUint16[row], bufferUint32[row], bufferUint64[row], bufferDouble[row],
				bufferFloat[row], bufferBool[row], bufferOptionalInt8[row], bufferOptionalInt16[row],
				bufferOptionalInt32[row], bufferOptionalInt64[row], bufferOptionalUint8[row], bufferOptionalUint16[row],
				bufferOptionalUint32[row], bufferOptionalUint64[row], bufferOptionalDouble[row],
				bufferOptionalFloat[row], bufferString[row], bufferTimer[row], bufferTimerDuration[row],
				bufferBool[row], bufferString[row], bufferOptionalDouble[row], bufferOptionalDouble[row]);
		}

		RETURN_IF_FALSE(basicTableCheck(table, 20,
			{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111, 1212121212,
				1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
				2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
				2929292929, 3030303030, 3131313131, 3232323232, 3333333333 }));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 0, row, bufferString)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer, 20, __TableTypes>(table, 1, row, bufferTimer)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 2, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 3, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 4, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<int8_t, 20, __TableTypes>(table, 5, row, bufferInt8)));
			RETURN_IF_FALSE((checkTableElement.operator()<int16_t, 20, __TableTypes>(table, 6, row, bufferInt16)));
			RETURN_IF_FALSE((checkTableElement.operator()<int32_t, 20, __TableTypes>(table, 7, row, bufferInt32)));
			RETURN_IF_FALSE((checkTableElement.operator()<int64_t, 20, __TableTypes>(table, 8, row, bufferInt64)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint8_t, 20, __TableTypes>(table, 9, row, bufferUint8)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint16_t, 20, __TableTypes>(table, 10, row, bufferUint16)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint32_t, 20, __TableTypes>(table, 11, row, bufferUint32)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint64_t, 20, __TableTypes>(table, 12, row, bufferUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<double, 20, __TableTypes>(table, 13, row, bufferDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<float, 20, __TableTypes>(table, 14, row, bufferFloat)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, __TableTypes>(table, 15, row, bufferBool)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int8_t>, 20, __TableTypes>(
				table, 16, row, bufferOptionalInt8)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int16_t>, 20, __TableTypes>(
				table, 17, row, bufferOptionalInt16)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int32_t>, 20, __TableTypes>(
				table, 18, row, bufferOptionalInt32)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int64_t>, 20, __TableTypes>(
				table, 19, row, bufferOptionalInt64)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint8_t>, 20, __TableTypes>(
				table, 20, row, bufferOptionalUint8)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint16_t>, 20, __TableTypes>(
				table, 21, row, bufferOptionalUint16)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint32_t>, 20, __TableTypes>(
				table, 22, row, bufferOptionalUint32)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint64_t>, 20, __TableTypes>(
				table, 23, row, bufferOptionalUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 20, __TableTypes>(
				table, 24, row, bufferOptionalDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<float>, 20, __TableTypes>(
				table, 25, row, bufferOptionalFloat)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<std::string, 20, __TableTypes>(table, 26, row, bufferString)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<MSAPI::Timer, 20, __TableTypes>(table, 27, row, bufferTimer)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 28, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, __TableTypes>(table, 29, row, bufferBool)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<std::string, 20, __TableTypes>(table, 30, row, bufferString)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 20, __TableTypes>(
				table, 31, row, bufferOptionalDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 20, __TableTypes>(
				table, 32, row, bufferOptionalDouble)));
		}

		const std::string_view expectedString{
			"Table:\n{\n\tBuffer size: 4837\n\tColumns:\n\t{\n\t\t[0] 11111 String\n\t\t[1] 22222 Timer\n\t\t[2] "
			"33333 Duration\n\t\t[3] 44444 Duration\n\t\t[4] 55555 Duration\n\t\t[5] 66666 Int8\n\t\t[6] 77777 "
			"Int16\n\t\t[7] 88888 Int32\n\t\t[8] 99999 Int64\n\t\t[9] 1010101010 Uint8\n\t\t[10] 1111111111 "
			"Uint16\n\t\t[11] 1212121212 Uint32\n\t\t[12] 1313131313 Uint64\n\t\t[13] 1414141414 Double\n\t\t[14] "
			"1515151515 Float\n\t\t[15] 1616161616 Bool\n\t\t[16] 1717171717 OptionalInt8\n\t\t[17] 1818181818 "
			"OptionalInt16\n\t\t[18] 1919191919 OptionalInt32\n\t\t[19] 2020202020 OptionalInt64\n\t\t[20] "
			"2121212121 OptionalUint8\n\t\t[21] 2222222222 OptionalUint16\n\t\t[22] 2323232323 "
			"OptionalUint32\n\t\t[23] 2424242424 OptionalUint64\n\t\t[24] 2525252525 OptionalDouble\n\t\t[25] "
			"2626262626 OptionalFloat\n\t\t[26] 2727272727 String\n\t\t[27] 2828282828 Timer\n\t\t[28] 2929292929 "
			"Duration\n\t\t[29] 3030303030 Bool\n\t\t[30] 3131313131 String\n\t\t[31] 3232323232 "
			"OptionalDouble\n\t\t[32] 3333333333 OptionalDouble\n\t}\n\tRows:\n\t{\n\t\t[0, 0]  [1| 1999-03-04 "
			"12:44:23.000746384 [2| 7929342421 nanoseconds [3| 7929342421 nanoseconds [4| 7929342421 nanoseconds "
			"[5| -128 [6| -100 [7| 11111 [8| 11111 [9| 0 [10| 100 [11| 11111 [12| 11111 [13| -0.84291000000000005 "
			"[14| -0.842909992 [15| true [16| -128 [17|  [18| 287918237 [19| 287918237 [20| 0 [21| 18589 [22| "
			"287918237 [23| 287918237 [24| -0.84291000000000005 [25| -0.842909992 [26|  [27| 1999-03-04 "
			"12:44:23.000746384 [28| 7929342421 nanoseconds [29| true [30|  [31| -0.84291000000000005 [32| "
			"-0.84291000000000005\n\t\t[0, 1]  [1| 2023-11-27 00:00:00.000000000 [2| 348238000 nanoseconds [3| "
			"348238000 nanoseconds [4| 348238000 nanoseconds [5| -64 [6| 200 [7| 22222 [8| 22222 [9| 1 [10| 200 "
			"[11| 22222 [12| 22222 [13| 0.00000000000000000 [14| 0.000000000 [15| true [16| -64 [17|  [18|  [19|  "
			"[20| 1 [21|  [22|  [23|  [24| 0.00000000000000000 [25| 0.000000000 [26|  [27| 2023-11-27 "
			"00:00:00.000000000 [28| 348238000 nanoseconds [29| true [30|  [31| 0.00000000000000000 [32| "
			"0.00000000000000000\n\t\t[0, 2] Some random --2 string here [1| 2024-05-12 00:00:00.000000000 [2| "
			"348225223423438000 nanoseconds [3| 348225223423438000 nanoseconds [4| 348225223423438000 nanoseconds "
			"[5| -32 [6| 30 [7| -33333 [8| -33333 [9| 2 [10| 30 [11| 33333 [12| 33333 [13| 23492.43582999999853200 "
			"[14| 23492.435546875 [15| false [16|  [17|  [18| 9098345 [19| 9098345 [20|  [21| 54377 [22| 9098345 "
			"[23| 9098345 [24| 23492.43582999999853200 [25| 23492.435546875 [26| Some random --2 string here [27| "
			"2024-05-12 00:00:00.000000000 [28| 348225223423438000 nanoseconds [29| false [30| Some random --2 "
			"string here [31| 23492.43582999999853200 [32| 23492.43582999999853200\n\t\t[0, 3] Some random string "
			"---3 here [1| 1970-01-01 00:00:00.000000000 [2| 343248238000 nanoseconds [3| 343248238000 nanoseconds "
			"[4| 343248238000 nanoseconds [5| -16 [6| 0 [7| 44444 [8| 44444 [9| 3 [10| 0 [11| 44444 [12| 44444 "
			"[13| -0.00002342340000000 [14| -0.000023423 [15| true [16| -16 [17|  [18|  [19|  [20| 3 [21|  [22|  "
			"[23|  [24| -0.00002342340000000 [25| -0.000023423 [26| Some random string ---3 here [27| 1970-01-01 "
			"00:00:00.000000000 [28| 343248238000 nanoseconds [29| true [30| Some random string ---3 here [31| "
			"-0.00002342340000000 [32| -0.00002342340000000\n\t\t[0, 4] Some random string here ----4 [1| "
			"2024-05-12 00:00:00.000000000 [2| 234234000000 nanoseconds [3| 234234000000 nanoseconds [4| "
			"234234000000 nanoseconds [5| -8 [6| -41 [7| 55555 [8| 55555 [9| 4 [10| 41 [11| 55555 [12| 55555 [13| "
			"4583045.00234999973326921 [14| 4583045.000000000 [15| false [16| -8 [17|  [18| 209348023 [19| "
			"209348023 [20| 4 [21| 26039 [22| 209348023 [23| 209348023 [24|  [25|  [26| Some random string here "
			"----4 [27| 2024-05-12 00:00:00.000000000 [28| 234234000000 nanoseconds [29| false [30| Some random "
			"string here ----4 [31|  [32| \n\t\t[0, 5] -----5 Some random string here [1| 1970-01-01 "
			"00:00:00.000000000 [2| 28434000000000 nanoseconds [3| 28434000000000 nanoseconds [4| 28434000000000 "
			"nanoseconds [5| -4 [6| 52 [7| 66666 [8| 66666 [9| 5 [10| 52 [11| 66666 [12| 66666 [13| "
			"-2342234.23482001014053822 [14| -2342234.250000000 [15| false [16| -4 [17|  [18| 1154250426 [19| "
			"99938498234 [20| 5 [21| 30394 [22| 1154250426 [23| 99938498234 [24| -2342234.23482001014053822 [25| "
			"-2342234.250000000 [26| -----5 Some random string here [27| 1970-01-01 00:00:00.000000000 [28| "
			"28434000000000 nanoseconds [29| false [30| -----5 Some random string here [31| "
			"-2342234.23482001014053822 [32| -2342234.23482001014053822\n\t\t[0, 6] Some ------6 random string "
			"here [1| 2024-05-12 00:00:00.000000000 [2| 1407180000000000 nanoseconds [3| 1407180000000000 "
			"nanoseconds [4| 1407180000000000 nanoseconds [5| -2 [6| 63 [7| -77777 [8| -77777 [9| 6 [10| 63 [11| "
			"77777 [12| 77777 [13| -7.89123456000000001 [14| -7.891234398 [15| false [16| -2 [17|  [18| 0 [19| 0 "
			"[20| 6 [21| 0 [22| 0 [23| 0 [24| -7.89123456000000001 [25| -7.891234398 [26| Some ------6 random "
			"string here [27| 2024-05-12 00:00:00.000000000 [28| 1407180000000000 nanoseconds [29| false [30| Some "
			"------6 random string here [31| -7.89123456000000001 [32| -7.89123456000000001\n\t\t[0, 7] Some "
			"random -------7 string here [1| 2024-05-12 00:00:00.000000000 [2| 950400000000000 nanoseconds [3| "
			"950400000000000 nanoseconds [4| 950400000000000 nanoseconds [5| -1 [6| 74 [7| 88888 [8| 88888 [9| 7 "
			"[10| 74 [11| 88888 [12| 88888 [13| 8.91234567000000055 [14| 8.912345886 [15| true [16| -1 [17|  [18|  "
			"[19|  [20| 7 [21|  [22|  [23|  [24| 8.91234567000000055 [25| 8.912345886 [26| Some random -------7 "
			"string here [27| 2024-05-12 00:00:00.000000000 [28| 950400000000000 nanoseconds [29| true [30| Some "
			"random -------7 string here [31| 8.91234567000000055 [32| 8.91234567000000055\n\t\t[0, 8] Some random "
			"string --------8 here [1| 2024-05-12 00:00:00.000000000 [2| 556502400000000000 nanoseconds [3| "
			"556502400000000000 nanoseconds [4| 556502400000000000 nanoseconds [5| 0 [6| 85 [7| 99999 [8| 99999 "
			"[9| 8 [10| 85 [11| 99999 [12| 99999 [13| -9.12345677999999971 [14| -9.123456955 [15| true [16|  [17|  "
			"[18| -829413270 [19| 238472934729834 [20|  [21| 10346 [22| 3465554026 [23| 238472934729834 [24| "
			"-9.12345677999999971 [25| -9.123456955 [26| Some random string --------8 here [27| 2024-05-12 "
			"00:00:00.000000000 [28| 556502400000000000 nanoseconds [29| true [30| Some random string --------8 "
			"here [31| -9.12345677999999971 [32| -9.12345677999999971\n\t\t[0, 9]  [1| 2024-05-12 "
			"00:00:00.000000000 [2| 0 nanoseconds [3| 0 nanoseconds [4| 0 nanoseconds [5| 1 [6| 96 [7| -1010101010 "
			"[8| -1010101010 [9| 9 [10| 96 [11| 1010101010 [12| 1010101010 [13| 10.23456788999999922 [14| "
			"10.234567642 [15| false [16|  [17|  [18| 27346277 [19| 27346277 [20|  [21| 17765 [22| 27346277 [23| "
			"27346277 [24|  [25|  [26|  [27| 2024-05-12 00:00:00.000000000 [28| 0 nanoseconds [29| false [30|  "
			"[31|  [32| \n\t\t[0, 10] 0 Some random string here [1| 1999-03-04 12:44:23.000746384 [2| 7929342421 "
			"nanoseconds [3| 7929342421 nanoseconds [4| 7929342421 nanoseconds [5| 2 [6| 107 [7| 1111111111 [8| "
			"1111111111 [9| 10 [10| 107 [11| 1111111111 [12| 1111111111 [13| -0.84291000000000005 [14| "
			"-0.842909992 [15| true [16| 2 [17|  [18| 287918237 [19| 287918237 [20| 10 [21| 18589 [22| 287918237 "
			"[23| 287918237 [24| -0.84291000000000005 [25| -0.842909992 [26| 0 Some random string here [27| "
			"1999-03-04 12:44:23.000746384 [28| 7929342421 nanoseconds [29| true [30| 0 Some random string here "
			"[31| -0.84291000000000005 [32| -0.84291000000000005\n\t\t[0, 11] Some -1 random string here [1| "
			"2023-11-27 00:00:00.000000000 [2| 348238000 nanoseconds [3| 348238000 nanoseconds [4| 348238000 "
			"nanoseconds [5| 4 [6| 118 [7| 1212121212 [8| 1212121212 [9| 11 [10| 118 [11| 1212121212 [12| "
			"1212121212 [13| 0.00000000000000000 [14| 0.000000000 [15| true [16| 4 [17|  [18|  [19|  [20| 11 [21|  "
			"[22|  [23|  [24| 0.00000000000000000 [25| 0.000000000 [26| Some -1 random string here [27| 2023-11-27 "
			"00:00:00.000000000 [28| 348238000 nanoseconds [29| true [30| Some -1 random string here [31| "
			"0.00000000000000000 [32| 0.00000000000000000\n\t\t[0, 12] Some random --2 string here [1| 2024-05-12 "
			"00:00:00.000000000 [2| 348225223423438000 nanoseconds [3| 348225223423438000 nanoseconds [4| "
			"348225223423438000 nanoseconds [5| 8 [6| -129 [7| 1313131313 [8| 1313131313 [9| 12 [10| 129 [11| "
			"1313131313 [12| 1313131313 [13| 23492.43582999999853200 [14| 23492.435546875 [15| true [16| 8 [17|  "
			"[18| 9098345 [19| 9098345 [20| 12 [21| 54377 [22| 9098345 [23| 9098345 [24| 23492.43582999999853200 "
			"[25| 23492.435546875 [26| Some random --2 string here [27| 2024-05-12 00:00:00.000000000 [28| "
			"348225223423438000 nanoseconds [29| true [30| Some random --2 string here [31| "
			"23492.43582999999853200 [32| 23492.43582999999853200\n\t\t[0, 13] Some random string ---3 here [1| "
			"1970-01-01 00:00:00.000000000 [2| 343248238000 nanoseconds [3| 343248238000 nanoseconds [4| "
			"343248238000 nanoseconds [5| 16 [6| 140 [7| -1414141414 [8| -1414141414 [9| 100 [10| 140 [11| "
			"1414141414 [12| 1414141414 [13| -0.00002342340000000 [14| -0.000023423 [15| false [16| 16 [17|  [18|  "
			"[19|  [20| 100 [21|  [22|  [23|  [24| -0.00002342340000000 [25| -0.000023423 [26| Some random string "
			"---3 here [27| 1970-01-01 00:00:00.000000000 [28| 343248238000 nanoseconds [29| false [30| Some "
			"random string ---3 here [31| -0.00002342340000000 [32| -0.00002342340000000\n\t\t[0, 14]  [1| "
			"2024-05-12 00:00:00.000000000 [2| 234234000000 nanoseconds [3| 234234000000 nanoseconds [4| "
			"234234000000 nanoseconds [5| 32 [6| 151 [7| 1515151515 [8| 1515151515 [9| 127 [10| 151 [11| "
			"1515151515 [12| 1515151515 [13| 4583045.00234999973326921 [14| 4583045.000000000 [15| true [16| 32 "
			"[17|  [18| 209348023 [19| 209348023 [20| 127 [21| 26039 [22| 209348023 [23| 209348023 [24| "
			"4583045.00234999973326921 [25| 4583045.000000000 [26|  [27| 2024-05-12 00:00:00.000000000 [28| "
			"234234000000 nanoseconds [29| true [30|  [31| 4583045.00234999973326921 [32| "
			"4583045.00234999973326921\n\t\t[0, 15] -----5 Some random string here [1| 1970-01-01 "
			"00:00:00.000000000 [2| 28434000000000 nanoseconds [3| 28434000000000 nanoseconds [4| 28434000000000 "
			"nanoseconds [5| 64 [6| -162 [7| 1616161616 [8| 1616161616 [9| 128 [10| 162 [11| 1616161616 [12| "
			"1616161616 [13| -2342234.23482001014053822 [14| -2342234.250000000 [15| false [16| 64 [17|  [18| "
			"1154250426 [19| 99938498234 [20| 128 [21| 30394 [22| 1154250426 [23| 99938498234 [24| "
			"-2342234.23482001014053822 [25| -2342234.250000000 [26| -----5 Some random string here [27| "
			"1970-01-01 00:00:00.000000000 [28| 28434000000000 nanoseconds [29| false [30| -----5 Some random "
			"string here [31| -2342234.23482001014053822 [32| -2342234.23482001014053822\n\t\t[0, 16] Some ------6 "
			"random string here [1| 2024-05-12 00:00:00.000000000 [2| 1407180000000000 nanoseconds [3| "
			"1407180000000000 nanoseconds [4| 1407180000000000 nanoseconds [5| 100 [6| 173 [7| 1717171717 [8| "
			"1717171717 [9| 200 [10| 173 [11| 1717171717 [12| 1717171717 [13| -7.89123456000000001 [14| "
			"-7.891234398 [15| true [16| 100 [17|  [18| 0 [19| 0 [20| 200 [21| 0 [22| 0 [23| 0 [24|  [25|  [26| "
			"Some ------6 random string here [27| 2024-05-12 00:00:00.000000000 [28| 1407180000000000 nanoseconds "
			"[29| true [30| Some ------6 random string here [31|  [32| \n\t\t[0, 17] Some random -------7 string "
			"here [1| 2024-05-12 00:00:00.000000000 [2| 950400000000000 nanoseconds [3| 950400000000000 "
			"nanoseconds [4| 950400000000000 nanoseconds [5| 112 [6| 184 [7| -1818181818 [8| -1818181818 [9| 220 "
			"[10| 184 [11| 1818181818 [12| 1818181818 [13| 8.91234567000000055 [14| 8.912345886 [15| true [16| 112 "
			"[17|  [18|  [19|  [20| 220 [21|  [22|  [23|  [24| 8.91234567000000055 [25| 8.912345886 [26| Some "
			"random -------7 string here [27| 2024-05-12 00:00:00.000000000 [28| 950400000000000 nanoseconds [29| "
			"true [30| Some random -------7 string here [31| 8.91234567000000055 [32| 8.91234567000000055\n\t\t[0, "
			"18] Some random string --------8 here [1| 2024-05-12 00:00:00.000000000 [2| 556502400000000000 "
			"nanoseconds [3| 556502400000000000 nanoseconds [4| 556502400000000000 nanoseconds [5| 120 [6| 195 [7| "
			"-1919191919 [8| -1919191919 [9| 254 [10| 195 [11| 1919191919 [12| 1919191919 [13| "
			"-9.12345677999999971 [14| -9.123456955 [15| true [16| 120 [17|  [18| -829413270 [19| 238472934729834 "
			"[20| 254 [21| 10346 [22| 3465554026 [23| 238472934729834 [24| -9.12345677999999971 [25| -9.123456955 "
			"[26| Some random string --------8 here [27| 2024-05-12 00:00:00.000000000 [28| 556502400000000000 "
			"nanoseconds [29| true [30| Some random string --------8 here [31| -9.12345677999999971 [32| "
			"-9.12345677999999971\n\t\t[0, 19] Some random string here---------9 [1| 2024-05-12 00:00:00.000000000 "
			"[2| 0 nanoseconds [3| 0 nanoseconds [4| 0 nanoseconds [5| 127 [6| -206 [7| 2020202020 [8| 2020202020 "
			"[9| 255 [10| 206 [11| 2020202020 [12| 2020202020 [13| 10.23456788999999922 [14| 10.234567642 [15| "
			"false [16|  [17|  [18| 27346277 [19| 27346277 [20|  [21| 17765 [22| 27346277 [23| 27346277 [24|  [25| "
			" [26| Some random string here---------9 [27| 2024-05-12 00:00:00.000000000 [28| 0 nanoseconds [29| "
			"false [30| Some random string here---------9 [31|  [32| \n\t}\n}"
		};
		RETURN_IF_FALSE(t.Assert(table.ToString(), expectedString, "Table to string"));

		const std::string_view expectedString2{
			"{\"Buffer "
			"size\":4837,\"Columns\":[{\"id\":11111,\"type\":\"String\"},{\"id\":22222,\"type\":\"Timer\"},{\"id\":"
			"33333,\"type\":\"Duration\"},{\"id\":44444,\"type\":\"Duration\"},{\"id\":55555,\"type\":\"Duration\"}"
			",{\"id\":66666,\"type\":\"Int8\"},{\"id\":77777,\"type\":\"Int16\"},{\"id\":88888,\"type\":\"Int32\"},"
			"{\"id\":99999,\"type\":\"Int64\"},{\"id\":1010101010,\"type\":\"Uint8\"},{\"id\":1111111111,\"type\":"
			"\"Uint16\"},{\"id\":1212121212,\"type\":\"Uint32\"},{\"id\":1313131313,\"type\":\"Uint64\"},{\"id\":"
			"1414141414,\"type\":\"Double\"},{\"id\":1515151515,\"type\":\"Float\"},{\"id\":1616161616,\"type\":"
			"\"Bool\"},{\"id\":1717171717,\"type\":\"OptionalInt8\"},{\"id\":1818181818,\"type\":\"OptionalInt16\"}"
			",{\"id\":1919191919,\"type\":\"OptionalInt32\"},{\"id\":2020202020,\"type\":\"OptionalInt64\"},{"
			"\"id\":2121212121,\"type\":\"OptionalUint8\"},{\"id\":2222222222,\"type\":\"OptionalUint16\"},{\"id\":"
			"2323232323,\"type\":\"OptionalUint32\"},{\"id\":2424242424,\"type\":\"OptionalUint64\"},{\"id\":"
			"2525252525,\"type\":\"OptionalDouble\"},{\"id\":2626262626,\"type\":\"OptionalFloat\"},{\"id\":"
			"2727272727,\"type\":\"String\"},{\"id\":2828282828,\"type\":\"Timer\"},{\"id\":2929292929,\"type\":"
			"\"Duration\"},{\"id\":3030303030,\"type\":\"Bool\"},{\"id\":3131313131,\"type\":\"String\"},{\"id\":"
			"3232323232,\"type\":\"OptionalDouble\"},{\"id\":3333333333,\"type\":\"OptionalDouble\"}],\"Rows\":[["
			"\"\",\"1999-03-04 12:44:23.000746384\",\"7929342421 nanoseconds\",\"7929342421 "
			"nanoseconds\",\"7929342421 "
			"nanoseconds\",-128,-100,11111,11111,0,100,11111,11111,-0.84291000000000005,-0.842909992,true,-128,"
			"null,287918237,287918237,0,18589,287918237,287918237,-0.84291000000000005,-0.842909992,\"\",\"1999-03-"
			"04 12:44:23.000746384\",\"7929342421 "
			"nanoseconds\",true,\"\",-0.84291000000000005,-0.84291000000000005],[\"\",\"2023-11-27 "
			"00:00:00.000000000\",\"348238000 nanoseconds\",\"348238000 nanoseconds\",\"348238000 "
			"nanoseconds\",-64,200,22222,22222,1,200,22222,22222,0.00000000000000000,0.000000000,true,-64,null,"
			"null,null,1,null,null,null,0.00000000000000000,0.000000000,\"\",\"2023-11-27 "
			"00:00:00.000000000\",\"348238000 "
			"nanoseconds\",true,\"\",0.00000000000000000,0.00000000000000000],[\"Some random --2 string "
			"here\",\"2024-05-12 00:00:00.000000000\",\"348225223423438000 nanoseconds\",\"348225223423438000 "
			"nanoseconds\",\"348225223423438000 "
			"nanoseconds\",-32,30,-33333,-33333,2,30,33333,33333,23492.43582999999853200,23492.435546875,false,"
			"null,null,9098345,9098345,null,54377,9098345,9098345,23492.43582999999853200,23492.435546875,\"Some "
			"random --2 string here\",\"2024-05-12 00:00:00.000000000\",\"348225223423438000 "
			"nanoseconds\",false,\"Some random --2 string "
			"here\",23492.43582999999853200,23492.43582999999853200],[\"Some random string ---3 "
			"here\",\"1970-01-01 00:00:00.000000000\",\"343248238000 nanoseconds\",\"343248238000 "
			"nanoseconds\",\"343248238000 "
			"nanoseconds\",-16,0,44444,44444,3,0,44444,44444,-0.00002342340000000,-0.000023423,true,-16,null,null,"
			"null,3,null,null,null,-0.00002342340000000,-0.000023423,\"Some random string ---3 here\",\"1970-01-01 "
			"00:00:00.000000000\",\"343248238000 nanoseconds\",true,\"Some random string ---3 "
			"here\",-0.00002342340000000,-0.00002342340000000],[\"Some random string here ----4\",\"2024-05-12 "
			"00:00:00.000000000\",\"234234000000 nanoseconds\",\"234234000000 nanoseconds\",\"234234000000 "
			"nanoseconds\",-8,-41,55555,55555,4,41,55555,55555,4583045.00234999973326921,4583045.000000000,false,-"
			"8,null,209348023,209348023,4,26039,209348023,209348023,null,null,\"Some random string here "
			"----4\",\"2024-05-12 00:00:00.000000000\",\"234234000000 nanoseconds\",false,\"Some random string "
			"here ----4\",null,null],[\"-----5 Some random string here\",\"1970-01-01 "
			"00:00:00.000000000\",\"28434000000000 nanoseconds\",\"28434000000000 nanoseconds\",\"28434000000000 "
			"nanoseconds\",-4,52,66666,66666,5,52,66666,66666,-2342234.23482001014053822,-2342234.250000000,false,-"
			"4,null,1154250426,99938498234,5,30394,1154250426,99938498234,-2342234.23482001014053822,-2342234."
			"250000000,\"-----5 Some random string here\",\"1970-01-01 00:00:00.000000000\",\"28434000000000 "
			"nanoseconds\",false,\"-----5 Some random string "
			"here\",-2342234.23482001014053822,-2342234.23482001014053822],[\"Some ------6 random string "
			"here\",\"2024-05-12 00:00:00.000000000\",\"1407180000000000 nanoseconds\",\"1407180000000000 "
			"nanoseconds\",\"1407180000000000 "
			"nanoseconds\",-2,63,-77777,-77777,6,63,77777,77777,-7.89123456000000001,-7.891234398,false,-2,null,0,"
			"0,6,0,0,0,-7.89123456000000001,-7.891234398,\"Some ------6 random string here\",\"2024-05-12 "
			"00:00:00.000000000\",\"1407180000000000 nanoseconds\",false,\"Some ------6 random string "
			"here\",-7.89123456000000001,-7.89123456000000001],[\"Some random -------7 string here\",\"2024-05-12 "
			"00:00:00.000000000\",\"950400000000000 nanoseconds\",\"950400000000000 "
			"nanoseconds\",\"950400000000000 "
			"nanoseconds\",-1,74,88888,88888,7,74,88888,88888,8.91234567000000055,8.912345886,true,-1,null,null,"
			"null,7,null,null,null,8.91234567000000055,8.912345886,\"Some random -------7 string "
			"here\",\"2024-05-12 00:00:00.000000000\",\"950400000000000 nanoseconds\",true,\"Some random -------7 "
			"string here\",8.91234567000000055,8.91234567000000055],[\"Some random string --------8 "
			"here\",\"2024-05-12 00:00:00.000000000\",\"556502400000000000 nanoseconds\",\"556502400000000000 "
			"nanoseconds\",\"556502400000000000 "
			"nanoseconds\",0,85,99999,99999,8,85,99999,99999,-9.12345677999999971,-9.123456955,true,null,null,-"
			"829413270,238472934729834,null,10346,3465554026,238472934729834,-9.12345677999999971,-9.123456955,"
			"\"Some random string --------8 here\",\"2024-05-12 00:00:00.000000000\",\"556502400000000000 "
			"nanoseconds\",true,\"Some random string --------8 "
			"here\",-9.12345677999999971,-9.12345677999999971],[\"\",\"2024-05-12 00:00:00.000000000\",\"0 "
			"nanoseconds\",\"0 nanoseconds\",\"0 "
			"nanoseconds\",1,96,-1010101010,-1010101010,9,96,1010101010,1010101010,10.23456788999999922,10."
			"234567642,false,null,null,27346277,27346277,null,17765,27346277,27346277,null,null,\"\",\"2024-05-12 "
			"00:00:00.000000000\",\"0 nanoseconds\",false,\"\",null,null],[\"0 Some random string "
			"here\",\"1999-03-04 12:44:23.000746384\",\"7929342421 nanoseconds\",\"7929342421 "
			"nanoseconds\",\"7929342421 "
			"nanoseconds\",2,107,1111111111,1111111111,10,107,1111111111,1111111111,-0.84291000000000005,-0."
			"842909992,true,2,null,287918237,287918237,10,18589,287918237,287918237,-0.84291000000000005,-0."
			"842909992,\"0 Some random string here\",\"1999-03-04 12:44:23.000746384\",\"7929342421 "
			"nanoseconds\",true,\"0 Some random string here\",-0.84291000000000005,-0.84291000000000005],[\"Some "
			"-1 random string here\",\"2023-11-27 00:00:00.000000000\",\"348238000 nanoseconds\",\"348238000 "
			"nanoseconds\",\"348238000 "
			"nanoseconds\",4,118,1212121212,1212121212,11,118,1212121212,1212121212,0.00000000000000000,0."
			"000000000,true,4,null,null,null,11,null,null,null,0.00000000000000000,0.000000000,\"Some -1 random "
			"string here\",\"2023-11-27 00:00:00.000000000\",\"348238000 nanoseconds\",true,\"Some -1 random "
			"string here\",0.00000000000000000,0.00000000000000000],[\"Some random --2 string here\",\"2024-05-12 "
			"00:00:00.000000000\",\"348225223423438000 nanoseconds\",\"348225223423438000 "
			"nanoseconds\",\"348225223423438000 "
			"nanoseconds\",8,-129,1313131313,1313131313,12,129,1313131313,1313131313,23492.43582999999853200,23492."
			"435546875,true,8,null,9098345,9098345,12,54377,9098345,9098345,23492.43582999999853200,23492."
			"435546875,\"Some random --2 string here\",\"2024-05-12 00:00:00.000000000\",\"348225223423438000 "
			"nanoseconds\",true,\"Some random --2 string "
			"here\",23492.43582999999853200,23492.43582999999853200],[\"Some random string ---3 "
			"here\",\"1970-01-01 00:00:00.000000000\",\"343248238000 nanoseconds\",\"343248238000 "
			"nanoseconds\",\"343248238000 "
			"nanoseconds\",16,140,-1414141414,-1414141414,100,140,1414141414,1414141414,-0.00002342340000000,-0."
			"000023423,false,16,null,null,null,100,null,null,null,-0.00002342340000000,-0.000023423,\"Some random "
			"string ---3 here\",\"1970-01-01 00:00:00.000000000\",\"343248238000 nanoseconds\",false,\"Some random "
			"string ---3 here\",-0.00002342340000000,-0.00002342340000000],[\"\",\"2024-05-12 "
			"00:00:00.000000000\",\"234234000000 nanoseconds\",\"234234000000 nanoseconds\",\"234234000000 "
			"nanoseconds\",32,151,1515151515,1515151515,127,151,1515151515,1515151515,4583045.00234999973326921,"
			"4583045.000000000,true,32,null,209348023,209348023,127,26039,209348023,209348023,4583045."
			"00234999973326921,4583045.000000000,\"\",\"2024-05-12 00:00:00.000000000\",\"234234000000 "
			"nanoseconds\",true,\"\",4583045.00234999973326921,4583045.00234999973326921],[\"-----5 Some random "
			"string here\",\"1970-01-01 00:00:00.000000000\",\"28434000000000 nanoseconds\",\"28434000000000 "
			"nanoseconds\",\"28434000000000 "
			"nanoseconds\",64,-162,1616161616,1616161616,128,162,1616161616,1616161616,-2342234.23482001014053822,-"
			"2342234.250000000,false,64,null,1154250426,99938498234,128,30394,1154250426,99938498234,-2342234."
			"23482001014053822,-2342234.250000000,\"-----5 Some random string here\",\"1970-01-01 "
			"00:00:00.000000000\",\"28434000000000 nanoseconds\",false,\"-----5 Some random string "
			"here\",-2342234.23482001014053822,-2342234.23482001014053822],[\"Some ------6 random string "
			"here\",\"2024-05-12 00:00:00.000000000\",\"1407180000000000 nanoseconds\",\"1407180000000000 "
			"nanoseconds\",\"1407180000000000 "
			"nanoseconds\",100,173,1717171717,1717171717,200,173,1717171717,1717171717,-7.89123456000000001,-7."
			"891234398,true,100,null,0,0,200,0,0,0,null,null,\"Some ------6 random string here\",\"2024-05-12 "
			"00:00:00.000000000\",\"1407180000000000 nanoseconds\",true,\"Some ------6 random string "
			"here\",null,null],[\"Some random -------7 string here\",\"2024-05-12 "
			"00:00:00.000000000\",\"950400000000000 nanoseconds\",\"950400000000000 "
			"nanoseconds\",\"950400000000000 "
			"nanoseconds\",112,184,-1818181818,-1818181818,220,184,1818181818,1818181818,8.91234567000000055,8."
			"912345886,true,112,null,null,null,220,null,null,null,8.91234567000000055,8.912345886,\"Some random "
			"-------7 string here\",\"2024-05-12 00:00:00.000000000\",\"950400000000000 nanoseconds\",true,\"Some "
			"random -------7 string here\",8.91234567000000055,8.91234567000000055],[\"Some random string "
			"--------8 here\",\"2024-05-12 00:00:00.000000000\",\"556502400000000000 "
			"nanoseconds\",\"556502400000000000 nanoseconds\",\"556502400000000000 "
			"nanoseconds\",120,195,-1919191919,-1919191919,254,195,1919191919,1919191919,-9.12345677999999971,-9."
			"123456955,true,120,null,-829413270,238472934729834,254,10346,3465554026,238472934729834,-9."
			"12345677999999971,-9.123456955,\"Some random string --------8 here\",\"2024-05-12 "
			"00:00:00.000000000\",\"556502400000000000 nanoseconds\",true,\"Some random string --------8 "
			"here\",-9.12345677999999971,-9.12345677999999971],[\"Some random string here---------9\",\"2024-05-12 "
			"00:00:00.000000000\",\"0 nanoseconds\",\"0 nanoseconds\",\"0 "
			"nanoseconds\",127,-206,2020202020,2020202020,255,206,2020202020,2020202020,10.23456788999999922,10."
			"234567642,false,null,null,27346277,27346277,null,17765,27346277,27346277,null,null,\"Some random "
			"string here---------9\",\"2024-05-12 00:00:00.000000000\",\"0 nanoseconds\",false,\"Some random "
			"string here---------9\",null,null]]}"
		};

		RETURN_IF_FALSE(t.Assert(table.ToJson(), expectedString2, "Table to json"));

		MSAPI::AutoClearPtr<void> buffer{ table.Encode() };

		const size_t expectedBufferSize{ (sizeof(bool) * 14 + sizeof(int8_t) + sizeof(int16_t) + sizeof(int32_t)
											 + sizeof(int64_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t)
											 + sizeof(uint64_t) + sizeof(double) + sizeof(float)
											 + sizeof(MSAPI::Timer) * 2 + sizeof(MSAPI::Timer::Duration) * 4)
				* 20
			+ sizeof(size_t) + 16 * sizeof(int8_t) + 14 * sizeof(int32_t) + 14 * sizeof(int64_t) + 16 * sizeof(uint8_t)
			+ 14 * sizeof(uint16_t) + 14 * sizeof(uint32_t) + 14 * sizeof(uint64_t) + 16 * sizeof(double) * 3
			+ 16 * sizeof(float) + sizeofBufferString * 3 };
		RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size"));

		table.Clear();
		RETURN_IF_FALSE(basicTableCheck(table, 0,
			{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111, 1212121212,
				1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
				2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
				2929292929, 3030303030, 3131313131, 3232323232, 3333333333 }));

		MSAPI::TableData tableData{ buffer.ptr };
		table.Copy(tableData);

		RETURN_IF_FALSE(basicTableCheck(table, 20,
			{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111, 1212121212,
				1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
				2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
				2929292929, 3030303030, 3131313131, 3232323232, 3333333333 }));

		RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size"));

		for (size_t row{ 0 }; row < 20; ++row) {
			RETURN_IF_FALSE((checkTableElement.operator()<std::string, 20, __TableTypes>(table, 0, row, bufferString)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer, 20, __TableTypes>(table, 1, row, bufferTimer)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 2, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 3, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 4, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<int8_t, 20, __TableTypes>(table, 5, row, bufferInt8)));
			RETURN_IF_FALSE((checkTableElement.operator()<int16_t, 20, __TableTypes>(table, 6, row, bufferInt16)));
			RETURN_IF_FALSE((checkTableElement.operator()<int32_t, 20, __TableTypes>(table, 7, row, bufferInt32)));
			RETURN_IF_FALSE((checkTableElement.operator()<int64_t, 20, __TableTypes>(table, 8, row, bufferInt64)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint8_t, 20, __TableTypes>(table, 9, row, bufferUint8)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint16_t, 20, __TableTypes>(table, 10, row, bufferUint16)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint32_t, 20, __TableTypes>(table, 11, row, bufferUint32)));
			RETURN_IF_FALSE((checkTableElement.operator()<uint64_t, 20, __TableTypes>(table, 12, row, bufferUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<double, 20, __TableTypes>(table, 13, row, bufferDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<float, 20, __TableTypes>(table, 14, row, bufferFloat)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, __TableTypes>(table, 15, row, bufferBool)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int8_t>, 20, __TableTypes>(
				table, 16, row, bufferOptionalInt8)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int16_t>, 20, __TableTypes>(
				table, 17, row, bufferOptionalInt16)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int32_t>, 20, __TableTypes>(
				table, 18, row, bufferOptionalInt32)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int64_t>, 20, __TableTypes>(
				table, 19, row, bufferOptionalInt64)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint8_t>, 20, __TableTypes>(
				table, 20, row, bufferOptionalUint8)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint16_t>, 20, __TableTypes>(
				table, 21, row, bufferOptionalUint16)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint32_t>, 20, __TableTypes>(
				table, 22, row, bufferOptionalUint32)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint64_t>, 20, __TableTypes>(
				table, 23, row, bufferOptionalUint64)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 20, __TableTypes>(
				table, 24, row, bufferOptionalDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<float>, 20, __TableTypes>(
				table, 25, row, bufferOptionalFloat)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<std::string, 20, __TableTypes>(table, 26, row, bufferString)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<MSAPI::Timer, 20, __TableTypes>(table, 27, row, bufferTimer)));
			RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 20, __TableTypes>(
				table, 28, row, bufferTimerDuration)));
			RETURN_IF_FALSE((checkTableElement.operator()<bool, 20, __TableTypes>(table, 29, row, bufferBool)));
			RETURN_IF_FALSE(
				(checkTableElement.operator()<std::string, 20, __TableTypes>(table, 30, row, bufferString)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 20, __TableTypes>(
				table, 31, row, bufferOptionalDouble)));
			RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 20, __TableTypes>(
				table, 32, row, bufferOptionalDouble)));
		}

		std::array<int8_t, 40> bufferInt82{ -128, -64, -32, -16, -8, -4, -2, -1, 0, 1, 2, 4, 8, 16, 32, 64, 100, 112,
			120, 127, -128, -64, -32, -16, -8, -4, -2, -1, 0, 1, 2, 4, 8, 16, 32, 64, 100, 112, 120, 127 };
		std::array<int16_t, 40> bufferInt162{ -100, 200, 30, 0, -41, 52, 63, 74, 85, 96, 107, 118, -129, 140, 151, -162,
			173, 184, 195, -206, -100, 200, 30, 0, -41, 52, 63, 74, 85, 96, 107, 118, -129, 140, 151, -162, 173, 184,
			195, -206 };
		std::array<int32_t, 40> bufferInt322{ 11111, 22222, -33333, 44444, 55555, 66666, -77777, 88888, 99999,
			-1010101010, 1111111111, 1212121212, 1313131313, -1414141414, 1515151515, 1616161616, 1717171717,
			-1818181818, -1919191919, 2020202020, 11111, 22222, -33333, 44444, 55555, 66666, -77777, 88888, 99999,
			-1010101010, 1111111111, 1212121212, 1313131313, -1414141414, 1515151515, 1616161616, 1717171717,
			-1818181818, -1919191919, 2020202020 };
		std::array<int64_t, 40> bufferInt642{ 11111, 22222, -33333, 44444, 55555, 66666, -77777, 88888, 99999,
			-1010101010, 1111111111, 1212121212, 1313131313, -1414141414, 1515151515, 1616161616, 1717171717,
			-1818181818, -1919191919, 2020202020, 11111, 22222, -33333, 44444, 55555, 66666, -77777, 88888, 99999,
			-1010101010, 1111111111, 1212121212, 1313131313, -1414141414, 1515151515, 1616161616, 1717171717,
			-1818181818, -1919191919, 2020202020 };
		std::array<uint8_t, 40> bufferUint82{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 100, 127, 128, 200, 220, 254,
			255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 100, 127, 128, 200, 220, 254, 255 };
		std::array<uint16_t, 40> bufferUint162{ 100, 200, 30, 0, 41, 52, 63, 74, 85, 96, 107, 118, 129, 140, 151, 162,
			173, 184, 195, 206, 100, 200, 30, 0, 41, 52, 63, 74, 85, 96, 107, 118, 129, 140, 151, 162, 173, 184, 195,
			206 };
		std::array<uint32_t, 40> bufferUint322{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999,
			1010101010, 1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818,
			1919191919, 2020202020, 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010,
			1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919,
			2020202020 };
		std::array<uint64_t, 40> bufferUint642{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999,
			1010101010, 1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818,
			1919191919, 2020202020, 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010,
			1111111111, 1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919,
			2020202020 };
		std::array<double, 40> bufferDouble2{ -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001,
			-7.89123456, 8.91234567, -9.12345678, 10.23456789, -0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235,
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789, -0.84291, 0, 23492.43583,
			-0.0000234234, 4583045.00235, -2342234.23482001, -7.89123456, 8.91234567, -9.12345678, 10.23456789,
			-0.84291, 0, 23492.43583, -0.0000234234, 4583045.00235, -2342234.23482001, -7.89123456, 8.91234567,
			-9.12345678, 10.23456789 };
		std::array<float, 40> bufferFloat2{ -0.84291f, 0.f, 23492.43583f, -0.0000234234f, 4583045.00235f,
			-2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, 10.23456789f, -0.84291f, 0.f, 23492.43583f,
			-0.0000234234f, 4583045.00235f, -2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, 10.23456789f,
			-0.84291f, 0.f, 23492.43583f, -0.0000234234f, 4583045.00235f, -2342234.23482001f, -7.89123456f, 8.91234567f,
			-9.12345678f, 10.23456789f, -0.84291f, 0.f, 23492.43583f, -0.0000234234f, 4583045.00235f,
			-2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, 10.23456789f };
		std::array<bool, 40> bufferBool2{ true, true, false, true, false, false, false, true, true, false, true, true,
			true, false, true, false, true, true, true, false, true, true, false, true, false, false, false, true, true,
			false, true, true, true, false, true, false, true, true, true, false };
		std::array<std::optional<int8_t>, 40> bufferOptionalInt82{ -128, -64, {}, -16, -8, -4, -2, -1, {}, {}, 2, 4, 8,
			16, 32, 64, 100, 112, 120, {}, -128, -64, {}, -16, -8, -4, -2, -1, {}, {}, 2, 4, 8, 16, 32, 64, 100, 112,
			120, {} };
		std::array<std::optional<int16_t>, 40> bufferOptionalInt162{ std::optional<int16_t>{}, {}, {}, {}, {}, {}, {},
			{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
			{}, {}, {}, {}, {}, {} };
		std::array<std::optional<int32_t>, 40> bufferOptionalInt322{ 287918237, {}, 9098345, {}, 209348023, 99938498234,
			0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834,
			27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834, 27346277 };
		std::array<std::optional<int64_t>, 40> bufferOptionalInt642{ 287918237, {}, 9098345, {}, 209348023, 99938498234,
			0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834,
			27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834, 27346277 };
		std::array<std::optional<uint8_t>, 40> bufferOptionalUint82{ 0, 1, {}, 3, 4, 5, 6, 7, {}, {}, 10, 11, 12, 100,
			127, 128, 200, 220, 254, {}, 0, 1, {}, 3, 4, 5, 6, 7, {}, {}, 10, 11, 12, 100, 127, 128, 200, 220, 254,
			{} };
		std::array<std::optional<uint16_t>, 40> bufferOptionalUint162{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834,
			27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834, 27346277 };
		std::array<std::optional<uint32_t>, 40> bufferOptionalUint322{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834,
			27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834, 27346277 };
		std::array<std::optional<uint64_t>, 40> bufferOptionalUint642{ 287918237, {}, 9098345, {}, 209348023,
			99938498234, 0, {}, 238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {},
			238472934729834, 27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834,
			27346277, 287918237, {}, 9098345, {}, 209348023, 99938498234, 0, {}, 238472934729834, 27346277 };
		std::array<std::optional<double>, 40> bufferOptionalDouble2{ -0.84291, 0, 23492.43583, -0.0000234234, {},
			-2342234.23482001, -7.89123456, 8.91234567, -9.12345678, {}, -0.84291, 0, 23492.43583, -0.0000234234,
			4583045.00235, -2342234.23482001, {}, 8.91234567, -9.12345678, {}, -0.84291, 0, 23492.43583, -0.0000234234,
			{}, -2342234.23482001, -7.89123456, 8.91234567, -9.12345678, {}, -0.84291, 0, 23492.43583, -0.0000234234,
			4583045.00235, -2342234.23482001, {}, 8.91234567, -9.12345678, {} };
		std::array<std::optional<float>, 40> bufferOptionalFloat2{ -0.84291f, 0.f, 23492.43583f, -0.0000234234f, {},
			-2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, {}, -0.84291f, 0.f, 23492.43583f,
			-0.0000234234f, 4583045.00235f, -2342234.23482001f, {}, 8.91234567f, -9.12345678f, {}, -0.84291f, 0.f,
			23492.43583f, -0.0000234234f, {}, -2342234.23482001f, -7.89123456f, 8.91234567f, -9.12345678f, {},
			-0.84291f, 0.f, 23492.43583f, -0.0000234234f, 4583045.00235f, -2342234.23482001f, {}, 8.91234567f,
			-9.12345678f, {} };
		std::array<std::string, 40> bufferString2{ "", "", "Some random --2 string here",
			"Some random string ---3 here", "Some random string here ----4", "-----5 Some random string here",
			"Some ------6 random string here", "Some random -------7 string here", "Some random string --------8 here",
			"", "0 Some random string here", "Some -1 random string here", "Some random --2 string here",
			"Some random string ---3 here", "", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here", "Some random string --------8 here",
			"Some random string here---------9", "", "", "Some random --2 string here", "Some random string ---3 here",
			"Some random string here ----4", "-----5 Some random string here", "Some ------6 random string here",
			"Some random -------7 string here", "Some random string --------8 here", "", "0 Some random string here",
			"Some -1 random string here", "Some random --2 string here", "Some random string ---3 here", "",
			"-----5 Some random string here", "Some ------6 random string here", "Some random -------7 string here",
			"Some random string --------8 here", "Some random string here---------9" };
		std::array<MSAPI::Timer, 40> bufferTimer2{ MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(1999, 3, 4, 12, 44, 23, 746384),
			MSAPI::Timer::Create(2023, 11, 27, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 },
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), { 0 }, MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0), MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0),
			MSAPI::Timer::Create(2024, 5, 12, 0, 0, 0, 0) };
		std::array<MSAPI::Timer::Duration, 40> bufferTimerDuration2{
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
			MSAPI::Timer::Duration::CreateMinutes(0), MSAPI::Timer::Duration{ 7929342421 },
			MSAPI::Timer::Duration::CreateMicroseconds(348238),
			MSAPI::Timer::Duration::CreateMicroseconds(348225223423438),
			MSAPI::Timer::Duration::CreateMicroseconds(343248238), MSAPI::Timer::Duration::CreateMilliseconds(234234),
			MSAPI::Timer::Duration::CreateSeconds(28434), MSAPI::Timer::Duration::CreateMinutes(23453),
			MSAPI::Timer::Duration::CreateHours(264), MSAPI::Timer::Duration::CreateDays(6441),
			MSAPI::Timer::Duration::CreateMinutes(0), MSAPI::Timer::Duration::CreateNanoseconds(7929342421),
			MSAPI::Timer::Duration::CreateMicroseconds(348238),
			MSAPI::Timer::Duration::CreateMicroseconds(348225223423438),
			MSAPI::Timer::Duration::CreateMicroseconds(343248238), MSAPI::Timer::Duration::CreateMilliseconds(234234),
			MSAPI::Timer::Duration::CreateSeconds(28434), MSAPI::Timer::Duration::CreateMinutes(23453),
			MSAPI::Timer::Duration::CreateHours(264), MSAPI::Timer::Duration::CreateDays(6441),
			MSAPI::Timer::Duration::CreateMinutes(0)
		};

		for (size_t row{ 0 }; row < 20; ++row) {
			table.AddRow(bufferString[row], bufferTimer[row], bufferTimerDuration[row], bufferTimerDuration[row],
				bufferTimerDuration[row], bufferInt8[row], bufferInt16[row], bufferInt32[row], bufferInt64[row],
				bufferUint8[row], bufferUint16[row], bufferUint32[row], bufferUint64[row], bufferDouble[row],
				bufferFloat[row], bufferBool[row], bufferOptionalInt8[row], bufferOptionalInt16[row],
				bufferOptionalInt32[row], bufferOptionalInt64[row], bufferOptionalUint8[row], bufferOptionalUint16[row],
				bufferOptionalUint32[row], bufferOptionalUint64[row], bufferOptionalDouble[row],
				bufferOptionalFloat[row], bufferString[row], bufferTimer[row], bufferTimerDuration[row],
				bufferBool[row], bufferString[row], bufferOptionalDouble[row], bufferOptionalDouble[row]);
		}

		const auto check{ [&] [[nodiscard]] () {
			RETURN_IF_FALSE(basicTableCheck(table, 40,
				{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111, 1212121212,
					1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
					2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
					2929292929, 3030303030, 3131313131, 3232323232, 3333333333 }));

			const size_t expectedBufferSize{ (sizeof(bool) * 14 + sizeof(int8_t) + sizeof(int16_t) + sizeof(int32_t)
												 + sizeof(int64_t) + sizeof(uint8_t) + sizeof(uint16_t)
												 + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(double) + sizeof(float)
												 + sizeof(MSAPI::Timer) * 2 + sizeof(MSAPI::Timer::Duration) * 4)
					* 40
				+ sizeof(size_t) + 32 * sizeof(int8_t) + 28 * sizeof(int32_t) + 28 * sizeof(int64_t)
				+ 32 * sizeof(uint8_t) + 28 * sizeof(uint16_t) + 28 * sizeof(uint32_t) + 28 * sizeof(uint64_t)
				+ 32 * sizeof(double) * 3 + 32 * sizeof(float) + sizeofBufferString * 6 };
			RETURN_IF_FALSE(t.Assert(table.GetBufferSize(), expectedBufferSize, "Table buffer size"));

			for (size_t row{ 0 }; row < 20; ++row) {
				RETURN_IF_FALSE(
					(checkTableElement.operator()<std::string, 40, __TableTypes>(table, 0, row, bufferString2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<MSAPI::Timer, 40, __TableTypes>(table, 1, row, bufferTimer2)));
				RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 40, __TableTypes>(
					table, 2, row, bufferTimerDuration2)));
				RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 40, __TableTypes>(
					table, 3, row, bufferTimerDuration2)));
				RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 40, __TableTypes>(
					table, 4, row, bufferTimerDuration2)));
				RETURN_IF_FALSE((checkTableElement.operator()<int8_t, 40, __TableTypes>(table, 5, row, bufferInt82)));
				RETURN_IF_FALSE((checkTableElement.operator()<int16_t, 40, __TableTypes>(table, 6, row, bufferInt162)));
				RETURN_IF_FALSE((checkTableElement.operator()<int32_t, 40, __TableTypes>(table, 7, row, bufferInt322)));
				RETURN_IF_FALSE((checkTableElement.operator()<int64_t, 40, __TableTypes>(table, 8, row, bufferInt642)));
				RETURN_IF_FALSE((checkTableElement.operator()<uint8_t, 40, __TableTypes>(table, 9, row, bufferUint82)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<uint16_t, 40, __TableTypes>(table, 10, row, bufferUint162)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<uint32_t, 40, __TableTypes>(table, 11, row, bufferUint322)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<uint64_t, 40, __TableTypes>(table, 12, row, bufferUint642)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<double, 40, __TableTypes>(table, 13, row, bufferDouble2)));
				RETURN_IF_FALSE((checkTableElement.operator()<float, 40, __TableTypes>(table, 14, row, bufferFloat2)));
				RETURN_IF_FALSE((checkTableElement.operator()<bool, 40, __TableTypes>(table, 15, row, bufferBool2)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int8_t>, 40, __TableTypes>(
					table, 16, row, bufferOptionalInt82)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int16_t>, 40, __TableTypes>(
					table, 17, row, bufferOptionalInt162)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int32_t>, 40, __TableTypes>(
					table, 18, row, bufferOptionalInt322)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<int64_t>, 40, __TableTypes>(
					table, 19, row, bufferOptionalInt642)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint8_t>, 40, __TableTypes>(
					table, 20, row, bufferOptionalUint82)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint16_t>, 40, __TableTypes>(
					table, 21, row, bufferOptionalUint162)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint32_t>, 40, __TableTypes>(
					table, 22, row, bufferOptionalUint322)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<uint64_t>, 40, __TableTypes>(
					table, 23, row, bufferOptionalUint642)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 40, __TableTypes>(
					table, 24, row, bufferOptionalDouble2)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<float>, 40, __TableTypes>(
					table, 25, row, bufferOptionalFloat2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<std::string, 40, __TableTypes>(table, 26, row, bufferString2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<MSAPI::Timer, 40, __TableTypes>(table, 27, row, bufferTimer2)));
				RETURN_IF_FALSE((checkTableElement.operator()<MSAPI::Timer::Duration, 40, __TableTypes>(
					table, 28, row, bufferTimerDuration2)));
				RETURN_IF_FALSE((checkTableElement.operator()<bool, 40, __TableTypes>(table, 29, row, bufferBool2)));
				RETURN_IF_FALSE(
					(checkTableElement.operator()<std::string, 40, __TableTypes>(table, 30, row, bufferString2)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 40, __TableTypes>(
					table, 31, row, bufferOptionalDouble2)));
				RETURN_IF_FALSE((checkTableElement.operator()<std::optional<double>, 40, __TableTypes>(
					table, 32, row, bufferOptionalDouble2)));
#undef __TableTypes
			}

			return true;
		} };

		RETURN_IF_FALSE(check());

		std::reverse(bufferInt82.begin(), bufferInt82.end());
		std::reverse(bufferInt162.begin(), bufferInt162.end());
		std::reverse(bufferInt322.begin(), bufferInt322.end());
		std::reverse(bufferInt642.begin(), bufferInt642.end());
		std::reverse(bufferUint82.begin(), bufferUint82.end());
		std::reverse(bufferUint162.begin(), bufferUint162.end());
		std::reverse(bufferUint322.begin(), bufferUint322.end());
		std::reverse(bufferUint642.begin(), bufferUint642.end());
		std::reverse(bufferDouble2.begin(), bufferDouble2.end());
		std::reverse(bufferFloat2.begin(), bufferFloat2.end());
		std::reverse(bufferBool2.begin(), bufferBool2.end());
		std::reverse(bufferOptionalInt82.begin(), bufferOptionalInt82.end());
		std::reverse(bufferOptionalInt162.begin(), bufferOptionalInt162.end());
		std::reverse(bufferOptionalInt322.begin(), bufferOptionalInt322.end());
		std::reverse(bufferOptionalInt642.begin(), bufferOptionalInt642.end());
		std::reverse(bufferOptionalUint82.begin(), bufferOptionalUint82.end());
		std::reverse(bufferOptionalUint162.begin(), bufferOptionalUint162.end());
		std::reverse(bufferOptionalUint322.begin(), bufferOptionalUint322.end());
		std::reverse(bufferOptionalUint642.begin(), bufferOptionalUint642.end());
		std::reverse(bufferOptionalDouble2.begin(), bufferOptionalDouble2.end());
		std::reverse(bufferOptionalFloat2.begin(), bufferOptionalFloat2.end());
		std::reverse(bufferString2.begin(), bufferString2.end());
		std::reverse(bufferTimer2.begin(), bufferTimer2.end());
		std::reverse(bufferTimerDuration2.begin(), bufferTimerDuration2.end());

		for (size_t row{ 0 }; row < 40; ++row) {
			table.UpdateCell(0, row, bufferString2[row]);
			table.UpdateCell(1, row, bufferTimer2[row]);
			table.UpdateCell(2, row, bufferTimerDuration2[row]);
			table.UpdateCell(3, row, bufferTimerDuration2[row]);
			table.UpdateCell(4, row, bufferTimerDuration2[row]);
			table.UpdateCell(5, row, bufferInt82[row]);
			table.UpdateCell(6, row, bufferInt162[row]);
			table.UpdateCell(7, row, bufferInt322[row]);
			table.UpdateCell(8, row, bufferInt642[row]);
			table.UpdateCell(9, row, bufferUint82[row]);
			table.UpdateCell(10, row, bufferUint162[row]);
			table.UpdateCell(11, row, bufferUint322[row]);
			table.UpdateCell(12, row, bufferUint642[row]);
			table.UpdateCell(13, row, bufferDouble2[row]);
			table.UpdateCell(14, row, bufferFloat2[row]);
			table.UpdateCell(15, row, bufferBool2[row]);
			table.UpdateCell(16, row, bufferOptionalInt82[row]);
			table.UpdateCell(17, row, bufferOptionalInt162[row]);
			table.UpdateCell(18, row, bufferOptionalInt322[row]);
			table.UpdateCell(19, row, bufferOptionalInt642[row]);
			table.UpdateCell(20, row, bufferOptionalUint82[row]);
			table.UpdateCell(21, row, bufferOptionalUint162[row]);
			table.UpdateCell(22, row, bufferOptionalUint322[row]);
			table.UpdateCell(23, row, bufferOptionalUint642[row]);
			table.UpdateCell(24, row, bufferOptionalDouble2[row]);
			table.UpdateCell(25, row, bufferOptionalFloat2[row]);
			table.UpdateCell(26, row, bufferString2[row]);
			table.UpdateCell(27, row, bufferTimer2[row]);
			table.UpdateCell(28, row, bufferTimerDuration2[row]);
			table.UpdateCell(29, row, bufferBool2[row]);
			table.UpdateCell(30, row, bufferString2[row]);
			table.UpdateCell(31, row, bufferOptionalDouble2[row]);
			table.UpdateCell(32, row, bufferOptionalDouble2[row]);
		}

		RETURN_IF_FALSE(checkTableData(table));

		RETURN_IF_FALSE(check());

		RETURN_IF_FALSE(checkCopy(table));
		table.Clear();
		RETURN_IF_FALSE(basicTableCheck(table, 0,
			{ 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111, 1212121212,
				1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
				2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828,
				2929292929, 3030303030, 3131313131, 3232323232, 3333333333 }));
		RETURN_IF_FALSE(checkEmptyPrints());
	}

	{
		MSAPI::Table<bool> table1;
		RETURN_IF_FALSE(basicTableCheck(table1, 0, { 0 }));

		MSAPI::Table<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t> table2;
		RETURN_IF_FALSE(basicTableCheck(table2, 0, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

		MSAPI::Table<int32_t, int32_t> table3{ 1, 1 };
		RETURN_IF_FALSE(t.Assert(table3.GetColumnsSize(), 0, "Table ill formed"));
	}

	{
		MSAPI::Table<bool> table{ std::vector<size_t>{ 153 } };
		RETURN_IF_FALSE(basicTableCheck(table, 0, { 153 }));

		MSAPI::Table<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t> table2{
			std::vector<size_t>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
		};
		RETURN_IF_FALSE(basicTableCheck(table2, 0, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

		MSAPI::Table<int32_t, int32_t> table3{ std::vector<size_t>{ 1, 1 } };
		RETURN_IF_FALSE(t.Assert(table3.GetColumnsSize(), 0, "Table ill formed"));
	}

	{
		struct FirstTest {
			enum class TestEnum : int8_t { Undefined, Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Max };

			[[nodiscard]] static std::string_view EnumToString(const TestEnum value)
			{
				switch (value) {
				case TestEnum::Undefined:
					return "Undefined 1";
				case TestEnum::Zero:
					return "Zero 1";
				case TestEnum::One:
					return "One 1";
				case TestEnum::Two:
					return "Two 1";
				case TestEnum::Three:
					return "Three 1";
				case TestEnum::Four:
					return "Four 1";
				case TestEnum::Five:
					return "Five 1";
				case TestEnum::Six:
					return "Six 1";
				case TestEnum::Seven:
					return "Seven 1";
				case TestEnum::Eight:
					return "Eight 1";
				case TestEnum::Nine:
					return "Nine 1";
				case TestEnum::Max:
					return "Max 1";
				default:
					return "Unknown 1";
				}
			}
		};

		struct SecondTest {
			enum class TestEnum : int32_t {
				Undefined,
				Zero,
				One,
				Two,
				Three,
				Four,
				Five,
				Six,
				Seven,
				Eight,
				Nine,
				Max
			};

			[[nodiscard]] static std::string_view EnumToString(const TestEnum value)
			{
				switch (value) {
				case TestEnum::Undefined:
					return "Undefined 2";
				case TestEnum::Zero:
					return "Zero 2";
				case TestEnum::One:
					return "One 2";
				case TestEnum::Two:
					return "Two 2";
				case TestEnum::Three:
					return "Three 2";
				case TestEnum::Four:
					return "Four 2";
				case TestEnum::Five:
					return "Five 2";
				case TestEnum::Six:
					return "Six 2";
				case TestEnum::Seven:
					return "Seven 2";
				case TestEnum::Eight:
					return "Eight 2";
				case TestEnum::Nine:
					return "Nine 2";
				case TestEnum::Max:
					return "Max 2";
				default:
					return "Unknown 2";
				}
			}
		};

		MSAPI::Table<FirstTest::TestEnum, SecondTest::TestEnum> table{ 153, 1599 };
		table.AddMetadataForEnum<FirstTest::TestEnum>(FirstTest::EnumToString);

		RETURN_IF_FALSE(basicTableCheck(table, 0, { 153, 1599 }));

		RETURN_IF_FALSE(t.Assert(table.GetColumnsSize(), 2, "Table ill formed"));

		table.SetColumnMetadata(0, "\"name\":\"First column\",\"privet\":\"Hello, world!\"");
		table.SetColumnMetadata(1, "\"name\":\"Second column\",\"privet\":\"Medved\"");

		auto columnIt{ table.GetColumns()->begin() };
		RETURN_IF_FALSE(t.Assert(columnIt->metadata,
			"\"stringInterpretations\":{\"0\":\"Undefined 1\",\"1\":\"Zero 1\",\"2\":\"One 1\",\"3\":\"Two "
			"1\",\"4\":\"Three 1\",\"5\":\"Four 1\",\"6\":\"Five 1\",\"7\":\"Six 1\",\"8\":\"Seven "
			"1\",\"9\":\"Eight 1\",\"10\":\"Nine 1\"},\"name\":\"First column\",\"privet\":\"Hello, world!\"",
			"Table column metadata"));

		++columnIt;
		RETURN_IF_FALSE(
			t.Assert(columnIt->metadata, "\"name\":\"Second column\",\"privet\":\"Medved\"", "Table column metadata"));

		table.AddMetadataForEnum<SecondTest::TestEnum>(SecondTest::EnumToString);
		RETURN_IF_FALSE(t.Assert(columnIt->metadata,
			"\"name\":\"Second column\",\"privet\":\"Medved\",\"stringInterpretations\":{\"0\":\"Undefined "
			"2\",\"1\":\"Zero 2\",\"2\":\"One 2\",\"3\":\"Two "
			"2\",\"4\":\"Three 2\",\"5\":\"Four 2\",\"6\":\"Five 2\",\"7\":\"Six 2\",\"8\":\"Seven "
			"2\",\"9\":\"Eight 2\",\"10\":\"Nine 2\"}",
			"Table column metadata"));

		table.SetColumnNames("First column", "Second column");
		RETURN_IF_FALSE(t.Assert(columnIt->metadata,
			"\"name\":\"Second column\",\"privet\":\"Medved\",\"stringInterpretations\":{\"0\":\"Undefined "
			"2\",\"1\":\"Zero 2\",\"2\":\"One 2\",\"3\":\"Two "
			"2\",\"4\":\"Three 2\",\"5\":\"Four 2\",\"6\":\"Five 2\",\"7\":\"Six 2\",\"8\":\"Seven "
			"2\",\"9\":\"Eight 2\",\"10\":\"Nine 2\"},\"name\":\"Second column\"",
			"Table column metadata"));

		columnIt = table.GetColumns()->begin();
		RETURN_IF_FALSE(t.Assert(columnIt->metadata,
			"\"stringInterpretations\":{\"0\":\"Undefined 1\",\"1\":\"Zero 1\",\"2\":\"One 1\",\"3\":\"Two "
			"1\",\"4\":\"Three 1\",\"5\":\"Four 1\",\"6\":\"Five 1\",\"7\":\"Six 1\",\"8\":\"Seven "
			"1\",\"9\":\"Eight 1\",\"10\":\"Nine 1\"},\"name\":\"First column\",\"privet\":\"Hello, "
			"world!\",\"name\":\"First column\"",
			"Table column metadata"));
	}

	{
		MSAPI::TableData tableData;
		RETURN_IF_FALSE(t.Assert(tableData.GetBufferSize(), 8, "TableData buffer size"));
		RETURN_IF_FALSE(t.Assert(tableData.GetBuffer() != nullptr, true, "TableData buffer not nullptr"));
		RETURN_IF_FALSE(t.Assert(*static_cast<const size_t*>(tableData.GetBuffer()), 8, "TableData buffer value"));
	}

	{
		std::array<int64_t, 3> x1{ std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), 0 };
		std::array<int64_t, 3> x2{ std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), 0 };
		std::array<int64_t, 3> x3{ std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), 0 };
		std::array<int64_t, 3> x4{ std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), 0 };
		std::array<uint64_t, 3> x5{ std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::min(), 0 };
		std::array<uint64_t, 3> x6{ std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max(), 0 };
		std::array<uint64_t, 3> x7{ std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::min(), 0 };
		std::array<uint64_t, 3> x8{ std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max(), 0 };
		std::array<double, 3> x9{ 7474, -4324, 0 };
		std::array<double, 3> x10{ 234234, -48384, 0 };
		std::array<bool, 3> x11{ true, false, true };
		std::array<std::optional<int64_t>, 3> x12{ std::numeric_limits<int8_t>::max(),
			std::numeric_limits<int8_t>::min(), std::nullopt };

		std::array<std::optional<int64_t>, 3> x13{ std::numeric_limits<int16_t>::max(),
			std::numeric_limits<int16_t>::min(), std::nullopt };
		std::array<std::optional<int64_t>, 3> x14{ std::numeric_limits<int32_t>::max(),
			std::numeric_limits<int32_t>::min(), std::nullopt };
		std::array<std::optional<int64_t>, 3> x15{ std::numeric_limits<int64_t>::max(),
			std::numeric_limits<int64_t>::min(), std::nullopt };
		std::array<std::optional<uint64_t>, 3> x16{ std::numeric_limits<uint8_t>::max(),
			std::numeric_limits<uint8_t>::min(), std::nullopt };
		std::array<std::optional<uint64_t>, 3> x17{ std::numeric_limits<uint16_t>::max(),
			std::numeric_limits<uint16_t>::min(), std::nullopt };
		std::array<std::optional<uint64_t>, 3> x18{ std::numeric_limits<uint32_t>::max(),
			std::numeric_limits<uint32_t>::min(), std::nullopt };
		std::array<std::optional<uint64_t>, 3> x19{ std::numeric_limits<uint64_t>::max(),
			std::numeric_limits<uint64_t>::min(), std::nullopt };
		std::array<std::optional<double>, 3> x20{ -123123, 123123, std::nullopt };
		std::array<std::optional<double>, 3> x21{ -13.12312, 3332, std::nullopt };
		std::array<std::string, 3> x22{
			"Some random string here, and that string can be rally large one, so lets write some more text here to be "
			"sure that this thing works as expected. Some random string here, and that string can be rally large one, "
			"so lets write some more text here to be sure that this thing works as expected. Some random string here, "
			"and that string can be rally large one, so lets write some more text here to be sure that this thing "
			"works as expected. Some random string here, and that string can be rally large one, so lets write some "
			"more text here to be sure that this thing works as expected. Some random string here, and that string can "
			"be rally large one, so lets write some more text here to be sure that this thing works as expected. Some "
			"random string here, and that string can be rally large one, so lets write some more text here to be sure "
			"that this thing works as expected. Some random string here, and that string can be rally large one, so "
			"lets write some more text here to be sure that this thing works as expected. Some random string here, and "
			"that string can be rally large one, so lets write some more text here to be sure that this thing works as "
			"expected. Some random string here, and that string can be rally large one, so lets write some more text "
			"here to be sure that this thing works as expected.Some random string here, and that string can be rally "
			"large one, so lets write some more text here to be sure that this thing works as expected. Some random "
			"string here, and that string can be rally large one, so lets write some more text here to be sure that "
			"this thing works as expected. Some random string here, and that string can be rally large one, so lets "
			"write some more text here to be sure that this thing works as expected. Some random string here, and that "
			"string can be rally large one, so lets write some more text here to be sure that this thing works as "
			"expected. Some random string here, and that string can be rally large one, so lets write some more text "
			"here to be sure that this thing works as expected. Some random string here, and that string can be rally "
			"large one, so lets write some more text here to be sure that this thing works as expected. Some random "
			"string here, and that string can be rally large one, so lets write some more text here to be sure that "
			"this thing works as expected. Some random string here, and that string can be rally large one, so lets "
			"write some more text here to be sure that this thing works as expected. Some random string here, and that "
			"string can be rally large one, so lets write some more text here to be sure that this thing works as "
			"expected.",
			"Hello, here!", ""
		};
		std::array<uint64_t, 3> x23{ 922337203, 922337202, 0 };
		std::array<int64_t, 3> x24{ std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), 0 };

		std::list<MSAPI::JsonNode> rows;

		MSAPI::Table<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, double, float, bool,
			std::optional<int8_t>, std::optional<int16_t>, std::optional<int32_t>, std::optional<int64_t>,
			std::optional<uint8_t>, std::optional<uint16_t>, std::optional<uint32_t>, std::optional<uint64_t>,
			std::optional<double>, std::optional<float>, std::string, MSAPI::Timer, MSAPI::Timer::Duration>
			table;

		for (size_t index{ 0 }; index < 6; ++index) {
			std::list<MSAPI::JsonNode> row;
			row.emplace_back(x1[index % 3]);
			row.emplace_back(x2[index % 3]);
			row.emplace_back(x3[index % 3]);
			row.emplace_back(x4[index % 3]);
			row.emplace_back(x5[index % 3]);
			row.emplace_back(x6[index % 3]);
			row.emplace_back(x7[index % 3]);
			row.emplace_back(x8[index % 3]);
			row.emplace_back(x9[index % 3]);
			row.emplace_back(x10[index % 3]);
			row.emplace_back(x11[index % 3]);
			row.push_back(
				x12[index % 3].has_value() ? MSAPI::JsonNode{ x12[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x13[index % 3].has_value() ? MSAPI::JsonNode{ x13[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x14[index % 3].has_value() ? MSAPI::JsonNode{ x14[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x15[index % 3].has_value() ? MSAPI::JsonNode{ x15[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x16[index % 3].has_value() ? MSAPI::JsonNode{ x16[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x17[index % 3].has_value() ? MSAPI::JsonNode{ x17[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x18[index % 3].has_value() ? MSAPI::JsonNode{ x18[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x19[index % 3].has_value() ? MSAPI::JsonNode{ x19[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x20[index % 3].has_value() ? MSAPI::JsonNode{ x20[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.push_back(
				x21[index % 3].has_value() ? MSAPI::JsonNode{ x21[index % 3].value() } : MSAPI::JsonNode{ nullptr });
			row.emplace_back(x22[index % 3]);
			row.emplace_back(x23[index % 3] * 1000000000 + 6854775807);
			row.emplace_back(x24[index % 3]);

			rows.emplace_back(std::move(row));

			table.AddRow(static_cast<int8_t>(x1[index % 3]), static_cast<int16_t>(x2[index % 3]),
				static_cast<int32_t>(x3[index % 3]), INT64(x4[index % 3]), static_cast<uint8_t>(x5[index % 3]),
				static_cast<uint16_t>(x6[index % 3]), static_cast<uint32_t>(x7[index % 3]), UINT64(x8[index % 3]),
				static_cast<double>(x9[index % 3]), static_cast<float>(x10[index % 3]), x11[index % 3],
				std::optional<int8_t>(x12[index % 3]), std::optional<int16_t>(x13[index % 3]),
				std::optional<int32_t>(x14[index % 3]), std::optional<int64_t>(x15[index % 3]),
				std::optional<uint8_t>(x16[index % 3]), std::optional<uint16_t>(x17[index % 3]),
				std::optional<uint32_t>(x18[index % 3]), std::optional<uint64_t>(x19[index % 3]),
				std::optional<double>(x20[index % 3]), std::optional<float>(x21[index % 3]), x22[index % 3],
				MSAPI::Timer(INT64(x23[index % 3]), 6854775807), MSAPI::Timer::Duration(x24[index % 3]));
		}

		std::vector<MSAPI::StandardType::Type> columnTypes{ MSAPI::StandardType::Type::Int8,
			MSAPI::StandardType::Type::Int16, MSAPI::StandardType::Type::Int32, MSAPI::StandardType::Type::Int64,
			MSAPI::StandardType::Type::Uint8, MSAPI::StandardType::Type::Uint16, MSAPI::StandardType::Type::Uint32,
			MSAPI::StandardType::Type::Uint64, MSAPI::StandardType::Type::Double, MSAPI::StandardType::Type::Float,
			MSAPI::StandardType::Type::Bool, MSAPI::StandardType::Type::OptionalInt8,
			MSAPI::StandardType::Type::OptionalInt16, MSAPI::StandardType::Type::OptionalInt32,
			MSAPI::StandardType::Type::OptionalInt64, MSAPI::StandardType::Type::OptionalUint8,
			MSAPI::StandardType::Type::OptionalUint16, MSAPI::StandardType::Type::OptionalUint32,
			MSAPI::StandardType::Type::OptionalUint64, MSAPI::StandardType::Type::OptionalDouble,
			MSAPI::StandardType::Type::OptionalFloat, MSAPI::StandardType::Type::String,
			MSAPI::StandardType::Type::Timer, MSAPI::StandardType::Type::Duration };

		MSAPI::TableData tableDataConstructed{ rows, columnTypes };

		const std::string_view expectedJson{
			"{\"Buffer "
			"size\":5878,\"Rows\":[[127,-32768,2147483647,-9223372036854775808,255,0,4294967295,0,7474."
			"00000000000000000,234234.000000000,true,127,32767,2147483647,9223372036854775807,255,65535,4294967295,"
			"18446744073709551615,-123123,-13.12312,\"Some random string here, and that string can be rally large "
			"one, so lets write some more text here to be sure that this thing works as expected. Some random "
			"string here, and that string can be rally large one, so lets write some more text here to be sure "
			"that this thing works as expected. Some random string here, and that string can be rally large one, "
			"so lets write some more text here to be sure that this thing works as expected. Some random string "
			"here, and that string can be rally large one, so lets write some more text here to be sure that this "
			"thing works as expected. Some random string here, and that string can be rally large one, so lets "
			"write some more text here to be sure that this thing works as expected. Some random string here, and "
			"that string can be rally large one, so lets write some more text here to be sure that this thing "
			"works as expected. Some random string here, and that string can be rally large one, so lets write "
			"some more text here to be sure that this thing works as expected. Some random string here, and that "
			"string can be rally large one, so lets write some more text here to be sure that this thing works as "
			"expected. Some random string here, and that string can be rally large one, so lets write some more "
			"text here to be sure that this thing works as expected.Some random string here, and that string can "
			"be rally large one, so lets write some more text here to be sure that this thing works as expected. "
			"Some random string here, and that string can be rally large one, so lets write some more text here to "
			"be sure that this thing works as expected. Some random string here, and that string can be rally "
			"large one, so lets write some more text here to be sure that this thing works as expected. Some "
			"random string here, and that string can be rally large one, so lets write some more text here to be "
			"sure that this thing works as expected. Some random string here, and that string can be rally large "
			"one, so lets write some more text here to be sure that this thing works as expected. Some random "
			"string here, and that string can be rally large one, so lets write some more text here to be sure "
			"that this thing works as expected. Some random string here, and that string can be rally large one, "
			"so lets write some more text here to be sure that this thing works as expected. Some random string "
			"here, and that string can be rally large one, so lets write some more text here to be sure that this "
			"thing works as expected. Some random string here, and that string can be rally large one, so lets "
			"write some more text here to be sure that this thing works as "
			"expected.\",922337209854775807,-9223372036854775808],[-128,32767,-2147483648,9223372036854775807,0,"
			"65535,0,18446744073709551615,-4324.00000000000000000,-48384.000000000,false,-128,-32768,-2147483648,-"
			"9223372036854775808,0,0,0,0,123123,3332,\"Hello, "
			"here!\",922337208854775807,9223372036854775807],[0,0,0,0,0,0,0,0,0.00000000000000000,0.000000000,true,"
			"null,null,null,null,null,null,null,null,null,null,\"\",6854775807,0],[127,-32768,2147483647,-"
			"9223372036854775808,255,0,4294967295,0,7474.00000000000000000,234234.000000000,true,127,32767,"
			"2147483647,9223372036854775807,255,65535,4294967295,18446744073709551615,-123123,-13.12312,\"Some "
			"random string here, and that string can be rally large one, so lets write some more text here to be "
			"sure that this thing works as expected. Some random string here, and that string can be rally large "
			"one, so lets write some more text here to be sure that this thing works as expected. Some random "
			"string here, and that string can be rally large one, so lets write some more text here to be sure "
			"that this thing works as expected. Some random string here, and that string can be rally large one, "
			"so lets write some more text here to be sure that this thing works as expected. Some random string "
			"here, and that string can be rally large one, so lets write some more text here to be sure that this "
			"thing works as expected. Some random string here, and that string can be rally large one, so lets "
			"write some more text here to be sure that this thing works as expected. Some random string here, and "
			"that string can be rally large one, so lets write some more text here to be sure that this thing "
			"works as expected. Some random string here, and that string can be rally large one, so lets write "
			"some more text here to be sure that this thing works as expected. Some random string here, and that "
			"string can be rally large one, so lets write some more text here to be sure that this thing works as "
			"expected.Some random string here, and that string can be rally large one, so lets write some more "
			"text here to be sure that this thing works as expected. Some random string here, and that string can "
			"be rally large one, so lets write some more text here to be sure that this thing works as expected. "
			"Some random string here, and that string can be rally large one, so lets write some more text here to "
			"be sure that this thing works as expected. Some random string here, and that string can be rally "
			"large one, so lets write some more text here to be sure that this thing works as expected. Some "
			"random string here, and that string can be rally large one, so lets write some more text here to be "
			"sure that this thing works as expected. Some random string here, and that string can be rally large "
			"one, so lets write some more text here to be sure that this thing works as expected. Some random "
			"string here, and that string can be rally large one, so lets write some more text here to be sure "
			"that this thing works as expected. Some random string here, and that string can be rally large one, "
			"so lets write some more text here to be sure that this thing works as expected. Some random string "
			"here, and that string can be rally large one, so lets write some more text here to be sure that this "
			"thing works as "
			"expected.\",922337209854775807,-9223372036854775808],[-128,32767,-2147483648,9223372036854775807,0,"
			"65535,0,18446744073709551615,-4324.00000000000000000,-48384.000000000,false,-128,-32768,-2147483648,-"
			"9223372036854775808,0,0,0,0,123123,3332,\"Hello, "
			"here!\",922337208854775807,9223372036854775807],[0,0,0,0,0,0,0,0,0.00000000000000000,0.000000000,true,"
			"null,null,null,null,null,null,null,null,null,null,\"\",6854775807,0]]}"
		};

		RETURN_IF_FALSE(
			t.Assert(tableDataConstructed.LookUpToJson(columnTypes), expectedJson, "TableData JSON representation"));

		RETURN_IF_FALSE(t.Assert(tableDataConstructed.GetBufferSize(), table.GetBufferSize(), "TableData buffer size"));

		MSAPI::TableData tableData{ table };

		RETURN_IF_FALSE(
			t.Assert(memcmp(tableDataConstructed.GetBuffer(), tableData.GetBuffer(), tableData.GetBufferSize()), 0,
				"TableData buffer equals Table buffer"));
	}

	{
		MSAPI::Table<int16_t, double> table;

		std::list<JsonNode> rows;
		rows.emplace_back(std::list<JsonNode>{ JsonNode{ uint64_t(1) }, uint64_t(100) });
		table.AddRow(int16_t(1), double(100));
		rows.emplace_back(std::list<JsonNode>{ JsonNode{ uint64_t(2) }, int64_t(-100) });
		table.AddRow(int16_t(2), double(-100));
		rows.emplace_back(std::list<JsonNode>{ JsonNode{ uint64_t(3) }, uint64_t(0) });
		table.AddRow(int16_t(3), double(0));
		rows.emplace_back(std::list<JsonNode>{ JsonNode{ uint64_t(4) }, 100.1003 });
		table.AddRow(int16_t(4), double(100.1003));
		rows.emplace_back(std::list<JsonNode>{ JsonNode{ uint64_t(5) }, -100.1003 });
		table.AddRow(int16_t(5), double(-100.1003));
		rows.emplace_back(std::list<JsonNode>{ JsonNode{ uint64_t(6) }, -0.1003 });
		table.AddRow(int16_t(6), double(-0.1003));
		std::vector<MSAPI::StandardType::Type> columnTypes{ MSAPI::StandardType::Type::Int16,
			MSAPI::StandardType::Type::Double };

		MSAPI::TableData tableDataConstructed{ rows, columnTypes };

		const std::string_view expectedJson{
			"{\"Buffer "
			"size\":68,\"Rows\":[[1,100.00000000000000000],[2,-100.00000000000000000],[3,0."
			"00000000000000000],[4,100.10030000000000427],[5,-100.10030000000000427],[6,-0."
			"10030000000000000]]}"
		};
		RETURN_IF_FALSE(
			t.Assert(tableDataConstructed.LookUpToJson(columnTypes), expectedJson, "TableData JSON representation"));

		RETURN_IF_FALSE(t.Assert(tableDataConstructed.GetBufferSize(), table.GetBufferSize(), "TableData buffer size"));

		MSAPI::TableData tableData{ table };

		RETURN_IF_FALSE(
			t.Assert(memcmp(tableDataConstructed.GetBuffer(), tableData.GetBuffer(), tableData.GetBufferSize()) == 0,
				true, "TableData buffer equals Table buffer"));
	}

	{
		MSAPI::Table<bool> table1;
		MSAPI::Table<int64_t> table2;

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table1 }, false,
			"TableData of empty table is equal to itself, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ table1 },
			"TableData of empty table is equal to itself, operator =="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table2 }, false,
			"TableData of empty table is equal to same TableData, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ table2 },
			"TableData of empty table is equal to same TableData, operator =="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ nullptr }, false,
			"TableData of empty table is equal to TableData from nullptr, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ nullptr },
			"TableData of empty table is equal to TableData from nullptr, operator =="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ nullptr } != MSAPI::TableData{ nullptr }, false,
			"TableData from nullptr is equal to same TableData, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ nullptr }, MSAPI::TableData{ nullptr },
			"TableData from nullptr is equal to same TableData, operator =="));

		MSAPI::AutoClearPtr<void> table1Data1{ table1.Encode() };
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table1Data1.ptr }, false,
			"TableData of empty table is equal to TableData from pointer to its table encoded buffer, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ table1Data1.ptr },
			"TableData of empty table is equal to TableData from pointer to its table encoded buffer, operator =="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1Data1.ptr } != MSAPI::TableData{ table1Data1.ptr }, false,
			"TableData of empty table from pointer to its table encoded buffer is equal to itself, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1Data1.ptr }, MSAPI::TableData{ table1Data1.ptr },
			"TableData of empty table from pointer to its table encoded buffer is equal to itself, operator =="));

		MSAPI::AutoClearPtr<void> table2Data1{ table2.Encode() };
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table2Data1.ptr }, false,
			"TableData of empty table is equal to TableData from pointer to same table's encoded buffer, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ table2Data1.ptr },
			"TableData of empty table is equal to TableData from pointer to same table's encoded buffer, operator =="));

		table1.AddRow(false);
		table2.AddRow(int64_t(0));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table1 }, false,
			"TableData of non empty table is equal to itself, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ table1 },
			"TableData of non empty table is equal to itself, operator =="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table2 }, true,
			"TableData of non empty table is not equal to different non empty TableData, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } == MSAPI::TableData{ table2 }, false,
			"TableData of non empty table is not equal to different non empty TableData, operator =="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ nullptr }, true,
			"TableData of non empty table is not equal to TableData from nullptr, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } == MSAPI::TableData{ nullptr }, false,
			"TableData of non empty table is not equal to TableData from nullptr, operator =="));

		MSAPI::AutoClearPtr<void> table1Data2{ table1.Encode() };
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table1Data2.ptr }, false,
			"TableData of non empty table is equal to TableData from pointer to its table encoded buffer, operator "
			"!="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 }, MSAPI::TableData{ table1Data2.ptr },
			"TableData of non empty table is equal to TableData from pointer to its table encoded buffer, operator "
			"=="));

		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1Data2.ptr } != MSAPI::TableData{ table1Data2.ptr }, false,
			"TableData of non empty table from pointer to its table encoded buffer is equal to itself, operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1Data2.ptr }, MSAPI::TableData{ table1Data2.ptr },
			"TableData of non empty table from pointer to its table encoded buffer is equal to itself, operator =="));

		MSAPI::AutoClearPtr<void> table2Data2{ table2.Encode() };
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } != MSAPI::TableData{ table2Data2.ptr }, true,
			"TableData of non empty table is not equal to TableData from pointer to different table's encoded buffer, "
			"operator !="));
		RETURN_IF_FALSE(t.Assert(MSAPI::TableData{ table1 } == MSAPI::TableData{ table2Data2.ptr }, false,
			"TableData of non empty table is not equal to TableData from pointer to different table's encoded buffer, "
			"operator =="));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_TABLE_INL