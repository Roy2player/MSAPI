/**************************
 * @file        objectDistributor.h
 * @version     6.0
 * @date        2023-12-16
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

#ifndef OBJECT_DISTRIBUTOR_H
#define OBJECT_DISTRIBUTOR_H

#include "../../../../library/source/protocol/object.h"
#include "../../../../library/source/server/server.h"
#include "commonStructures.h"

/**************************
 * @brief Object distributor for MSAPI tests of object protocol.
 *
 * @brief Can send objects of type: InstrumentStructure, OrderStructure.
 */
class ObjectDistributor : public MSAPI::Server, MSAPI::ObjectProtocol::Distributor<FilterStructure> {
private:
	std::set<InstrumentStructure> m_instruments;
	std::set<OrderStructure> m_orders;

public:
	ObjectDistributor();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::ObjectProtocol::Distributor
	void HandleNewStreamOpened(int streamId, const MSAPI::ObjectProtocol::StreamData& streamData) final;

	void SetInstrument(const InstrumentStructure& instrument);
	void SetOrder(const OrderStructure& order);
	void Clear();

private:
	std::function<bool(const MSAPI::ObjectProtocol::FilterBase* filter, const InstrumentStructure& instrument)>
		m_predicateForInstrument
		= [](const MSAPI::ObjectProtocol::FilterBase* filter, const InstrumentStructure& instrument) {
			  if (filter->GetFilterObjectHash() == typeid(FilterStructure).hash_code()) {
				  for (const auto& filter :
					  reinterpret_cast<const MSAPI::ObjectProtocol::Filter<FilterStructure>*>(filter)->GetObjects()) {

					  if (instrument.figi == filter.figi) {
						  LOG_PROTOCOL(
							  "Object figi: " + _S(instrument.figi) + " match with filter figi: " + _S(filter.figi));
						  return true;
					  }
				  }
			  }
			  else {
				  LOG_ERROR("Unknown filter's object hash: " + _S(filter->GetFilterObjectHash()));
			  }
			  return false;
		  };

	std::function<bool(const MSAPI::ObjectProtocol::FilterBase* filter, const OrderStructure& order)>
		m_predicateForOrder = [](const MSAPI::ObjectProtocol::FilterBase* filter, const OrderStructure& order) {
			if (filter->GetFilterObjectHash() == typeid(FilterStructure).hash_code()) {
				for (const auto& filter :
					reinterpret_cast<const MSAPI::ObjectProtocol::Filter<FilterStructure>*>(filter)->GetObjects()) {

					if (order.figi == filter.figi) {
						LOG_PROTOCOL("Object figi: " + _S(order.figi) + " match with filter figi: " + _S(filter.figi));
						return true;
					}
				}
			}
			else {
				LOG_ERROR("Unknown filter's object hash: " + _S(filter->GetFilterObjectHash()));
			}
			return false;
		};
};

#endif //* OBJECT_DISTRIBUTOR_H