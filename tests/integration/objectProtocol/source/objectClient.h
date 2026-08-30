/**************************
 * @file        objectClient.h
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

#ifndef OBJECT_CLIENT_H
#define OBJECT_CLIENT_H

#include "../../../../library/source/protocol/object.inl"
#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"
#include "commonStructures.h"

/**************************
 * @brief Object client for MSAPI tests of object protocol.
 */
class ObjectClient : public MSAPI::Server,
					 public MSAPI::ActionsCounter,
					 MSAPI::Protocol::Object::IHandler<InstrumentStructure>,
					 MSAPI::Protocol::Object::IHandler<OrderStructure> {
private:
	MSAPI::Protocol::Object::Stream<InstrumentStructure, FilterStructure> m_instrumentStream{ *this };
	MSAPI::Protocol::Object::Stream<OrderStructure, FilterStructure> m_orderStream{ *this };

	std::set<InstrumentStructure> m_instruments;
	std::set<OrderStructure> m_orders;

public:
	ObjectClient();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBuffer& recvBuffer) final;
	//* MSAPI::Protocol::Object::IHandler
	void HandleStreamOpened(uint64_t streamId) noexcept final;
	void HandleStreamSnapshotDone(uint64_t streamId) noexcept final;
	void HandleStreamFailed(uint64_t streamId, MSAPI::Protocol::Object::Issue issue) noexcept final;
	void HandleObject(uint64_t streamId, const InstrumentStructure& object) noexcept final;
	void HandleObject(uint64_t streamId, const OrderStructure& object) noexcept final;

	void Clear();

	const std::set<InstrumentStructure>& GetInstruments() const;
	const std::set<OrderStructure>& GetOrders() const;

	bool HasInstrument(const InstrumentStructure& instrument) const;
	bool HasOrder(const OrderStructure& order) const;

	MSAPI::Protocol::Object::Stream<InstrumentStructure, FilterStructure>& GetInstrumentStream();
	MSAPI::Protocol::Object::Stream<OrderStructure, FilterStructure>& GetOrderStream();

	void SetConnectionForStreams(const std::shared_ptr<MSAPI::Connection::Data>& connectionData);
};

#endif //* OBJECT_CLIENT_H