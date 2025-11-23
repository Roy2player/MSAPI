/**************************
 * @file        objectClient.cpp
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

#include "objectClient.h"

ObjectClient::ObjectClient()
	: MSAPI::ObjectProtocol::ApplicationStateChecker(this)
{
	MSAPI::Application::SetState(MSAPI::Application::State::Running);
}

void ObjectClient::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI::DataHeader header{ *recvBufferInfo->buffer };

	if (header.GetCipher() == 2666999999) {
		if (!Server::ReadAdditionalData(recvBufferInfo, header.GetBufferSize())) {
			return;
		}

		MSAPI::ObjectProtocol::Data data{ std::move(header), *recvBufferInfo->buffer };

		void* object;
		MSAPI::ObjectProtocol::Data::UnpackData(&object, *recvBufferInfo->buffer);

		if (data.GetHash() == typeid(MSAPI::ObjectProtocol::StreamStateResponse).hash_code()) {
			CollectStreamState(
				data.GetStreamId(), reinterpret_cast<MSAPI::ObjectProtocol::StreamStateResponse*>(object));
			return;
		}

		if (data.GetHash() == typeid(InstrumentStructure).hash_code()) {
			IHandler<InstrumentStructure>::Collect(data, object);
			return;
		}
		if (data.GetHash() == typeid(OrderStructure).hash_code()) {
			IHandler<OrderStructure>::Collect(data, object);
			return;
		}

		LOG_ERROR("Unknown object protocol data: " + data.ToString());
	}

	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void ObjectClient::Clear()
{
	m_instruments.clear();
	m_orders.clear();
	MSAPI::ActionsCounter::ClearActionsNumber();
}

const std::set<InstrumentStructure>& ObjectClient::GetInstruments() const { return m_instruments; }

const std::set<OrderStructure>& ObjectClient::GetOrders() const { return m_orders; }

bool ObjectClient::HasInstrument(const InstrumentStructure& instrument) const
{
	return m_instruments.find(instrument) != m_instruments.end();
}

bool ObjectClient::HasOrder(const OrderStructure& order) const { return m_orders.find(order) != m_orders.end(); }

MSAPI::ObjectProtocol::Stream<InstrumentStructure, FilterStructure>& ObjectClient::GetInstrumentStream()
{
	return m_instrumentStream;
}

MSAPI::ObjectProtocol::Stream<OrderStructure, FilterStructure>& ObjectClient::GetOrderStream() { return m_orderStream; }

void ObjectClient::HandleObject([[maybe_unused]] const int streamId, const InstrumentStructure& object)
{
	LOG_DEBUG("Got Instrument object");
	m_instruments.emplace(object);
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleObject([[maybe_unused]] const int streamId, const OrderStructure& object)
{
	LOG_DEBUG("Got Order object");
	m_orders.emplace(object);
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleStreamOpened(const int streamId)
{
	LOG_DEBUG("Stream open, id: " + _S(streamId));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleStreamSnapshotDone(const int streamId)
{
	LOG_DEBUG("Stream done, id: " + _S(streamId));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleStreamFailed(const int streamId)
{
	LOG_DEBUG("Stream failed, id: " + _S(streamId));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::SetConnectionForStreams(const int id)
{
	const auto connection{ GetConnect(id) };
	if (!connection.has_value()) {
		LOG_ERROR("Din't find connection for id: " + _S(id));
		return;
	}
	const auto connectionValue{ connection.value() };
	m_instrumentStream.SetConnection(connectionValue);
	m_orderStream.SetConnection(connectionValue);
}