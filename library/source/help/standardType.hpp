/**************************
 * @file        standardType.hpp
 * @version     6.0
 * @date        2024-05-19
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

#ifndef MSAPI_STANDARD_TYPE_H
#define MSAPI_STANDARD_TYPE_H

#include "log.h"

namespace MSAPI {

/**************************
 * @brief Standard types for MSAPI, they are can be send by standard protocol, used in parameters and added in table
 * (except TableData). Expected to be run at least on 64-bit architecture, where float and double precision float are 4
 * bytes and 8 bytes accordingly.
 *
 * @attention 80-bit FPU is not supported, due to it's highly sensetive to aligiment during accessing. It is
 * scientific-purpose type and its supporting should be strongly verified.
 */
class StandardType {
public:
	enum class Type : int8_t {
		Undefined,
		Int8,
		Int16,
		Int32,
		Int64,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Float,
		Double,
		Bool,
		OptionalInt8,
		OptionalInt16,
		OptionalInt32,
		OptionalInt64,
		OptionalUint8,
		OptionalUint16,
		OptionalUint32,
		OptionalUint64,
		OptionalInt8Empty,
		OptionalInt16Empty,
		OptionalInt32Empty,
		OptionalInt64Empty,
		OptionalUint8Empty,
		OptionalUint16Empty,
		OptionalUint32Empty,
		OptionalUint64Empty,
		OptionalFloat,
		OptionalDouble,
		OptionalFloatEmpty,
		OptionalDoubleEmpty,
		String,
		StringEmpty,
		Timer,
		Duration,
		TableData,
		Max
	};

	/**************************
	 * @return Reinterpretation of Type enum to string.
	 */
	static FORCE_INLINE std::string_view EnumToString(const Type type)
	{
		static_assert(U(Type::Max) == 37, "Type::Max has been changed, update EnumToString");

		switch (type) {
		case Type::Undefined:
			return "Undefined";
		case Type::Int8:
			return "Int8";
		case Type::Int16:
			return "Int16";
		case Type::Int32:
			return "Int32";
		case Type::Int64:
			return "Int64";
		case Type::Uint8:
			return "Uint8";
		case Type::Uint16:
			return "Uint16";
		case Type::Uint32:
			return "Uint32";
		case Type::Uint64:
			return "Uint64";
		case Type::Float:
			return "Float";
		case Type::Double:
			return "Double";
		case Type::Bool:
			return "Bool";
		case Type::OptionalInt8:
			return "OptionalInt8";
		case Type::OptionalInt16:
			return "OptionalInt16";
		case Type::OptionalInt32:
			return "OptionalInt32";
		case Type::OptionalInt64:
			return "OptionalInt64";
		case Type::OptionalUint8:
			return "OptionalUint8";
		case Type::OptionalUint16:
			return "OptionalUint16";
		case Type::OptionalUint32:
			return "OptionalUint32";
		case Type::OptionalUint64:
			return "OptionalUint64";
		case Type::OptionalInt8Empty:
			return "OptionalInt8Empty";
		case Type::OptionalInt16Empty:
			return "OptionalInt16Empty";
		case Type::OptionalInt32Empty:
			return "OptionalInt32Empty";
		case Type::OptionalInt64Empty:
			return "OptionalInt64Empty";
		case Type::OptionalUint8Empty:
			return "OptionalUint8Empty";
		case Type::OptionalUint16Empty:
			return "OptionalUint16Empty";
		case Type::OptionalUint32Empty:
			return "OptionalUint32Empty";
		case Type::OptionalUint64Empty:
			return "OptionalUint64Empty";
		case Type::OptionalFloat:
			return "OptionalFloat";
		case Type::OptionalDouble:
			return "OptionalDouble";
		case Type::OptionalFloatEmpty:
			return "OptionalFloatEmpty";
		case Type::OptionalDoubleEmpty:
			return "OptionalDoubleEmpty";
		case Type::String:
			return "String";
		case Type::StringEmpty:
			return "StringEmpty";
		case Type::Timer:
			return "Timer";
		case Type::Duration:
			return "Duration";
		case Type::TableData:
			return "TableData";
		case Type::Max:
			return "Max";
		default:
			LOG_ERROR("Unknown type: " + _S(U(type)));
			return "Unknown";
		}
	}
};

} //* namespace MSAPI

#endif //* MSAPI_STANDARD_TYPE_H