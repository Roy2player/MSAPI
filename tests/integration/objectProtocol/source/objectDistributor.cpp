/**************************
 * @file        objectDistributor.cpp
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

#include "objectDistributor.h"

ObjectDistributor::ObjectDistributor()
	: MSAPI::ObjectProtocol::ApplicationStateChecker(this)
{
	MSAPI::Application::SetState(MSAPI::Application::State::Running);
}

void ObjectDistributor::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
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
			Distributor::StreamExternalAction({ data.GetStreamId(), recvBufferInfo->connection },
				reinterpret_cast<MSAPI::ObjectProtocol::StreamStateResponse*>(object));
			return;
		}

		if (data.GetHash() == typeid(MSAPI::ObjectProtocol::Filter<FilterStructure>).hash_code()
			|| data.GetHash() == typeid(FilterStructure).hash_code()) {

			Distributor::Collect<FilterStructure>(recvBufferInfo->connection, data, object);
			return;
		}

		LOG_ERROR("Unknown object protocol data: " + data.ToString());
	}

	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void ObjectDistributor::SetInstrument(const InstrumentStructure& instrument)
{
	Distributor::SendNewObject(instrument, m_predicateForInstrument);
	m_instruments.emplace(instrument);
}

void ObjectDistributor::SetOrder(const OrderStructure& order)
{
	Distributor::SendNewObject(order, m_predicateForOrder);
	m_orders.emplace(order);
}

void ObjectDistributor::HandleNewStreamOpened(const int streamId, const MSAPI::ObjectProtocol::StreamData& streamData)
{
	if (typeid(InstrumentStructure).hash_code() == streamData.objectHash) {
		LOG_DEBUG("Stream id: " + _S(streamId) + ", connection: " + _S(streamData.connection)
			+ ", hash: " + _S(streamData.objectHash) + " is open");
		Distributor::SendOldObjects(streamId, streamData, m_instruments, m_predicateForInstrument);
		return;
	}
	if (typeid(OrderStructure).hash_code() == streamData.objectHash) {
		LOG_DEBUG("Stream id: " + _S(streamId) + ", connection: " + _S(streamData.connection)
			+ ", hash: " + _S(streamData.objectHash) + " is open");
		Distributor::SendOldObjects(streamId, streamData, m_orders, m_predicateForOrder);
		return;
	}

	LOG_ERROR("Unknown hash for opening stream: " + streamData.ToString());
}

void ObjectDistributor::Clear()
{
	m_instruments.clear();
	m_orders.clear();
}