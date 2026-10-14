/**************************
 * @file        httpServer.cpp
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

#include "httpServer.h"
#include "../../../../library/source/help/io.inl"

HTTPServer::HTTPServer()
	: MSAPI::Protocol::HTTP::IHandler(this)
{
	RegisterParameter(1001, { "Web source path", &m_webSourcesPath });
	MSAPI::Application::SetState(MSAPI::Application::State::Paused);
}

void HTTPServer::HandleBuffer(MSAPI::RecvBuffer& recvBuffer)
{
	MSAPI::ActionsCounter::IncrementActionsNumber();
	MSAPI::DataHeader header{ recvBuffer.GetBuffer() };
	MSAPI_HANDLER_HTTP_PRESET
	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void HTTPServer::HandleModifyRequest(const std::map<uint64_t, std::variant<standardTypes>>& parametersUpdate)
{
	MSAPI::Application::MergeParameters(parametersUpdate);

	if (!MSAPI::IO::HasPath(m_webSourcesPath.c_str())) {
		MSAPI::Application::SetCustomError(1001, "Web source path does not exist");
	}

	// Rebuild fixed roots after configuration changes; LookupPath retains registration order.
	ClearPaths();
	for (const std::string_view key : { "html", "css", "images", "js" }) {
		MSAPI::SString<256> root;
		if (m_webSourcesPath.empty() || !root.Copy(m_webSourcesPath)
			|| (m_webSourcesPath.back() != '/' && !root.Concatenate("/")) || !root.Concatenate(key)
			|| !AddPath(key, root.Get())) {

			MSAPI::Application::SetCustomError(1001, "Web source path does not fit HTTP root storage");
			break;
		}
	}

	if (!MSAPI::Application::AreParametersValid()) {
		HandlePauseRequest();
	}
}

void HTTPServer::HandleHttp(
	const std::shared_ptr<MSAPI::Connection::Data>& connectionData, const MSAPI::Protocol::HTTP::Data& data)
{
	if (MSAPI::Application::GetState() != MSAPI::Application::State::Running) {
		LOG_DEBUG("State is not Running, do nothing");
		return;
	}

	struct R {
		HTTPServer* t;

		R(HTTPServer* t)
			: t(t)
		{
		}

		~R() { t->MSAPI::ActionsCounter::IncrementActionsNumber(); }
	} r{ this };

	m_HTTPData = data;
	const std::string& url{ data.GetUrl() };
	LOG_DEBUG("Request url: " + url + ", version: " + data.GetVersion());
	if (const auto& type{ data.GetTypeMessage() }; type != "GET") {
		(void)data.Send404(
			connectionData->GetConnection(), "{\"Error\":\"Method \"" + type + "\" not allowed\"}", "application/json");
		return;
	}

	// Test canonical-host redirects through Data's shared response formatter.
	if (data.GetHost().starts_with("www.")) {
		MSAPI::SString<2048> target;
		MSAPI::SString<2304> location;
		if (target.Copy(data.GetUrl()) && target.Concatenate(data.GetQuery())
			&& MSAPI::Protocol::HTTP::BuildRedirectLocation(location, "http", data.GetHost().substr(4), target.Get())) {
			(void)data.SendRedirect(connectionData->GetConnection(), location.Get(),
				MSAPI::Protocol::HTTP::StatusCode::MovedPermanently, MSAPI::Protocol::HTTP::CLOSE_CONNECTION);
		}

		return;
	}

	const std::string& format{ data.GetFormat() };
	if (format == "html") {
		if (url == "/index.html" || url == "/index" || url == "/") {
			MSAPI::SString<512> source;
			if (LookupPath("html", "/index.html", source)) {
				(void)data.SendSource(
					connectionData->GetConnection(), source.Get(), "text/html", data.IsConnectionClose());
				return;
			}

			(void)data.Send404(connectionData->GetConnection(), "{\"Error\":\"Internal error\"}", "application/json");
			return;
		}

		if (url == "/api") {
			const std::string* identifier{ data.GetValue("Identifier") };
			if (identifier == nullptr) {
				(void)data.Send404(
					connectionData->GetConnection(), "{\"Error\":\"Identifier not found\"}", "application/json");
				return;
			}
			const std::string* value{ data.GetValue("Action") };
			if (value == nullptr) {
				(void)data.Send404(
					connectionData->GetConnection(), "{\"Error\":\"Action not found\"}", "application/json");
				return;
			}
			if (*identifier != "369") {
				(void)data.Send404(
					connectionData->GetConnection(), "{\"Error\":\"Identifier is not valid\"}", "application/json");
				return;
			}
			if (*value != "Send me some JSON, please") {
				(void)data.Send404(
					connectionData->GetConnection(), "{\"Error\":\"Action is not valid\"}", "application/json");
				return;
			}

			(void)data.SendResponse(
				connectionData->GetConnection(), "{\"Message\":\"Here is your JSON\"}", "application/json");
			return;
		}

		if (url == "/unknown.html") {
			(void)data.Send404(connectionData->GetConnection());
			return;
		}

		(void)data.Send404(
			connectionData->GetConnection(), "{\"Error\":\"Page \"" + url + "\" not found\"}", "application/json");
		return;
	}

	const std::string_view key{ format == "css" ? "css" : format == "ico" ? "images" : format == "js" ? "js" : "" };
	if (!key.empty()) {
		MSAPI::SString<512> source;
		const auto closeConnection{ data.IsConnectionClose() };
		if (LookupPath(key, url, source)) {
			(void)data.SendSource(connectionData->GetConnection(), source.Get(), /*contentType=*/"", closeConnection);
			return;
		}

		(void)data.Send404(connectionData->GetConnection(), /*body=*/{}, /*contentType=*/{}, closeConnection);
		return;
	}

	(void)data.Send404(
		connectionData->GetConnection(), "{\"Error\":\"Source \"" + url + "\" not found\"}", "application/json");
}

const std::optional<MSAPI::Protocol::HTTP::Data>& HTTPServer::GetHTTPData() const noexcept { return m_HTTPData; }