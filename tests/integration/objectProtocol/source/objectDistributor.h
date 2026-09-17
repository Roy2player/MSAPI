/**************************
 * @file        objectDistributor.h
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

#include "../../../../library/source/protocol/object.inl"
#include "../../../../library/source/server/server.inl"
#include "../../../../library/source/test/actionsCounter.inl"
#include "commonStructures.h"

/**************************
 * @brief Object distributor for MSAPI tests of object protocol.
 *
 * @brief Can send objects of type: InstrumentStructure, OrderStructure.
 */
class ObjectDistributor : public MSAPI::Server, MSAPI::Protocol::Object::Distributor<FilterStructure> {
private:
	std::set<InstrumentStructure> m_instruments;
	std::set<OrderStructure> m_orders;

	MSAPI::ActionsCounter m_unhandledActions;
	uint64_t m_lastConnectionId{};
	MSAPI::Lock::AtomicRW m_distributionLock;

public:
	ObjectDistributor();

	// MSAPI::Server
	void HandleBuffer(MSAPI::RecvBuffer& recvBuffer) final;
	// MSAPI::Application
	void HandleIncomeDisconnect(const std::shared_ptr<MSAPI::Connection::Data>& connectionData) final
	{
		MSAPI::Protocol::Object::Distributor<FilterStructure>::ClearActiveStreamsForConnectionId(
			connectionData->GetConnectionId());
	}
	// MSAPI::Protocol::Object::Distributor
	void HandleNewStreamOpened(MSAPI::Protocol::Object::Distributor<FilterStructure>::StreamData& streamData) final;

	void SetInstrument(const InstrumentStructure& instrument);
	void SetOrder(const OrderStructure& order);
	void Clear();

	FORCE_INLINE void StopDistribution() noexcept { Distributor::Stop(); }

	FORCE_INLINE void CloseConnection()
	{
		const auto connectionData{ GetConnectionData(m_lastConnectionId) };
		if (connectionData == nullptr) {
			LOG_ERROR_NEW("No connection data for connection id: {}", m_lastConnectionId);
			return;
		}

		connectionData->GetConnection().Close();
	}

	FORCE_INLINE [[nodiscard]] uint64_t GetUnhandledActions() const noexcept
	{
		return m_unhandledActions.GetActionsNumber();
	}

	FORCE_INLINE [[nodiscard]] MSAPI::Lock::AtomicRW& GetDistributionLock() noexcept { return m_distributionLock; }

private:
	std::function<bool(const MSAPI::Protocol::Object::FilterBase* filter, const InstrumentStructure& instrument)>
		m_predicateForInstrument
		= [](const MSAPI::Protocol::Object::FilterBase* filter, const InstrumentStructure& instrument) {
			  if (filter->GetFilterObjectHash() == typeid(FilterStructure).hash_code()) {
				  for (const auto& filter :
					  reinterpret_cast<const MSAPI::Protocol::Object::Filter<FilterStructure>*>(filter)->GetObjects()) {

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

	std::function<bool(const MSAPI::Protocol::Object::FilterBase* filter, const OrderStructure& order)>
		m_predicateForOrder = [](const MSAPI::Protocol::Object::FilterBase* filter, const OrderStructure& order) {
			if (filter->GetFilterObjectHash() == typeid(FilterStructure).hash_code()) {
				for (const auto& filter :
					reinterpret_cast<const MSAPI::Protocol::Object::Filter<FilterStructure>*>(filter)->GetObjects()) {

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

#endif // OBJECT_DISTRIBUTOR_H