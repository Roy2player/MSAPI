/**************************
 * @file        httpServer.cpp
 * @version     6.0
 * @date        2024-05-02
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

#include "httpServer.h"
#include "../../../../library/source/help/io.inl"

HTTPServer::HTTPServer()
	: MSAPI::HTTP::IHandler(this)
{
	RegisterParameter(1001, { "Web source path", &m_webSourcesPath });
	MSAPI::Application::SetState(MSAPI::Application::State::Paused);
}

void HTTPServer::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI::ActionsCounter::IncrementActionsNumber();
	MSAPI::DataHeader header{ *recvBufferInfo->buffer };
	MSAPI_HANDLER_HTTP_PRESET
	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void HTTPServer::HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	MSAPI::Application::MergeParameters(parametersUpdate);

	if (!MSAPI::IO::HasPath(m_webSourcesPath.c_str())) {
		MSAPI::Application::SetCustomError(1001, "Web source path does not exist");
	}

	if (!MSAPI::Application::AreParametersValid()) {
		HandlePauseRequest();
	}
}

void HTTPServer::HandleHttp(const int connection, const MSAPI::HTTP::Data& data)
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
		data.Send404(connection, "{\"Error\":\"Method \"" + type + "\" not allowed\"}", "application/json");
		return;
	}

	const std::string& format{ data.GetFormat() };
	if (format == "html") {
		if (url == "/index.html" || url == "/index" || url == "/") {
			std::string indexPage;
			if (MSAPI::IO::ReadStr<std::string_view>(indexPage, m_webSourcesPath + "html/index.html")) {
				data.SendResponse(connection, indexPage, "text/html");
				return;
			}

			data.Send404(connection, "{\"Error\":\"Internal error\"}", "application/json");
			return;
		}

		if (url == "/api") {
			const std::string* identifier{ data.GetValue("Identifier") };
			if (identifier == nullptr) {
				data.Send404(connection, "{\"Error\":\"Identifier not found\"}", "application/json");
				return;
			}
			const std::string* value{ data.GetValue("Action") };
			if (value == nullptr) {
				data.Send404(connection, "{\"Error\":\"Action not found\"}", "application/json");
				return;
			}
			if (*identifier != "369") {
				data.Send404(connection, "{\"Error\":\"Identifier is not valid\"}", "application/json");
				return;
			}
			if (*value != "Send me some JSON, please") {
				data.Send404(connection, "{\"Error\":\"Action is not valid\"}", "application/json");
				return;
			}

			data.SendResponse(connection, "{\"Message\":\"Here is your JSON\"}", "application/json");
			return;
		}

		if (url == "/unknown.html") {
			data.Send404(connection);
			return;
		}

		data.Send404(connection, "{\"Error\":\"Page \"" + url + "\" not found\"}", "application/json");
		return;
	}

	if (format == "css") {
		data.SendSource(connection, m_webSourcesPath + "css" + url);
		return;
	}

	if (format == "ico") {
		data.SendSource(connection, m_webSourcesPath + "images" + url);
		return;
	}

	if (format == "js") {
		data.SendSource(connection, m_webSourcesPath + "js" + url);
		return;
	}

	data.Send404(connection, "{\"Error\":\"Source \"" + url + "\" not found\"}", "application/json");
}

const std::optional<MSAPI::HTTP::Data>& HTTPServer::GetHTTPData() const noexcept { return m_HTTPData; }