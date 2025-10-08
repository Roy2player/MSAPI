/**************************
 * @file        objectClient.h
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
 */

#ifndef OBJECT_CLIENT_H
#define OBJECT_CLIENT_H

#include "../../../library/source/protocol/object.h"
#include "../../../library/source/server/server.h"
#include "../../../library/source/test/actionsCounter.h"
#include "commonStructures.h"

/**************************
 * @brief Object client for MSAPI tests of object protocol.
 */
class ObjectClient : public MSAPI::Server,
					 public MSAPI::ActionsCounter,
					 MSAPI::ObjectProtocol::IHandler<InstrumentStructure>,
					 MSAPI::ObjectProtocol::IHandler<OrderStructure> {
private:
	MSAPI::ObjectProtocol::Stream<InstrumentStructure, FilterStructure> m_instrumentStream{ this };
	MSAPI::ObjectProtocol::Stream<OrderStructure, FilterStructure> m_orderStream{ this };

	std::set<InstrumentStructure> m_instruments;
	std::set<OrderStructure> m_orders;

public:
	ObjectClient();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::ObjectProtocol::IHandler
	void HandleStreamOpened(int streamId) final;
	void HandleStreamSnapshotDone(int streamId) final;
	void HandleStreamFailed(int streamId) final;
	void HandleObject(int streamId, const InstrumentStructure& object) final;
	void HandleObject(int streamId, const OrderStructure& object) final;

	void Clear();

	const std::set<InstrumentStructure>& GetInstruments() const;
	const std::set<OrderStructure>& GetOrders() const;

	bool HasInstrument(const InstrumentStructure& instrument) const;
	bool HasOrder(const OrderStructure& order) const;

	MSAPI::ObjectProtocol::Stream<InstrumentStructure, FilterStructure>& GetInstrumentStream();
	MSAPI::ObjectProtocol::Stream<OrderStructure, FilterStructure>& GetOrderStream();

	void SetConnectionForStreams(int id);
};

#endif //* OBJECT_CLIENT_H