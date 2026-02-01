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

void Manager::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI::DataHeader header{ *recvBufferInfo->buffer };
	if (header.GetCipher() == helloFotHelloCipher) {
		if (m_outcomeConnection != -1) {
			LOG_ERROR("Outcome connection is already set");
			m_unhandledActions.IncrementActionsNumber();
			return;
		}
		m_outcomeConnection = recvBufferInfo->connection;
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

void Manager::HandleHello(const int connection)
{
	if (m_clientConnection == -1) {
		m_clientConnection = connection;
	}

	MSAPI::StandardProtocol::Data data{ helloFotHelloCipher };
	MSAPI::StandardProtocol::Send(connection, data);

	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandleMetadata(const int connection, const std::string_view metadata)
{
	if (connection != m_clientConnection) {
		LOG_ERROR("Metadata update from unknown connection: " + _S(connection));
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}

	LOG_DEBUG("Handle metadata update, connection: " + _S(connection));
	m_metadata = metadata;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::HandleParameters(const int connection, const std::map<size_t, std::variant<standardTypes>>& parameters)
{
	if (connection != m_clientConnection) {
		LOG_ERROR("Parameters response from unknown connection: " + _S(connection));
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}

	LOG_DEBUG("Handle parameters response, connection: " + _S(connection));
	m_parametersResponse = parameters;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::SendData(const MSAPI::StandardProtocol::Data& data)
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::Send(m_activeConnection, data);
}

void Manager::SendActionRun()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::SendActionRun(m_activeConnection);
}

void Manager::SendActionPause()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::SendActionPause(m_activeConnection);
}

void Manager::SendActionDelete()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::SendActionDelete(m_activeConnection);
}

void Manager::SendActionHello()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::SendActionHello(m_activeConnection);
}

void Manager::SendMetadataRequest()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::SendMetadataRequest(m_activeConnection);
}

void Manager::SendParametersRequest()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::SendParametersRequest(m_activeConnection);
}

void Manager::SendMetadataResponse()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	const std::string metadata{ "{\"metadata\":true}" };

	MSAPI::StandardProtocol::Data metadataData{ MSAPI::StandardProtocol::cipherMetadataResponse };
	metadataData.SetData(0, metadata);
	MSAPI::StandardProtocol::Send(m_activeConnection, metadataData);
}

void Manager::SendParametersResponse()
{
	if (m_activeConnection == -1) {
		LOG_ERROR("Active connection is not set");
		return;
	}

	MSAPI::StandardProtocol::Data data{ MSAPI::StandardProtocol::cipherParametersResponse };
	data.SetData(505050, 960.960964);
	MSAPI::StandardProtocol::Send(m_activeConnection, data);
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
	m_clientConnection = -1;
	MSAPI::Server::Stop();
}

void Manager::UseOutcomeConnection()
{
	if (m_outcomeConnection == -1) {
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
	if (m_clientConnection == -1) {
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