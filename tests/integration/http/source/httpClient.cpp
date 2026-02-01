/**************************
 * @file        httpClient.cpp
 * @version     6.0
 * @date        2024-05-02
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

#include "httpClient.h"

HTTPClient::HTTPClient()
	: MSAPI::HTTP::IHandler(this)
{
	MSAPI::Application::SetState(MSAPI::Application::State::Running);
}

void HTTPClient::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI::ActionsCounter::IncrementActionsNumber();
	MSAPI::DataHeader header{ *recvBufferInfo->buffer };
	MSAPI_HANDLER_HTTP_PRESET

	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void HTTPClient::HandleHttp([[maybe_unused]] const int connection, const MSAPI::HTTP::Data& data)
{
	m_HTTPData = data;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

const std::optional<MSAPI::HTTP::Data>& HTTPClient::GetHTTPData() const noexcept { return m_HTTPData; }

void HTTPClient::SendRequest(const int id, const std::string& HTTP)
{
	if (const auto connect{ GetConnect(id) }; connect.has_value()) {
		MSAPI::HTTP::SendRequest(connect.value(), HTTP);
		return;
	}

	LOG_ERROR("Connection not found for id: " + _S(id));
}