/**************************
 * @file        table.h
 * @version     6.0
 * @date        2024-05-10
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

#ifndef MSAPI_TABLE_H
#define MSAPI_TABLE_H

#include "autoClearPtr.hpp"
#include "cstring"
#include "log.h"
#include "standardType.hpp"
#include <list>
#include <memory>
#include <numeric>
#include <set>
#include <string.h>
#include <variant>

namespace MSAPI {

class TableData;
class JsonNode;

/**************************
 * @brief Pure interface for table objects. The main purpose is to store pointer on table in parameters.
 */
class TableBase {
public:
	/**************************
	 * @brief Column structure for table. Can contain custom metadata and string interpretations if underlying type is
	 * enum. Metadata is in JSON format, but without parent brackets. It is only used in Application class and when
	 * metadata request arrives.
	 *
	 * Metadata fields: name (string). For enum string interpretations: stringInterpretations (object) : { id (string) :
	 * string (string), ... }.
	 *
	 * @todo Remove id, and signal about external string interpretations via metadata.
	 */
	struct Column {
		const size_t id;
		const StandardType::Type type;
		std::string metadata;

		/**************************
		 * @brief Construct a new Column object, empty constructor.
		 *
		 * @param id Column id.
		 * @param type Column type.
		 */
		Column(size_t id, StandardType::Type type);

		/**************************
		 * @brief Default comparison operator.
		 */
		friend bool operator==(const Column& first, const Column& second) = default;
	};

public:
	/**************************
	 * @brief Default destructor.
	 */
	virtual ~TableBase() = default;

	/**************************
	 * @brief Merge additional rows from the buffer into the table. Structure of the buffer should be: [(size_t) buffer
	 * size][(type size) row 1 column 1->n][(type size) row 2 column 1->n][rows...]. All optional types should have
	 * additional bool value before data for checking empty value (true is empty). String type should has additional
	 * size_t value before data for size of string.
	 *
	 * @attention There are no checking of pointer on buffer or it's size or columns compatibility.
	 *
	 * @param buffer Buffer with additional rows.
	 * @param bufferSize Size of buffer.
	 *
	 * @test Has unit tests.
	 */
	virtual void Copy(const TableData& TableData);

	/**************************
	 * @example Table:
	 * {
	 * 		Buffer size: 59
	 * 		Columns:
	 * 		{
	 * 			[0] 111111 Uint64
	 * 			[1] 222222 Bool
	 * 			[2] 333333 Double
	 * 		}
	 * 		Rows:
	 * 		{
	 *			[0, 1] 1999 [1| false [2| 20.3456789100000002
	 *			[0, 2] 1878 [1| true [2| -19.2345678900000010
	 * 			[0, 3] 1757 [1| true [2| 18.1234567800000015
	 * 		}
	 * }
	 *
	 * @test Has unit tests.
	 */
	virtual std::string ToString() const;

	/**************************
	 * @example {"Buffer
	 * size":59,"Columns":[{"id":111111,"type":"OptionalUint64"},{"id":111111,"type":"Uint64"}
	 * ,{"id":222222,"type":"Bool"},{"id":333333,"type":"Double"}],"Rows":[[null,false,
	 * 20.3456789100000002],[1878,true,-19.2345678900000010],[1757,true,18.1234567800000015]]}
	 *
	 * @test Has unit tests.
	 */
	virtual std::string ToJson() const;

	/**************************
	 * @return True if number of rows is zero, otherwise false.
	 *
	 * @test Has unit tests.
	 */
	virtual bool Empty() const noexcept;

	/**************************
	 * @return Readable pointer to list of columns in the table, nullptr if called pure method.
	 *
	 * @test Has unit tests.
	 */
	virtual const std::list<Column>* GetColumns() const noexcept;

	/**************************
	 * @return Size of buffer in bytes.
	 *
	 * @test Has unit tests.
	 */
	virtual size_t GetBufferSize() const noexcept;

	/**************************
	 * @brief  Structure of the buffer will be: [(size_t) buffer size][(type size) row 1 column 1->n][(type size) row 2
	 * column 1->n][rows...]. All optional types will have additional bool value before data for indicate empty value
	 * (true is empty). String type will have additional size_t value before data for size of string.
	 *
	 * @attention Freeing up memory is required after usage.
	 *
	 * @return Pointer to buffer with encoded table or nullptr if cannot allocate memory or pure method is called.
	 *
	 * @test Has unit tests.
	 */
	virtual void* Encode() const;
};

/**************************
 * @brief Object for storing table data buffer and transferring throw network. Table data has two constructors, the
 * difference between them is who own the buffer. If object created from Table (TableBase) directly - then TableData
 * will own the buffer, because real table should be encoded, and clear it in destroying. If object created from buffer
 * - then TableData will be only a view on buffer and will not clear it. The purpose of the second type is to merge data
 * from network into the table.
 */
class TableData {
private:
	//* Not const for enable copy/move semantics.
	//* Shared pointer to allow copy constructor and assignment operator.
	std::shared_ptr<AutoClearPtr<void>> m_ownBuffer{ nullptr };
	void* m_sharedBuffer{ nullptr };
	size_t m_bufferSize{ 8 };

public:
	/**************************
	 * @brief Construct a new Table Data object from Table, empty constructor. Has own buffer and will be cleared
	 * automatically.
	 *
	 * @param table Table with data.
	 *
	 * @test Has unit tests.
	 */
	TableData(const TableBase& table) noexcept;

	/**************************
	 * @brief Construct a new Table Data object with buffer and size, set buffer size if buffer is not nullptr. If
	 * nullptr is provided, the buffer will be address of buffer size. Has shared buffer and will not be cleared
	 * automatically.
	 *
	 * @attention Buffer must contain at least 8 bytes for size of buffer.
	 *
	 * @param buffer Buffer with table data.
	 *
	 * @test Has unit tests.
	 */
	TableData(void* buffer);

	/**************************
	 * @brief Construct a new Table Data object without useful data.
	 *
	 * @test Has unit tests.
	 */
	TableData() = default;

	/**************************
	 * @brief Construct a new Table Data object from ranges of JsonNodes and column types, fill buffer with table data.
	 * Has own buffer and will be cleared automatically. This constructor allows creating transferable object from Json
	 * with table rows if number and types of columns are known.
	 *
	 * @param rows JsonNodes with rows, where each row is an array of values for each column.
	 * @param columnTypes Types of columns.
	 *
	 * @test Has unit tests.
	 */
	TableData(const std::list<JsonNode>& rows, const std::vector<StandardType::Type>& columnTypes);

	/**************************
	 * @return Readable pointer to buffer with table data, nullptr if buffer is empty.
	 *
	 * @test Has unit tests.
	 */
	const void* GetBuffer() const noexcept;

	/**************************
	 * @return Size of buffer with table data.
	 *
	 * @test Has unit tests.
	 */
	size_t GetBufferSize() const noexcept;

	/**************************
	 * @example Encoded table with 34 bytes size
	 *
	 * @test Has unit tests.
	 */
	std::string ToString() const;

	/**************************
	 * @brief Looking up in the buffer and fill the json string with table data.
	 *
	 * @param columnTypes Types of columns.
	 *
	 * @return Json string with table data. Will be empty if buffer is empty or something went wrong.
	 *
	 * @example {"Buffer
	 * size":59,"Rows":[[null,false, 20.3456789100000002],[1878,true,-19.2345678900000010],[1757,true,18.1234567800000015]]}
	 *
	 * @test Has unit tests.
	 */
	std::string LookUpToJson(const std::vector<StandardType::Type>& columnTypes) const;

	/**************************
	 * @return True if memory buffers are equal, false otherwise. Type of buffer is not matter, only size and data.
	 *
	 * @test Has unit tests.
	 */
	friend FORCE_INLINE bool operator==(const TableData& first, const TableData& second)
	{
		if (first.m_bufferSize != second.m_bufferSize) {
			return false;
		}

		const auto* firstBuffer{ first.GetBuffer() };
		const auto* secondBuffer{ second.GetBuffer() };

		if (firstBuffer == secondBuffer) {
			return true;
		}

		if (firstBuffer == nullptr || secondBuffer == nullptr) {
			return false;
		}

		return memcmp(firstBuffer, secondBuffer, first.m_bufferSize) == 0;
	}
};

template <typename T>
concept TableType = requires {
	std::is_same_v<
		std::conditional_t<std::is_enum_v<T>, std::bool_constant<is_integer_type<safe_underlying_type_t<T>> && Enum<T>>,
			std::bool_constant<is_standard_type<T>>>,
		std::bool_constant<true>>;

	std::is_same_v<std::bool_constant<!std::is_same_v<TableData, T>>, std::bool_constant<true>>;
};

/**************************
 * @brief Template class for containing data table-like way with at least one column. It is required to specify the
 * types of columns and if table will be sended throw network, it can be merged correctly only with the same table. Add
 * new rows and update cells are allowed. Buffers size is calculated during adding rows, updating cells or merge action.
 *
 * @attention Table is alive only for one purpose - user friendly manipulation with application' parameters in the best
 * way for developer usage. Access and parsing of data are slower than for raw data and should not be used for
 * another purposes. That is, the best way is to create a child class for specific table and access merged data
 * from its fields directly.
 *
 * @tparam Ts Types of columns. Allowed all standard MSAPI types except MSAPI::TableData: int8_t, int16_t, int32_t,
 * int64_t uint8_t, uint16_t, uint32_t, uint64_t, double, float, bool, std::optional<int8_t>, std::optional<int16_t>,
 * std::optional<int32_t>, std::optional<int64_t>, std::optional<uint8_t>, std::optional<uint16_t>,
 * std::optional<uint32_t>, std::optional<uint64_t>, std::optional<double>, std::optional<float>, std::string,
 * MSAPI::Timer, MSAPI::Timer::Duration. Also enum types are allowed.
 */
template <TableType... Ts>
	requires(sizeof...(Ts) > 0)
class Table : public TableBase {
private:
	using UniqueTypes = Unique_t<Ts...>;
	using MultimapTypes = TransformPack_t<UniqueTypes>;
	using VariantType = tuple_to_variant<MultimapTypes>;

	std::vector<VariantType> m_data;
	size_t m_bufferSize{ sizeof(size_t) };
	std::list<Column> m_columns;
	size_t m_rows{ 0 };

private:
	/**************************
	 * @brief Parse types of columns and their IDs during creation of table and add them to the column list.
	 *
	 * @tparam Fs Types of table's columns.
	 *
	 * @param ids IDs of table's columns.
	 *
	 * @test Has unit tests.
	 */
	template <typename... Fs> void AddColumns(std::vector<size_t>&& ids)
	{
		size_t index{ 0 };
		(([&index, &ids, this](const auto& t) {
			using T = safe_underlying_type_t<std::decay_t<decltype(t)>>;
			m_data.emplace_back(std::map<size_t, std::decay_t<decltype(t)>>{});
			if constexpr (std::is_same_v<T, int8_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Int8);
			}
			else if constexpr (std::is_same_v<T, int16_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Int16);
			}
			else if constexpr (std::is_same_v<T, int32_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Int32);
			}
			else if constexpr (std::is_same_v<T, int64_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Int64);
			}
			else if constexpr (std::is_same_v<T, uint8_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Uint8);
			}
			else if constexpr (std::is_same_v<T, uint16_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Uint16);
			}
			else if constexpr (std::is_same_v<T, uint32_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Uint32);
			}
			else if constexpr (std::is_same_v<T, uint64_t>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Uint64);
			}
			else if constexpr (std::is_same_v<T, double>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Double);
			}
			else if constexpr (std::is_same_v<T, float>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Float);
			}
			else if constexpr (std::is_same_v<T, bool>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Bool);
			}
			else if constexpr (std::is_same_v<T, std::optional<int8_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalInt8);
			}
			else if constexpr (std::is_same_v<T, std::optional<int16_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalInt16);
			}
			else if constexpr (std::is_same_v<T, std::optional<int32_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalInt32);
			}
			else if constexpr (std::is_same_v<T, std::optional<int64_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalInt64);
			}
			else if constexpr (std::is_same_v<T, std::optional<uint8_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalUint8);
			}
			else if constexpr (std::is_same_v<T, std::optional<uint16_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalUint16);
			}
			else if constexpr (std::is_same_v<T, std::optional<uint32_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalUint32);
			}
			else if constexpr (std::is_same_v<T, std::optional<uint64_t>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalUint64);
			}
			else if constexpr (std::is_same_v<T, std::optional<double>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalDouble);
			}
			else if constexpr (std::is_same_v<T, std::optional<float>>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::OptionalFloat);
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::String);
			}
			else if constexpr (std::is_same_v<T, Timer>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Timer);
			}
			else if constexpr (std::is_same_v<T, Timer::Duration>) {
				m_columns.emplace_back(ids[index++], StandardType::Type::Duration);
			}
			else {
				static_assert(sizeof(T) + 1 == 0, "Unsupported type for table's colum");
			}
		}(Fs{})),
			...);
	}

public:
	/**************************
	 * @brief Construct a new Table object, connect types of columns and their range generated IDs.
	 *
	 * @test Has unit tests.
	 */
	Table()
	{
		std::vector<size_t> ids(sizeof...(Ts));
		std::iota(ids.begin(), ids.end(), 0);
		AddColumns<Ts...>(std::move(ids));
		LOG_DEBUG("Created with range ids " + ToString());
	}

	/**************************
	 * @brief Construct a new Table object, connect types of columns and their IDs. All columns should have unique id.
	 *
	 * @param ids IDs of table's columns.
	 *
	 * @test Has unit tests.
	 */
	Table(const std::initializer_list<size_t> ids)
	{
		if (sizeof...(Ts) != ids.size()) {
			LOG_ERROR("Process of creating table is interrupted. Unexpected size of IDs. Expected: " + _S(sizeof...(Ts))
				+ ", actual: " + _S(ids.size()));
			return;
		}
		if (std::set<size_t> uniqueIds(ids.begin(), ids.end()); uniqueIds.size() != ids.size()) {
			auto begin{ ids.begin() };
			std::string idsStr{ _S(*begin) };
			while (++begin != ids.end()) {
				idsStr += ", " + _S(*begin);
			}
			LOG_ERROR("Process of creating table is interrupted. Column IDs are not unique: " + idsStr);
			return;
		}

		AddColumns<Ts...>(ids);
		LOG_DEBUG("Created " + ToString());
	}

	/**************************
	 * @brief Construct a new Table object, connect types of columns and their IDs. All columns should have unique id.
	 *
	 * @param ids IDs of table's columns.
	 *
	 * @test Has unit tests.
	 */
	Table(std::vector<size_t>&& ids)
	{
		if (sizeof...(Ts) != ids.size()) {
			LOG_ERROR("Process of creating table is interrupted. Unexpected size of IDs. Expected: " + _S(sizeof...(Ts))
				+ ", actual: " + _S(ids.size()));
			return;
		}
		if (std::set<size_t> uniqueIds(ids.begin(), ids.end()); uniqueIds.size() != ids.size()) {
			auto begin{ ids.begin() };
			std::string idsStr{ _S(*begin) };
			while (++begin != ids.end()) {
				idsStr += ", " + _S(*begin);
			}
			LOG_ERROR("Process of creating table is interrupted. Column IDs are not unique: " + idsStr);
			return;
		}

		AddColumns<Ts...>(std::move(ids));
		LOG_DEBUG("Created " + ToString());
	}

	/**************************
	 * @brief Add a new row in the table. Data should contain the same number of elements with the same types as
	 * in columns in the order.
	 *
	 * @tparam Fs Types of cells.
	 *
	 * @param cells Cells for row.
	 *
	 * @test Has unit tests.
	 */
	template <typename... Fs>
		requires(sizeof...(Fs) == sizeof...(Ts)
			&& std::conjunction_v<std::is_same<std::decay_t<Fs>,
				std::conditional_t<std::is_same_v<Ts, std::string>,
					std::conditional_t<std::is_same_v<std::decay_t<Fs>, std::string>, std::decay_t<Fs>, const char*>,
					Ts>>...>)
	void AddRow(Fs&&... cells)
	{
		std::vector<tuple_to_variant<UniqueTypes>> data = { std::forward<Fs>(cells)... };
		if (data.size() == m_columns.size()) {
			for (size_t index{ 0 }; auto& value : data) {
				std::visit(
					[index, &value, this](auto& map) {
						std::visit(
							[index, &map, this](auto&& arg) {
								using T = std::decay_t<decltype(map)>::value_type::second_type;
								using S = std::decay_t<decltype(arg)>;
								if constexpr (std::is_same_v<T, S>) {
									if constexpr (is_standard_primitive_type<safe_underlying_type_t<T>>
										|| std::is_same_v<T, Timer> || std::is_same_v<T, Timer::Duration>) {

										map.emplace(m_rows, arg);
										m_bufferSize += sizeof(T);
									}
									else if constexpr (is_standard_primitive_type_optional<T>) {
										using N = remove_optional_t<T>;
										if (arg.has_value()) {
											map.emplace(m_rows, arg);
											m_bufferSize += sizeof(bool) + sizeof(N);
										}
										else {
											map.emplace(m_rows, arg);
											m_bufferSize += sizeof(bool);
										}
									}
									else if constexpr (std::is_same_v<T, std::string>) {
										m_bufferSize += map.emplace(m_rows, arg).first->second.size() + sizeof(size_t);
									}
									else {
										static_assert(sizeof(T) + 1 == 0, "Unsupported type for table's row");
									}

									return;
								}

								LOG_ERROR("Process of adding a row is encountered an error. Cell [" + _S(index) + ", "
									+ _S(m_rows) + "] has incorrect type");
							},
							std::move(value));
					},
					m_data[index]);
				++index;
			}
			++m_rows;

			return;
		}

		LOG_ERROR("Process of adding a row is interrupted. Unexpected size of data. Expected: " + _S(m_columns.size())
			+ ", actual: " + _S(data.size()));
	}

	/**************************
	 * @brief Update cell in the table. Data should contain the same type as column.
	 *
	 * @tparam T Type of update.
	 *
	 * @param column Column index, starts from 0.
	 * @param row Row index, starts from 0.
	 * @param update Update for cell.
	 *
	 * @test Has unit tests.
	 */
	template <typename T>
		requires is_included_in<T, Ts...>
	void UpdateCell(const size_t column, const size_t row, const T& update)
	{
		if (column < m_columns.size()) {
			std::visit(
				[column, row, &update, this](auto& map) {
					using S = std::decay_t<decltype(map)>::value_type::second_type;
					if constexpr (std::is_same_v<T, S>) {
						const auto it{ map.find(row) };
						if (it == map.end()) {
							LOG_ERROR("Process of updating cell is interrupted. Cell [" + _S(column) + ", " + _S(row)
								+ " (wrong)] does not exist");
							return;
						}

						if constexpr (is_standard_primitive_type<safe_underlying_type_t<T>> || std::is_same_v<T, Timer>
							|| std::is_same_v<T, Timer::Duration>) {

							//* skip
						}
						else if constexpr (is_standard_primitive_type_optional<T>) {
							if (update.has_value()) {
								if (!it->second.has_value()) {
									m_bufferSize += sizeof(T);
								}
							}
							else {
								if (it->second.has_value()) {
									m_bufferSize -= sizeof(T);
								}
							}
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							m_bufferSize += (update.size() - it->second.size());
						}
						else {
							static_assert(sizeof(T) + 1 == 0, "Unsupported type for table's cell");
						}

						it->second = update;
						return;
					}

					LOG_ERROR(
						"Update for cell [" + _S(column) + ", " + _S(row) + "] has incorrect type, update is skipped");
				},
				m_data[column]);

			return;
		}

		LOG_ERROR("Process of updating cell is interrupted. Unexpected column index: " + _S(column));
	}

	/**************************
	 * @brief Make the same Table as in TableData's buffer. Structure of the buffer should be: [(size_t) buffer
	 * size][(type size) row 1 column 1->n][(type size) row 2 column 1->n][rows...]. All optional types should have
	 * additional bool value before data for checking empty value (true is empty). String type should has additional
	 * size_t value before data for size of string.
	 *
	 * @attention There are no checking of pointer on buffer or it's size or columns compatibility.
	 *
	 * @param buffer Buffer with additional rows.
	 * @param bufferSize Size of buffer.
	 *
	 * @test Has unit tests.
	 */
	void Copy(const TableData& tableData) override
	{
		if (m_rows != 0) {
			Clear();
		}
		const void* buffer{ tableData.GetBuffer() };
		const size_t bufferSize{ *reinterpret_cast<const size_t*>(buffer) };
		size_t offset{ sizeof(size_t) };
		while (offset < bufferSize) {
			for (size_t index{ 0 }; const auto& [id, type, metadata] : m_columns) {
				switch (type) {
				case StandardType::Type::Int8:

#define TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(_type)                                                                      \
	std::visit(                                                                                                        \
		[index, &buffer, offset, type, this](auto& map) {                                                              \
			using T = std::decay_t<decltype(map)>::value_type::second_type;                                            \
			using U = safe_underlying_type_t<T>;                                                                       \
			if constexpr (std::is_same_v<U, _type>) {                                                                  \
				map.emplace(m_rows,                                                                                    \
					*reinterpret_cast<const remove_optional_t<T>*>(&reinterpret_cast<const char*>(buffer)[offset]));   \
				return;                                                                                                \
			}                                                                                                          \
			LOG_ERROR_NEW("Merging a cell in [{}, {}] encounters an error, column does not contain {} type", index,    \
				m_rows, StandardType::EnumToString(type));                                                             \
		},                                                                                                             \
		m_data[index]);                                                                                                \
	offset += sizeof(remove_optional_t<_type>);

					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(int8_t);
					break;
				case StandardType::Type::Int16:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(int16_t);
					break;
				case StandardType::Type::Int32:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(int32_t);
					break;
				case StandardType::Type::Int64:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(int64_t);
					break;
				case StandardType::Type::Uint8:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(uint8_t);
					break;
				case StandardType::Type::Uint16:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(uint16_t);
					break;
				case StandardType::Type::Uint32:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(uint32_t);
					break;
				case StandardType::Type::Uint64:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(uint64_t);
					break;
				case StandardType::Type::Double:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(double);
					break;
				case StandardType::Type::Float:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(float);
					break;
				case StandardType::Type::Bool:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(bool);
					break;
				case StandardType::Type::OptionalInt8:

#define TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(_type)                                                          \
	if (*reinterpret_cast<const bool*>(&reinterpret_cast<const char*>(buffer)[offset])) {                              \
		std::visit(                                                                                                    \
			[index, &buffer, offset, type, this](auto& map) {                                                          \
				using T = std::decay_t<decltype(map)>::value_type::second_type;                                        \
				if constexpr (std::is_same_v<T, _type>) {                                                              \
					map.emplace(m_rows, _type{});                                                                      \
					return;                                                                                            \
				}                                                                                                      \
				LOG_ERROR_NEW("Merging a cell in [{}, {}] encounters an error, column does not contain {} type",       \
					index, m_rows, StandardType::EnumToString(type));                                                  \
			},                                                                                                         \
			m_data[index]);                                                                                            \
		offset += sizeof(bool);                                                                                        \
		break;                                                                                                         \
	}                                                                                                                  \
	offset += sizeof(bool);

					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<int8_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<int8_t>);
					break;
				case StandardType::Type::OptionalInt16:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<int16_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<int16_t>);
					break;
				case StandardType::Type::OptionalInt32:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<int32_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<int32_t>);
					break;
				case StandardType::Type::OptionalInt64:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<int64_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<int64_t>);
					break;
				case StandardType::Type::OptionalUint8:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<uint8_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<uint8_t>);
					break;
				case StandardType::Type::OptionalUint16:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<uint16_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<uint16_t>);
					break;
				case StandardType::Type::OptionalUint32:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<uint32_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<uint32_t>);
					break;
				case StandardType::Type::OptionalUint64:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<uint64_t>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<uint64_t>);
					break;
				case StandardType::Type::OptionalDouble:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<double>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<double>);
					break;
				case StandardType::Type::OptionalFloat:
					TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY(std::optional<float>);
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(std::optional<float>);
					break;
				case StandardType::Type::String: {
					size_t size;
					memcpy(&size, &reinterpret_cast<const char*>(buffer)[offset], sizeof(size_t));
					offset += sizeof(size_t);
					if (size == 0) {
						std::visit(
							[index, this](auto& map) {
								using T = std::decay_t<decltype(map)>::value_type::second_type;
								if constexpr (std::is_same_v<T, std::string>) {
									map.emplace(m_rows, std::string{});
									return;
								}
								LOG_ERROR("Merge of cell in [" + _S(index) + ", " + _S(m_rows)
									+ "] is encountered an error, column does not contain String type");
							},
							m_data[index]);
						break;
					}
					std::visit(
						[index, size, &buffer, offset, this](auto& map) {
							using T = std::decay_t<decltype(map)>::value_type::second_type;
							if constexpr (std::is_same_v<T, std::string>) {
								map.emplace(
									m_rows, std::string{ &reinterpret_cast<const char*>(buffer)[offset], size });
								return;
							}
							LOG_ERROR("Merge of cell in [" + _S(index) + ", " + _S(m_rows)
								+ "] is encountered an error, column does not contain String type");
						},
						m_data[index]);
					offset += size;
				} break;
				case StandardType::Type::Timer:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(Timer);
					break;
				case StandardType::Type::Duration:
					TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE(Timer::Duration);

#undef TMP_MSAPI_TABLE_EMPLACE_TYPE_VALUE
#undef TMP_MSAPI_TABLE_EMPLACE_OPTIONAL_TYPE_IF_EMPTY

					break;
				default:
					LOG_ERROR_NEW("Unexpected column type for table: {}", StandardType::EnumToString(type));
					return;
				}
				++index;
			}
			++m_rows;
		}

		m_bufferSize += offset - sizeof(size_t);
	}

	/**************************
	 * @example Table:
	 * {
	 * 		Buffer size: 59
	 * 		Columns:
	 * 		{
	 * 			[0] 111111 unsigned long
	 * 			[1] 222222 bool
	 * 			[2] 333333 double
	 * 		}
	 * 		Rows:
	 * 		{
	 *			[0, 1] 1999 [1| false [2| 20.3456789100000002
	 *			[0, 2] 1878 [1| true [2| -19.2345678900000010
	 * 			[0, 3] 1757 [1| true [2| 18.1234567800000015
	 * 		}
	 * }
	 *
	 * @test Has unit tests.
	 */
	std::string ToString() const override
	{
		std::string result;
		result.reserve(256);
		BI(result, "Table:\n{{\n\tBuffer size: {}\n\tColumns:\n\t{{", m_bufferSize);
		for (size_t column{ 0 }; const auto& [id, type, metadata] : m_columns) {
			result += std::format("\n\t\t[{}] {} {}", column++, id, StandardType::EnumToString(type));
		}

		if (m_rows == 0) {
			return result + "\n\t}\n}";
		}

		result += "\n\t}\n\tRows:\n\t{";
		for (size_t row{ 0 }; row < m_rows; ++row) {
			for (size_t column{ 0 }; const auto& [id, type, metadata] : m_columns) {
				if (column == 0) [[unlikely]] {
					result += std::format("\n\t\t[{}, {}] ", column, row);
				}
				else {
					result += std::format(" [{}| ", column);
				}

				std::visit(
					[row, column, id, type, &result](const auto& map) {
						const auto it{ map.find(row) };
						if (it == map.end()) {
							result += "Error: Cell does not exist, nothing to print";
							LOG_ERROR_NEW("Cell [{}, {}] {} {} does not exist", column, row, id,
								StandardType::EnumToString(type));
							return;
						}

						using T = safe_underlying_type_t<typename std::decay_t<decltype(map)>::value_type::second_type>;
						if constexpr (is_integer_type<T>) {
							result += std::format("{}", _S(static_cast<T>(it->second)));
						}
						else if constexpr (std::is_same_v<T, bool>) {
							result += _S(it->second);
						}
						else if constexpr (is_integer_type_optional<T>) {
							if (it->second.has_value()) {
								result += std::format("{}", it->second.value());
							}
						}
						else if constexpr (is_float_type<T>) {
							result += _S(it->second);
						}
						else if constexpr (is_float_type_optional<T>) {
							if (it->second.has_value()) {
								result += _S(it->second.value());
							}
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							result += it->second;
						}
						else if constexpr (std::is_same_v<T, Timer> || std::is_same_v<T, Timer::Duration>) {
							result += it->second.ToString();
						}
						else {
							static_assert(sizeof(T) + 1 == 0, "Unsupported type for table cell");
						}
					},
					m_data[column]);
				++column;
			}
		}

		return result + "\n\t}\n}";
	}

	/**************************
	 * @example {"Buffer
	 * size":59,"Columns":[{"id":111111,"type":"std::optional<Uint64"},{"id":111111,"type":"Uint64"}
	 * ,{"id":222222,"type":"Bool"},{"id":333333,"type":"Double"}],"Rows":[[null,false,
	 * 20.3456789100000002],[1878,true,-19.2345678900000010],[1757,true,18.1234567800000015]]}
	 *
	 * @test Has unit tests.
	 */
	std::string ToJson() const override
	{
		std::string columns{ "{\"Buffer size\":" + _S(m_bufferSize) + ",\"Columns\":[" };
		auto begin{ m_columns.begin() };
		columns += std::format("{{\"id\":{},\"type\":\"{}\"}}", begin->id, StandardType::EnumToString(begin->type));
		++begin;
		for (auto end{ m_columns.end() }; begin != end; ++begin) {
			columns
				+= std::format(",{{\"id\":{},\"type\":\"{}\"}}", begin->id, StandardType::EnumToString(begin->type));
		}
		columns += "]";

		if (m_rows == 0) {
			return columns + ",\"Rows\":[]}";
		}

		std::string data{ ",\"Rows\":[" };
		for (size_t row{ 0 }; row < m_rows; ++row) {
			if (row == 0) [[likely]] {
				data += "[";
			}
			else {
				data += "],[";
			}

			for (size_t column{ 0 }; const auto& [id, type, metadata] : m_columns) {
				if (column != 0) [[likely]] {
					data += ",";
				}
				std::visit(
					[row, column, id, type, &data](const auto& map) {
						using T = safe_underlying_type_t<typename std::decay_t<decltype(map)>::value_type::second_type>;
						const auto it{ map.find(row) };
						if (it == map.end()) {
							data += "\"Error: Cell does not exist, nothing to print\"";
							LOG_ERROR_NEW("Cell [{}, {}] {} {} does not exist", column, row, id,
								StandardType::EnumToString(type));
							return;
						}

						if constexpr (is_integer_type<T>) {
							data += std::format("{}", _S(static_cast<T>(it->second)));
						}
						else if constexpr (std::is_same_v<T, bool>) {
							data += _S(it->second);
						}
						else if constexpr (is_integer_type_optional<T>) {
							if (it->second.has_value()) {
								data += std::format("{}", it->second.value());
							}
							else {
								data += "null";
							}
						}
						else if constexpr (is_float_type<T>) {
							data += _S(it->second);
						}
						else if constexpr (is_float_type_optional<T>) {
							if (it->second.has_value()) {
								data += _S(it->second.value());
							}
							else {
								data += "null";
							}
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							data += "\"" + it->second + "\"";
						}
						else if constexpr (std::is_same_v<T, Timer> || std::is_same_v<T, Timer::Duration>) {
							data += "\"" + it->second.ToString() + "\"";
						}
						else {
							static_assert(sizeof(T) + 1 == 0, "Unsupported type for table cell");
						}
					},
					m_data[column]);
				++column;
			}
		}

		return columns + data + "]]}";
	}

	/**************************
	 * @return True if number of rows is zero, otherwise false.
	 *
	 * @test Has unit tests.
	 */
	bool Empty() const noexcept final { return m_rows == 0; }

	/**************************
	 * @return Number of rows in the table.
	 *
	 * @test Has unit tests.
	 */
	size_t GetRowsSize() const noexcept { return m_rows; }

	/**************************
	 * @return Number of columns in the table.
	 *
	 * @test Has unit tests.
	 */
	size_t GetColumnsSize() const noexcept { return m_columns.size(); }

	/**************************
	 * @return Readable pointer to list of columns in the table, nullptr if called pure method.
	 *
	 * @test Has unit tests.
	 */
	const std::list<Column>* GetColumns() const noexcept final { return &m_columns; }

	/**************************
	 * @attention Type of cell should be known in the place of call.
	 *
	 * @param column Column index.
	 * @param row Row index.
	 *
	 * @return Readable pointer on the cell in the table or nullptr if cell does not exist.
	 *
	 * @test Has unit tests.
	 */
	const void* GetCell(const size_t column, const size_t row) const
	{
		if (column >= m_columns.size() || row >= m_rows) {
			return nullptr;
		}

		return std::visit(
			[row](const auto& map) -> const void* {
				if (const auto it{ map.find(row) }; it != map.end()) {
					return &it->second;
				}
				return nullptr;
			},
			m_data[column]);
	}

	/**************************
	 * @brief Clear all rows and buffer size.
	 *
	 * @test Has unit tests.
	 */
	void Clear()
	{
		m_rows = 0;
		m_bufferSize = sizeof(size_t);
		for (auto& data : m_data) {
			std::visit([](auto& map) { map.clear(); }, data);
		}
	}

	/**************************
	 * @return Size of buffer in bytes.
	 *
	 * @test Has unit tests.
	 */
	size_t GetBufferSize() const noexcept final { return m_bufferSize; }

	/**************************
	 * @brief  Structure of the buffer will be: [(size_t) buffer size][(type size) row 1 column 1->n][(type size) row 2
	 * column 1->n][rows...]. All optional types will have additional bool value before data for indicate empty value
	 * (true is empty). String type will have additional size_t value before data for size of string.
	 *
	 * @attention Freeing up memory is required after usage.
	 *
	 * @return Pointer to buffer with encoded table or nullptr if cannot allocate memory or pure method is called.
	 *
	 * @test Has unit tests.
	 */
	void* Encode() const final
	{
		if (m_bufferSize == 0) [[unlikely]] {
			LOG_ERROR("Process of encoding table encountered an error. Buffer size is zero");
			void* buffer{ malloc(sizeof(size_t)) };
			static_cast<size_t*>(buffer)[0] = sizeof(size_t);
			return buffer;
		}

		void* buffer{ malloc(m_bufferSize) };
		if (buffer == nullptr) [[unlikely]] {
			LOG_ERROR("Cannot allocate memory for encoding table. Error №" + _S(errno) + ": " + std::strerror(errno));
			return nullptr;
		}

		if (m_rows == 0) {
			if (m_bufferSize != sizeof(size_t)) [[unlikely]] {
				LOG_ERROR(
					"Process of encoding table encountered an error. Buffer size is not equal to calculated size");
			}

			memcpy(buffer, &m_bufferSize, sizeof(size_t));
			return buffer;
		}

		memcpy(buffer, &m_bufferSize, sizeof(size_t));
		size_t offset{ sizeof(size_t) };

		for (size_t row{ 0 }; row < m_rows; ++row) {
			for (size_t column{ 0 }, columns{ m_columns.size() }; column < columns; ++column) {
				std::visit(
					[column, row, &buffer, &offset](const auto& map) {
						const auto it{ map.find(row) };
						if (it == map.end()) {
							LOG_ERROR("Process of encoding table is interrupted. Cell [" + _S(column) + ", " + _S(row)
								+ " (wrong)] does not exist");
							return;
						}

						using T = std::decay_t<decltype(map)>::value_type::second_type;
						if constexpr (is_standard_primitive_type<safe_underlying_type_t<T>> || std::is_same_v<T, Timer>
							|| std::is_same_v<T, Timer::Duration>) {

							memcpy(&reinterpret_cast<char*>(buffer)[offset], &it->second, sizeof(T));
							offset += sizeof(T);
						}
						else if constexpr (is_standard_primitive_type_optional<T>) {
							using S = remove_optional_t<T>;
							if (it->second.has_value()) {
								memset(&reinterpret_cast<char*>(buffer)[offset], 0, sizeof(bool));
								offset += sizeof(bool);
								memcpy(&reinterpret_cast<char*>(buffer)[offset], &it->second.value(), sizeof(S));
								offset += sizeof(S);
							}
							else {
								memset(&reinterpret_cast<char*>(buffer)[offset], 1, sizeof(bool));
								offset += sizeof(bool);
							}
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							if (it->second.empty()) {
								memset(&reinterpret_cast<char*>(buffer)[offset], 0, sizeof(size_t));
								offset += sizeof(size_t);
							}
							else {
								const size_t size{ it->second.size() };
								memcpy(&reinterpret_cast<char*>(buffer)[offset], &size, sizeof(size_t));
								offset += sizeof(size_t);
								memcpy(&reinterpret_cast<char*>(buffer)[offset], it->second.data(), size);
								offset += size;
							}
						}
						else {
							static_assert(sizeof(T) + 1 == 0, "Unsupported type for table's cell");
						}
					},
					m_data[column]);
			}
		}

		if (offset != m_bufferSize) [[unlikely]] {
			LOG_ERROR("Process of encoding table encountered an error. Buffer size is not equal to calculated size. "
					  "Precalculated buffer size: "
				+ _S(m_bufferSize) + ", actual buffer size: " + _S(offset));

			memset(buffer, 0, m_bufferSize);
			memcpy(buffer, &m_bufferSize, sizeof(size_t));
			return buffer;
		}

		return buffer;
	}

	/**************************
	 * @brief Collection of names for each column and in the same order. Metadata is only used in Application class and
	 * when metadata request arrives.
	 *
	 * @param names Column names.
	 *
	 * @test Has unit tests.
	 */
	template <typename... Fs>
		requires(sizeof...(Fs) == sizeof...(Ts) && (std::is_same_v<std::decay_t<Fs>, const char*> && ...))
	void SetColumnNames(Fs&&... names)
	{
		auto begin = m_columns.begin();
		(void)std::initializer_list<int>{ (begin->metadata += begin->metadata.empty() ? "" : ",",
			begin->metadata += "\"name\":\"" + std::string{ names } + "\"", ++begin, 0)... };
	}

	/**************************
	 * @brief Add string interpretations for each column with T type.
	 * @brief Format: stringInterpretations (object) : { id (string) : string (string), ... }.
	 *
	 * @tparam T type of enum.
	 *
	 * @param printFunc Enum to string function.
	 *
	 * @test Has unit tests.
	 */
	template <Enum T> void AddMetadataForEnum(std::string_view (*printFunc)(T))
	{
		auto columnIt{ m_columns.begin() };
		AddMetadataForEnumImpl<T, Ts...>(columnIt, printFunc);
	}

	/**************************
	 * @brief Set custom string as a column metadata.
	 *
	 * @param columnIndex Index of a column since 0.
	 * @param metadata String with custom metadata in format "key":"value",....
	 *
	 * @test Has unit tests.
	 */
	void SetColumnMetadata(uint64_t columnIndex, const std::string metadata)
	{
		if (m_columns.size() <= columnIndex) {
			LOG_ERROR("Index of column (" + _S(columnIndex) + ") to set metadata is out of boundaries ("
				+ _S(m_columns.size()) + ")");
			return;
		}

		auto columnIt{ m_columns.begin() };
		while (columnIndex--) {
			++columnIt;
		}

		if (columnIt->metadata.empty()) {
			columnIt->metadata = metadata;
			return;
		}

		columnIt->metadata += "," + metadata;
	}

	/**************************
	 * @brief Compare by buffer size, number of rows, number of columns and data.
	 *
	 * @test Has unit tests.
	 */
	friend bool operator==(const Table& first, const Table& second) noexcept
	{
		return first.m_bufferSize == second.m_bufferSize && first.m_rows == second.m_rows
			&& first.m_columns == second.m_columns && first.m_data == second.m_data;
	}

private:
	/**************************
	 * @brief Implementation of extending metadata for enum function.
	 * @brief Format: stringInterpretations (object) : { id (string) : string (string), ... }.
	 *
	 * @tparam T type of enum.
	 * @tparam N Type under checking.
	 * @tparam Ns Types queue.
	 *
	 * @param columnIt Colum iterator for type under checking.
	 * @param printFunc Enum to string function.
	 */
	template <Enum T, typename N, typename... Ns>
	static constexpr void AddMetadataForEnumImpl(
		std::list<MSAPI::TableBase::Column>::iterator& columnIt, std::string_view (*printFunc)(T))
	{
		if constexpr (std::is_same_v<T, N>) {
			if (!columnIt->metadata.empty()) {
				columnIt->metadata.push_back(',');
			}

			columnIt->metadata += "\"stringInterpretations\":{";
			for (auto index{ U(T::Undefined) }; index < U(T::Max); ++index) {
				columnIt->metadata += std::format("\"{}\":\"{}\",", index, printFunc(static_cast<T>(index)));
			}
			columnIt->metadata[columnIt->metadata.size() - 1] = '}';
		}

		if constexpr (sizeof...(Ns) > 0) {
			AddMetadataForEnumImpl<T, Ns...>(++columnIt, printFunc);
		}
	}
};

}; //* namespace MSAPI

#endif //* MSAPI_TABLE_H