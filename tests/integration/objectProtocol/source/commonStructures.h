/**************************
 * @file        commonStructures.h
 * @version     6.0
 * @date        2023-12-16
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
 *
 * @brief Common structures for MSAPI tests of object protocol.
 */

#ifndef COMMON_STRUCTURES_H
#define COMMON_STRUCTURES_H

#include "../../../../library/source/help/log.h"

struct FilterStructure {
	size_t figi;
};

struct InstrumentStructure {
	enum class InstrumentStructureType : short { Undefined, First, Second, Third, Fourth, Max };

	struct Nominal {
		size_t currency{ 0 };
		size_t value{ 0 };

		Nominal(const size_t currency, const size_t value)
			: currency(currency)
			, value(value)
		{
		}

		std::string ToString() const
		{
			std::stringstream stream;
			stream << currency << "(" << value << ")";
			return stream.str();
		}
	};

	const InstrumentStructureType type;
	const size_t figi;
	const size_t ticker;
	const size_t classCode;
	const size_t isin;
	const int32_t lotSize;
	const size_t currency;
	const Nominal nominal;
	bool buyAvailable{ false };
	bool sellAvailable{ false };
	bool limitOrderAvailable{ true };
	bool marketOrderAvailable{ true };
	bool APITradeAvailable{ false };
	const size_t isoCurrencyName;
	double tick{ 0.0 };
	const size_t uid;
	int32_t requiredLotMultiplier{ 0 };

	InstrumentStructure(const InstrumentStructureType type, const size_t figi, const size_t ticker,
		const size_t classCode, const size_t isin, const int32_t lotSize, const size_t currency, const Nominal& nominal,
		const bool buyAvailable, const bool sellAvailable, const bool APITradeAvailable, const size_t isoCurrencyName,
		double tick, const size_t uid)
		: type(type)
		, figi(figi)
		, ticker(ticker)
		, classCode(classCode)
		, isin(isin)
		, lotSize(lotSize)
		, currency(currency)
		, nominal(nominal)
		, buyAvailable(buyAvailable)
		, sellAvailable(sellAvailable)
		, APITradeAvailable(APITradeAvailable)
		, isoCurrencyName(isoCurrencyName)
		, tick(tick)
		, uid(uid)
		, requiredLotMultiplier(lotSize * 10)
	{
	}

	std::string ToString() const
	{
		return std::format("Instrument:\n{{"
						   "\n\ttype       : {}"
						   "\n\tclass      : {}"
						   "\n\tISO        : {}"
						   "\n\tuid        : {}"
						   "\n\tfig        : {}"
						   "\n\tticker     : {}"
						   "\n\tisin       : {}"
						   "\n\tnominal    : {}"
						   "\n\tlot size   : {}"
						   "\n\tmultiplier : {}"
						   "\n\tcurrency   : {}"
						   "\n\ttick       : {}"
						   "\n\tcan buy    : {}"
						   "\n\tcan sell   : {}"
						   "\n\tcan trade  : {}"
						   "\n\tcan limit  : {}"
						   "\n\tcan order  : {}"
						   "\n}}",
			U(type), classCode, isoCurrencyName, uid, figi, ticker, isin, nominal.ToString(), lotSize,
			requiredLotMultiplier, currency, _S(tick), buyAvailable, sellAvailable, APITradeAvailable,
			limitOrderAvailable, marketOrderAvailable);
	}

	friend bool operator<(const InstrumentStructure& f, const InstrumentStructure& s) { return f.figi < s.figi; }
};

struct OrderStructure {
	size_t figi;
	double price;
	uint32_t quantity;

	OrderStructure(const size_t figi, const double price, const uint32_t quantity)
		: figi(figi)
		, price(price)
		, quantity(quantity)
	{
	}

	std::string ToString() const
	{
		std::stringstream stream;
		stream << figi << price << "(" << quantity << ")";
		return stream.str();
	}

	friend bool operator<(const OrderStructure& f, const OrderStructure& s) { return f.figi < s.figi; }
};

#endif //* COMMON_STRUCTURES_H