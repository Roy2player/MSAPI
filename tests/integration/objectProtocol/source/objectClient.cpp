/**************************
 * @file        objectClient.cpp
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

#include "objectClient.h"

ObjectClient::ObjectClient()
	: MSAPI::Protocol::Object::IHandlerBase{ *static_cast<const Application*>(this) }
{
	MSAPI::Application::SetState(MSAPI::Application::State::Running);
}

void ObjectClient::HandleBuffer(MSAPI::RecvBuffer& recvBuffer)
{
	MSAPI::DataHeader header{ recvBuffer.GetBuffer() };

	if (MSAPI::Protocol::Object::IHandlerBase::Collect(header, recvBuffer)) {
		return;
	}

	m_unhandledActions.IncrementActionsNumber();
	LOG_ERROR_NEW("Unknown protocol: {}", header.ToString());
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

MSAPI::Protocol::Object::Stream<InstrumentStructure, FilterStructure>& ObjectClient::GetInstrumentStream()
{
	return m_instrumentStream;
}

MSAPI::Protocol::Object::Stream<OrderStructure, FilterStructure>& ObjectClient::GetOrderStream()
{
	return m_orderStream;
}

void ObjectClient::HandleObject([[maybe_unused]] const uint64_t streamId, const InstrumentStructure& object) noexcept
{
	LOG_DEBUG("Got Instrument object");
	m_instruments.emplace(object);
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleObject([[maybe_unused]] const uint64_t streamId, const OrderStructure& object) noexcept
{
	LOG_DEBUG("Got Order object");
	m_orders.emplace(object);
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleStreamOpened(const uint64_t streamId) noexcept
{
	LOG_DEBUG("Stream open, id: " + _S(streamId));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleStreamSnapshotDone(const uint64_t streamId) noexcept
{
	LOG_DEBUG("Stream done, id: " + _S(streamId));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void ObjectClient::HandleStreamFailed(const uint64_t streamId, const MSAPI::Protocol::Object::Issue issue) noexcept
{
	LOG_DEBUG("Stream failed, id: " + _S(streamId) + ", reason: " + MSAPI::Protocol::Object::EnumToString(issue));
	m_lastFailedIssue = issue;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

bool ObjectClient::SetConnectionForStreams(const std::shared_ptr<MSAPI::Connection::Data>& connectionData)
{
	return m_instrumentStream.SetConnectionData(connectionData) && m_orderStream.SetConnectionData(connectionData);
}