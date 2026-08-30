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
	: MSAPI::Protocol::HTTP::IHandler(this)
{
	MSAPI::Application::SetState(MSAPI::Application::State::Running);
}

void HTTPClient::HandleBuffer(MSAPI::RecvBuffer& recvBuffer)
{
	MSAPI::ActionsCounter::IncrementActionsNumber();
	MSAPI::DataHeader header{ recvBuffer.GetBuffer() };
	MSAPI_HANDLER_HTTP_PRESET

	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void HTTPClient::HandleHttp([[maybe_unused]] const std::shared_ptr<MSAPI::Connection::Data>& connectionData,
	const MSAPI::Protocol::HTTP::Data& data)
{
	m_HTTPData = data;
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

const std::optional<MSAPI::Protocol::HTTP::Data>& HTTPClient::GetHTTPData() const noexcept { return m_HTTPData; }