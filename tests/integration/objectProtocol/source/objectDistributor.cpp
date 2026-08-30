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
	: MSAPI::Protocol::Object::Distributor<FilterStructure>(this)
{
	MSAPI::Application::SetState(MSAPI::Application::State::Running);
}

void ObjectDistributor::HandleBuffer(MSAPI::RecvBuffer& recvBuffer)
{
	MSAPI::DataHeader header{ recvBuffer.GetBuffer() };

	if (header.GetCipher() == 2666999999) {
		if (!recvBuffer.RecvAdditional(header.GetBufferSize())) {
			return;
		}

		MSAPI::Protocol::Object::Data data{ std::move(header), recvBuffer.GetBuffer() };

		const void* object;
		MSAPI::Protocol::Object::Data::UnpackData(&object, recvBuffer.GetData());

		if (data.GetObjectHash() == typeid(MSAPI::Protocol::Object::StreamStateResponse).hash_code()) {
			Distributor::StreamExternalAction(data.GetStreamId(), recvBuffer.GetConnectionId(),
				reinterpret_cast<const MSAPI::Protocol::Object::StreamStateResponse*>(object));
			return;
		}

		if (data.GetObjectHash() == typeid(MSAPI::Protocol::Object::Filter<FilterStructure>).hash_code()
			|| data.GetObjectHash() == typeid(FilterStructure).hash_code()) {

			Distributor::Collect<FilterStructure>(recvBuffer.GetConnectionData(), data, object);
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

void ObjectDistributor::HandleNewStreamOpened(
	MSAPI::Protocol::Object::Distributor<FilterStructure>::StreamData& streamData)
{
	const auto streamObjectHash{ streamData.GetStreamObjectHash() };
	if (typeid(InstrumentStructure).hash_code() == streamObjectHash) {
		std::cout << "--> try to send 1" << std::endl;
		(void)Distributor::SendObjectsToStream(streamData, m_instruments, m_predicateForInstrument);
		return;
	}
	if (typeid(OrderStructure).hash_code() == streamObjectHash) {
		std::cout << "--> try to send 2" << std::endl;
		(void)Distributor::SendObjectsToStream(streamData, m_orders, m_predicateForOrder);
		return;
	}

	LOG_ERROR_NEW("Unknown hash for opening stream: {}", streamObjectHash);
}

void ObjectDistributor::Clear()
{
	m_instruments.clear();
	m_orders.clear();
}