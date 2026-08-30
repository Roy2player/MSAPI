/**************************
 * @file        manager.cpp
 * @version     6.0
 * @date        2024-04-10
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

#include "manager.h"

void Manager::HandleBuffer(MSAPI::RecvBuffer& recvBuffer)
{
	MSAPI::DataHeader header{ recvBuffer.GetBuffer() };
	if (header.GetCipher() == helloForHelloCipher) {
		if (m_outcomeConnection != nullptr) {
			LOG_ERROR("Outcome connection is already set");
			m_unhandledActions.IncrementActionsNumber();
			return;
		}
		m_outcomeConnection = recvBuffer.GetConnectionData();
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}

	LOG_ERROR("Unexpected buffer received: " + header.ToString());
	m_unhandledActions.IncrementActionsNumber();
}

void Manager::HandleRunRequest()
{
	LOG_ERROR("Unexpected run request received");
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandlePauseRequest() { MSAPI::ActionsCounter::IncrementActionsNumber(); }

void Manager::HandleModifyRequest(
	[[maybe_unused]] const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	LOG_ERROR("Unexpected modify request received");
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandleHello(const std::shared_ptr<MSAPI::Connection::Data>& connectionData)
{
	if (m_clientConnection == nullptr) {
		m_clientConnection = connectionData;
	}

	MSAPI::Protocol::Standard::Data data{ helloForHelloCipher };
	MSAPI::Protocol::Standard::Send(connectionData->GetConnection(), data);

	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandleMetadata(
	const std::shared_ptr<MSAPI::Connection::Data>& connectionData, const std::string_view metadata)
{
	if (connectionData != m_clientConnection) {
		LOG_ERROR_NEW("Metadata update from unknown connection id: {}", connectionData->GetConnectionId());
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}

	LOG_DEBUG_NEW("Handle metadata update, connection id: {}", connectionData->GetConnectionId());
	m_metadata = metadata;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandleParameters(const std::shared_ptr<MSAPI::Connection::Data>& connectionData,
	const std::map<size_t, std::variant<standardTypes>>& parameters)
{
	if (connectionData != m_clientConnection) {
		LOG_ERROR_NEW("Parameters response from unknown connection id: {}", connectionData->GetConnectionId());
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}

	LOG_DEBUG_NEW("Handle parameters response, connection id: {}", connectionData->GetConnectionId());
	m_parametersResponse = parameters;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandleIncomeDisconnect(const std::shared_ptr<MSAPI::Connection::Data>& connectionData)
{
	LOG_PROTOCOL_NEW("connection id {}", connectionData->GetConnectionId());
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::SendData(const MSAPI::Protocol::Standard::Data& data)
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::Send(m_activeConnection->GetConnection(), data);
}

void Manager::SendActionRun()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::SendActionRun(m_activeConnection->GetConnection());
}

void Manager::SendActionPause()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::SendActionPause(m_activeConnection->GetConnection());
}

void Manager::SendActionDelete()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::SendActionDelete(m_activeConnection->GetConnection());
}

void Manager::SendActionHello()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::SendActionHello(m_activeConnection->GetConnection());
}

void Manager::SendMetadataRequest()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::SendMetadataRequest(m_activeConnection->GetConnection());
}

void Manager::SendParametersRequest()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::SendParametersRequest(m_activeConnection->GetConnection());
}

void Manager::SendMetadataResponse()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	const std::string metadata{ "{\"metadata\":true}" };

	MSAPI::Protocol::Standard::Data metadataData{ MSAPI::Protocol::Standard::cipherMetadataResponse };
	metadataData.SetData(0, metadata);
	MSAPI::Protocol::Standard::Send(m_activeConnection->GetConnection(), metadataData);
}

void Manager::SendParametersResponse()
{
	if (m_activeConnection == nullptr) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::Protocol::Standard::Data data{ MSAPI::Protocol::Standard::cipherParametersResponse };
	data.SetData(505050, 960.960964);
	MSAPI::Protocol::Standard::Send(m_activeConnection->GetConnection(), data);
}

std::string Manager::GetParameters() const
{
	std::string parameters;
	Application::GetParameters(parameters);
	return parameters;
}

const std::string& Manager::GetMetadata() const noexcept { return m_metadata; }

const std::map<size_t, std::variant<standardTypes>>& Manager::GetParametersResponse() const noexcept
{
	return m_parametersResponse;
}

void Manager::Stop()
{
	m_clientConnection.reset();
	MSAPI::Server::Stop();
}

void Manager::UseOutcomeConnection()
{
	if (m_outcomeConnection == nullptr) {
		LOG_ERROR("Outcome connection is not set");
		return;
	}

	if (m_activeConnection == m_outcomeConnection) {
		LOG_ERROR("Outcome connection is already active");
		return;
	}

	m_activeConnection = m_outcomeConnection;
}

void Manager::UseClientConnection()
{
	if (m_clientConnection == nullptr) {
		LOG_ERROR("Client connection is not set");
		return;
	}

	if (m_activeConnection == m_clientConnection) {
		LOG_ERROR("Client connection is already active");
		return;
	}

	m_activeConnection = m_clientConnection;
}

const size_t& Manager::GetUnhandledActions() const noexcept { return m_unhandledActions.GetActionsNumber(); }

void Manager::WaitUnhandledActions(const MSAPI::Test& test, const size_t delay, const size_t expected)
{
	m_unhandledActions.WaitActionsNumber(test, delay, expected);
}