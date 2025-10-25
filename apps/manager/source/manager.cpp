/**************************
 * @file        manager.cpp
 * @version     6.0
 * @date        2024-02-21
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

#include "manager.h"
#include "../../../library/source/help/bin.h"
#include <algorithm>
#include <sys/wait.h>

Manager::Manager()
	: MSAPI::HTTP::IHandler(this)
{
	if (access("/bin/bash", X_OK) != 0) {
		LOG_ERROR("Can't access to /bin/bash, terminate application");
		Server::Stop();
		return;
	}

	RegisterParameter(1001, { "Web sources path", &m_webSourcesPath });
}

Manager::~Manager()
{
	m_metadataRequestsLock.Lock();
	m_parametersRequestsLock.Lock();
	m_deleteRequestsLock.Lock();
	m_createdAppToPortLock.Lock();
}

void Manager::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI::DataHeader header{ *recvBufferInfo->buffer };
	MSAPI_HANDLER_HTTP_PRESET

	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void Manager::HandleHttp(const int connection, const MSAPI::HTTP::Data& data)
{
	const auto sendNegativeResponse = [this, &data, connection](const std::string& error) {
		LOG_DEBUG("Send negative response: " + error);
		data.SendResponse(connection, "{\"status\":false,\"message\":\"" + error + "\"}\n\r");
	};

	const std::string& url{ data.GetUrl() };
	LOG_DEBUG("Request url: " + url + ", version: " + data.GetVersion());
	if (data.GetTypeMessage() != "GET") {
		data.Send404(connection);
		return;
	}

	if (url == "/") {
		data.SendSource(connection, m_webSourcesPath + "html/index.html");
		return;
	}

	if (data.GetFormat() == "css") {
		data.SendSource(connection, m_webSourcesPath + "css" + url);
		return;
	}

	if (data.GetFormat() == "ico" || data.GetFormat() == "png" || data.GetFormat() == "jpg") {
		data.SendSource(connection, m_webSourcesPath + "images" + url);
		return;
	}

	if (data.GetFormat() == "js") {
		data.SendSource(connection, m_webSourcesPath + "js" + url);
		return;
	}

	if (url == "/api") {
		const auto* type{ data.GetValue("Type") };
		if (type == nullptr) {
			sendNegativeResponse("Key by Type is not found"); // 404 with message
			return;
		}
		LOG_DEBUG("Type key: " + *type);

		if (*type == "getInstalledApps") {
			std::string body{ "{\"status\":true,\"apps\":[" };
			if (!m_installedAppDataToHash.empty()) {
				auto it{ m_installedAppDataToHash.begin() };
				const auto end{ m_installedAppDataToHash.end() };

				body += "{\"type\":\"" + it->second.type + "\"";
				if (it->second.hasView) {
					body += ",\"viewPortParameter\":" + _S(it->second.viewPortParameter);
				}
				body += "}";
				for (++it; it != end; ++it) {
					body += ",{\"type\":\"" + it->second.type + "\"";
					if (it->second.hasView) {
						body += ",\"viewPortParameter\":" + _S(it->second.viewPortParameter);
					}
					body += "}";
				}
			}
			body += "]}\n\n";
			data.SendResponse(connection, body);
			return;
		}

		if (*type == "createApp") {
			const auto* appType{ data.GetValue("AppType") };
			if (appType == nullptr) {
				sendNegativeResponse("Key 'AppType' in header is not found"); // 404 with message
				return;
			}

			auto it{ m_installedAppDataToHash.find(MSAPI::Helper::stringHasher(*appType)) };
			if (it == m_installedAppDataToHash.end()) {
				sendNegativeResponse("Unknow app type: " + *appType);
				return;
			}

			std::string error;
			if (const auto port{ CreateApp(it, data, error) }; port != 0) {
				data.SendResponse(connection, "{\"status\":true,\"port\":" + _S(port) + "}\n\n");
			}
			else {
				sendNegativeResponse("Can't create instance of : " + *appType + ", error: " + error);
			}
			return;
		}

		if (*type == "getCreatedApps") {
			std::string body{ "{\"status\":true,\"apps\":[" };
			{
				MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_createdAppToPortLock };
				if (!m_createdAppToPort.empty()) {
					auto it{ m_createdAppToPort.begin() };
					const auto end{ m_createdAppToPort.end() };

					body += "{\"type\":\"" + it->second.appData->type + "\",\"port\":" + _S(it->first) + ",\"pid\":"
						+ _S(it->second.pid) + ",\"creation time\":\"" + it->second.created.ToString() + "\"}";
					for (++it; it != end; ++it) {
						body += ",{\"type\":\"" + it->second.appData->type + "\",\"port\":" + _S(it->first)
							+ ",\"pid\":" + _S(it->second.pid) + ",\"creation time\":\"" + it->second.created.ToString()
							+ "\"}";
					}
				}
			}
			body += "]}\n\n";
			data.SendResponse(connection, body);
			return;
		}

		if (*type == "getMetadata") {
			const auto* appType{ data.GetValue("AppType") };
			if (appType == nullptr) {
				sendNegativeResponse("Key 'AppType' in header is not found"); // 404 with message
				return;
			}

			const auto hash{ MSAPI::Helper::stringHasher(*appType) };
			const auto it{ m_installedAppDataToHash.find(hash) };
			if (it == m_installedAppDataToHash.end()) {
				sendNegativeResponse("Unknown app type: " + *appType); // 400 Not found
				return;
			}

			if (std::ranges::find_if(m_createdAppToPort,
					[&appType](const auto& createdAppData) { return createdAppData.second.appData->type == *appType; })
				== m_createdAppToPort.end()) {

				sendNegativeResponse("No instance of app " + *appType + " is created"); // 404 with message
				return;
			}

			if (!it->second.metadata.empty()) {
				data.SendResponse(connection, "{\"status\":true,\"metadata\":" + it->second.metadata + "}\n\n");
			}
			else {
				MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_metadataRequestsLock };
				m_metadataRequestsToHash.emplace(
					hash, RequestInfo{ RequestInfo::Type::Metadata, hash, connection, data });
			}
			return;
		}

		if (*type == "getParameters") {
			const auto* portStr{ data.GetValue("Port") };
			if (portStr == nullptr) {
				sendNegativeResponse("Key 'Port' in header is not found"); // 404 with message
				return;
			}
			if (portStr->empty()) {
				sendNegativeResponse("Key 'Port' in header is empty"); // 404 with message
				return;
			}

			uint16_t port;
			const auto error{ std::from_chars(portStr->data(), portStr->data() + portStr->size(), port).ec };
			if (error != std::errc{}) {
				sendNegativeResponse("Key 'Port' in header cannot be converted properly: " + *portStr
					+ ". Error: " + std::make_error_code(error).message()); // 404 with message
				return;
			}

			if (const auto it{ m_createdAppToPort.find(port) }; it != m_createdAppToPort.end()) {
				SendParametersRequest<true>(port, it->second.connection,
					RequestInfo{ RequestInfo::Type::Parameters, static_cast<size_t>(port), connection, data });
				return;
			}

			sendNegativeResponse("App with port: " + *portStr + " is not found"); // 404 with message
			return;
		}

		if (*type == "pause") {
			const auto* portStr{ data.GetValue("Port") };
			if (portStr == nullptr) {
				sendNegativeResponse("Key 'Port' in header is not found"); // 404 with message
				return;
			}
			if (portStr->empty()) {
				sendNegativeResponse("Key 'Port' in header is empty"); // 404 with message
				return;
			}

			uint16_t port;
			const auto error{ std::from_chars(portStr->data(), portStr->data() + portStr->size(), port).ec };
			if (error != std::errc{}) {
				sendNegativeResponse("Key 'Port' in header cannot be converted properly: " + *portStr
					+ ". Error: " + std::make_error_code(error).message()); // 404 with message
				return;
			}

			if (m_runRequestToPort.find(port) != m_runRequestToPort.end()) {
				sendNegativeResponse("Another action is a process"); // 404 with message
				return;
			}

			const auto it{ m_createdAppToPort.find(port) };
			if (it == m_createdAppToPort.end()) {
				sendNegativeResponse("App with port: " + *portStr + " is not found"); // 404 with message
				return;
			}

			if (it->second.connection == 0) {
				sendNegativeResponse("App with port: " + *portStr + " is not connected yet"); // 404 with message
			}
			else {
				MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_parametersRequestsLock };
				if (const auto actionIt{ m_pauseRequestToPort.find(port) }; actionIt == m_pauseRequestToPort.end()) {
					MSAPI::StandardProtocol::SendActionPause(it->second.connection);
					SendParametersRequest<false>(port, it->second.connection,
						RequestInfo{ RequestInfo::Type::Pause, static_cast<size_t>(port) });
				}
				m_pauseRequestToPort.emplace(
					port, RequestInfo{ RequestInfo::Type::Pause, static_cast<size_t>(port), connection, data });
			}
			return;
		}

		if (*type == "run") {
			const auto* portStr{ data.GetValue("Port") };
			if (portStr == nullptr) {
				sendNegativeResponse("Key 'Port' in header is not found"); // 404 with message
				return;
			}
			if (portStr->empty()) {
				sendNegativeResponse("Key 'Port' in header is empty"); // 404 with message
				return;
			}

			uint16_t port;
			const auto error{ std::from_chars(portStr->data(), portStr->data() + portStr->size(), port).ec };
			if (error != std::errc{}) {
				sendNegativeResponse("Key 'Port' in header cannot be converted properly: " + *portStr
					+ ". Error: " + std::make_error_code(error).message()); // 404 with message
				return;
			}

			if (m_pauseRequestToPort.find(port) != m_pauseRequestToPort.end()) {
				sendNegativeResponse("Another action is a process"); // 404 with message
				return;
			}

			const auto it{ m_createdAppToPort.find(port) };
			if (it == m_createdAppToPort.end()) {
				sendNegativeResponse("App with port: " + *portStr + " is not found"); // 404 with message
				return;
			}

			if (it->second.connection == 0) {
				sendNegativeResponse("App with port: " + *portStr + " is not connected yet"); // 404 with message
			}
			else {
				MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_parametersRequestsLock };
				if (const auto actionIt{ m_runRequestToPort.find(port) }; actionIt == m_runRequestToPort.end()) {
					MSAPI::StandardProtocol::SendActionRun(it->second.connection);
					SendParametersRequest<false>(
						port, it->second.connection, RequestInfo{ RequestInfo::Type::Run, static_cast<size_t>(port) });
				}
				m_runRequestToPort.emplace(
					port, RequestInfo{ RequestInfo::Type::Run, static_cast<size_t>(port), connection, data });
			}
			return;
		}

		if (*type == "delete") {
			const auto* portStr{ data.GetValue("Port") };
			if (portStr == nullptr) {
				sendNegativeResponse("Key 'Port' in header is not found"); // 404 with message
				return;
			}
			if (portStr->empty()) {
				sendNegativeResponse("Key 'Port' in header is empty"); // 404 with message
				return;
			}

			uint16_t port;
			const auto error{ std::from_chars(portStr->data(), portStr->data() + portStr->size(), port).ec };
			if (error != std::errc{}) {
				sendNegativeResponse("Key 'Port' in header cannot be converted properly: " + *portStr
					+ ". Error: " + std::make_error_code(error).message()); // 404 with message
				return;
			}

			const auto it{ m_createdAppToPort.find(port) };
			if (it == m_createdAppToPort.end()) {
				sendNegativeResponse("App with port: " + *portStr + " is not found"); // 404 with message
				return;
			}

			if (it->second.connection == 0) {
				sendNegativeResponse("App with port: " + *portStr + " is not connected yet"); // 404 with message
			}
			else {
				MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_deleteRequestsLock };
				if (const auto actionIt{ m_deleteRequestToPort.find(port) }; actionIt == m_deleteRequestToPort.end()) {
					MSAPI::StandardProtocol::SendActionDelete(it->second.connection);
				}
				m_deleteRequestToPort.emplace(
					port, RequestInfo{ RequestInfo::Type::Delete, static_cast<size_t>(port), connection, data });
			}
			return;
		}

		if (*type == "modify") {
			const auto* portStr{ data.GetValue("Port") };
			if (portStr == nullptr) {
				sendNegativeResponse("Key 'Port' in header is not found"); // 404 with message
				return;
			}
			if (portStr->empty()) {
				sendNegativeResponse("Key 'Port' in header is empty"); // 404 with message
				return;
			}

			uint16_t port;
			const auto error{ std::from_chars(portStr->data(), portStr->data() + portStr->size(), port).ec };
			if (error != std::errc{}) {
				sendNegativeResponse("Key 'Port' in header cannot be converted properly: " + *portStr
					+ ". Error: " + std::make_error_code(error).message()); // 404 with message
				return;
			}

			const auto it{ m_createdAppToPort.find(port) };
			if (it == m_createdAppToPort.end()) {
				sendNegativeResponse("App with port: " + *portStr + " is not found"); // 404 with message
				return;
			}

			if (it->second.connection == 0) {
				sendNegativeResponse("App with port: " + *portStr + " is not connected yet"); // 404 with message
				return;
			}

			if (!it->second.appData->metadataJson.Valid()) {
				sendNegativeResponse("Metadata for app with port: " + *portStr + " is not valid"); // 404 with message
				return;
			}

			const auto* mutableParameters{ it->second.appData->metadataJson.GetValue("mutable") };
			if (mutableParameters == nullptr) {
				sendNegativeResponse("Metadata for app with port: " + *portStr
					+ " does not contain mutable parameters"); // 404 with message
				return;
			}

			const auto* mutableParametersJsonPtr{ std::get_if<MSAPI::Json>(&mutableParameters->GetValue()) };
			if (mutableParametersJsonPtr == nullptr) {
				sendNegativeResponse("Metadata for app with port: " + *portStr
					+ " contains mutable parameters, but it is not JSON"); // 404 with message
				return;
			}

			if (mutableParametersJsonPtr->GetKeysAndValues().empty()) {
				sendNegativeResponse("Metadata for app with port: " + *portStr
					+ " contains empty mutable parameters"); // 404 with message
				return;
			}

			const auto* parametersStr{ data.GetValue("Parameters") };
			if (parametersStr == nullptr) {
				sendNegativeResponse("Key 'Parameters' in header is not found"); // 404 with message
				return;
			}

			MSAPI::Json parametersJson{ *parametersStr };
			if (!parametersJson.Valid()) {
				sendNegativeResponse("Parameters JSON is not valid"); // 404 with message
				return;
			}

			MSAPI::StandardProtocol::Data parametersUpdate{ MSAPI::StandardProtocol::cipherActionModify };
			for (const auto& [keyStr, node] : parametersJson.GetKeysAndValues()) {
				size_t key{};
				const auto error{ std::from_chars(keyStr.data(), keyStr.data() + keyStr.size(), key).ec };
				if (error != std::errc{}) {
					sendNegativeResponse("Key " + keyStr + " cannot be converted properly. Error:"
						+ std::make_error_code(error).message()); // 404 with message
					return;
				}

				if (const auto* metadataItem{ mutableParametersJsonPtr->GetValue(keyStr) }; metadataItem != nullptr) {
					const auto* metadataItemValue{ std::get_if<MSAPI::Json>(&metadataItem->GetValue()) };
					if (metadataItemValue == nullptr) {
						LOG_ERROR("Metadata item for " + keyStr + " is not JSON, app with port: " + *portStr);
						continue;
					}

					const auto* parameterTypeStr{ metadataItemValue->GetValue("type") };
					if (parameterTypeStr == nullptr) {
						LOG_ERROR("Metadata item for " + keyStr + " is not have type, app with port: " + *portStr);
						continue;
					}

					const auto* parameterType{ std::get_if<std::string>(&parameterTypeStr->GetValue()) };
					if (parameterType == nullptr) {
						LOG_ERROR("Metadata item for " + keyStr + " is not string, app with port: " + *portStr);
					}

					const auto* value{ &node.GetValue() };

#define TMP_MANAGER_TRY_SET_DATA_PARAMETER(type, jsonType)                                                             \
	if (const auto* param{ std::get_if<jsonType>(value) }; param != nullptr) {                                         \
		parametersUpdate.SetData(key, static_cast<type>(*param));                                                      \
		continue;                                                                                                      \
	}

#define TMP_MANAGER_CONTINUE_WITH_ERROR                                                                                \
	LOG_ERROR("Update for parameter " + keyStr + " is not a valid type, parameter type: " + *parameterType             \
		+ ", app with port: " + *portStr);

					if (*parameterType == "Int8") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int8_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int8_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Int16") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int16_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int16_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Bool") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(bool, bool);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Int32") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int32_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int32_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Float") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(float, double);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(float, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(float, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Timer") {
						if (const auto* param{ std::get_if<uint64_t>(value) }; param != nullptr) {
							parametersUpdate.SetData(
								key, MSAPI::Timer{ INT64(*param) / 1000000000, INT64(*param) % 1000000000 });
							continue;
						}
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Double") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(double, double);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(double, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(double, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "String") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(std::string, std::string);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Duration") {
						if (const auto* param{ std::get_if<uint64_t>(value) }; param != nullptr) {
							parametersUpdate.SetData(key, MSAPI::Timer::Duration{ INT64(*param) });
							continue;
						}
						if (const auto* param{ std::get_if<int64_t>(value) }; param != nullptr) {
							parametersUpdate.SetData(key, MSAPI::Timer::Duration{ INT64(*param) });
							continue;
						}
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "TableData") {
						if (const auto* param{ std::get_if<std::list<MSAPI::JsonNode>>(value) }; param != nullptr) {
							const auto tableColumnsIt{ m_columnsToTableId.find(key) };
							if (tableColumnsIt == m_columnsToTableId.end()) {
								LOG_ERROR("Columns for table with id: " + _S(key)
									+ " are not found, app with port: " + *portStr);
								continue;
							}

							if (param->empty()) {
								parametersUpdate.SetData(key, MSAPI::TableData{});
								continue;
							}

							const MSAPI::TableData table{ *param, tableColumnsIt->second };
							if (table.GetBufferSize() == sizeof(size_t) /* something went wrong */) {
								continue;
							}

							parametersUpdate.SetData(key, std::move(table));
							continue;
						}
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Int64") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int64_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(int64_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalInt8") {

#define TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(type)                                                        \
	if (std::holds_alternative<std::nullptr_t>(*value)) {                                                              \
		parametersUpdate.SetData(key, std::optional<type>{});                                                          \
		continue;                                                                                                      \
	}

#define TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(type, jsonType)                                                    \
	if (const auto* param{ std::get_if<jsonType>(value) }; param != nullptr) {                                         \
		parametersUpdate.SetData(key, std::optional<type>{ static_cast<type>(*param) });                               \
		continue;                                                                                                      \
	}

						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int8_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int8_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int8_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Uint8") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint8_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalInt16") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int16_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int16_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int16_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Uint16") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint16_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Uint32") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint32_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalInt32") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int32_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int32_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int32_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalFloat") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(float);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(float, double);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(float, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(float, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalDouble") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(double);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(double, double);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(double, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(double, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "Uint64") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint64_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalInt64") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int64_t, uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int64_t, int64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalUint8") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint8_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint8_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalUint16") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint16_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint16_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalUint32") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint32_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint32_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else if (*parameterType == "OptionalUint64") {
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint64_t);
						TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint64_t, uint64_t);
						TMP_MANAGER_CONTINUE_WITH_ERROR;
					}
					else {
						LOG_ERROR("Broken metadata, unknown or unsupported type of parameter \"" + *parameterType
							+ "\" in metadata for app with port: " + *portStr);
					}
				}
				else {
					LOG_DEBUG(
						"Metadata item for " + keyStr + " is not found or it is const, app with port: " + *portStr);
				}
			}

#undef TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL
#undef TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL
#undef TMP_MANAGER_CONTINUE_WITH_ERROR
#undef TMP_MANAGER_TRY_SET_DATA_PARAMETER

			if (parametersUpdate.GetBufferSize() > sizeof(size_t) * 2) {
				MSAPI::StandardProtocol::Send(it->second.connection, parametersUpdate);
				data.SendResponse(connection, "{\"status\":true}\n\n");
			}
			else {
				sendNegativeResponse("No parameters to update"); // 404 with message
			}

			return;
		}

		sendNegativeResponse("Key by Type in header is unknown: " + *type); // 404 with message
		return;
	}

	data.Send404(connection);
}

void Manager::HandleRunRequest()
{
	MSAPI_HANDLE_RUN_REQUEST_PRESET

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	path += "apps.json";

	if (!MSAPI::Bin::HasFile(path)) {
		LOG_ERROR_NEW("\"{}\" file is not found", path);
		HandlePauseRequest();
		return;
	}

	std::string apps;
	MSAPI::Bin::ReadStr(apps, path);
	MSAPI::Json appsJson{ apps };

	const auto* appsArray{ appsJson.GetValue("Apps") };
	if (!appsJson.Valid() || appsArray == nullptr || !appsArray->Valid()) {
		LOG_ERROR("apps.json is not valid");
		HandlePauseRequest();
		return;
	}

	const auto* value{ std::get_if<std::list<MSAPI::JsonNode>>(&appsArray->GetValue()) };
	if (value == nullptr) {
		LOG_ERROR("apps.json does not contain an array");
		HandlePauseRequest();
		return;
	}

	LOG_INFO("List of apps is applied: " + appsJson.ToString());

	size_t appId{};
	for (const auto& node : *value) {
		const auto* appNode{ std::get_if<MSAPI::Json>(&node.GetValue()) };
		if (appNode == nullptr) {
			LOG_WARNING("App node is not JSON");
			continue;
		}

		const auto* app{ appNode->GetValue("App") };
		const auto* bin{ appNode->GetValue("Bin") };

		if (app == nullptr || bin == nullptr) {
			LOG_WARNING("App or bin is not found");
			continue;
		}

		const auto* appValue{ std::get_if<std::string>(&app->GetValue()) };
		const auto* binValue{ std::get_if<std::string>(&bin->GetValue()) };

		if (appValue == nullptr || binValue == nullptr) {
			LOG_WARNING("App or bin is not string");
			continue;
		}

		const auto* view{ appNode->GetValue("View") };

		const uint64_t* viewValue{ [view]() -> const uint64_t* {
			if (view == nullptr) {
				return nullptr;
			}

			const auto* viewValue{ std::get_if<uint64_t>(&view->GetValue()) };
			if (viewValue == nullptr) {
				LOG_WARNING("View is not uint64_t");
				return nullptr;
			}

			return viewValue;
		}() };

		appId = MSAPI::Helper::stringHasher(*appValue);
		if (auto it{ m_installedAppDataToHash.find(appId) }; it == m_installedAppDataToHash.end()) {

			if (viewValue == nullptr) {
				LOG_INFO("New app registered: " + *appValue + ", id: " + _S(appId) + ", bin: " + *binValue);
				m_installedAppDataToHash.emplace(std::move(appId), InstalledAppData{ *appValue, *binValue });
			}
			else {
				LOG_INFO("New app registered: " + *appValue + ", id: " + _S(appId) + ", bin: " + *binValue
					+ ", parameter with port for view: " + _S(static_cast<int32_t>(*viewValue)));
				m_installedAppDataToHash.emplace(
					std::move(appId), InstalledAppData{ *appValue, *binValue, static_cast<int32_t>(*viewValue) });
			}
		}
		else if (it->second.bin != *binValue) {
			LOG_INFO("For app: " + *appValue + " bin path changed from " + it->second.bin + " to " + *binValue);
			it->second.bin = *binValue;
		}
		else if (viewValue != nullptr && it->second.viewPortParameter != static_cast<int32_t>(*viewValue)) {
			LOG_INFO("For app: " + *appValue + " view port changed from " + _S(it->second.viewPortParameter) + " to "
				+ _S(static_cast<int32_t>(*viewValue)));
			it->second.viewPortParameter = static_cast<int32_t>(*viewValue);
		}
	}

	if (m_installedAppDataToHash.empty()) {
		LOG_ERROR("No apps registered, manager is going to end its work");
		HandlePauseRequest();
		Server::Stop();
		return;
	}
}

void Manager::HandlePauseRequest()
{
	MSAPI_HANDLE_PAUSE_REQUEST_PRESET

	const char* message{ "{\"status\":false,\"message\":\"Manager is paused\"}\n\n" };

	{
		MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_parametersRequestsLock };
		for (auto& [port, requests] : m_parametersRequestsToPort) {
			for (const auto& request : requests) {
				const auto& requestData{ request.GetData().value() };
				requestData.data.SendResponse(requestData.connection, message);
			}
		}

		m_parametersRequestsToPort.clear();
		for (auto& [port, request] : m_pauseRequestToPort) {
			const auto& requestData{ request.GetData().value() };
			requestData.data.SendResponse(requestData.connection, message);
		}

		m_pauseRequestToPort.clear();
		for (auto& [port, request] : m_runRequestToPort) {
			const auto& requestData{ request.GetData().value() };
			requestData.data.SendResponse(requestData.connection, message);
		}
		m_runRequestToPort.clear();
	}

	{
		MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_deleteRequestsLock };
		for (auto& [port, request] : m_deleteRequestToPort) {
			const auto& requestData{ request.GetData().value() };
			requestData.data.SendResponse(requestData.connection, "{\"status\":true}\n\n");
		}
		m_deleteRequestToPort.clear();
	}

	{
		MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_metadataRequestsLock };
		for (auto& [hash, request] : m_metadataRequestsToHash) {
			const auto& requestData{ request.GetData().value() };
			requestData.data.SendResponse(requestData.connection, message);
		}
		m_metadataRequestsToHash.clear();
	}

	{
		MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_createdAppToPortLock };
		m_createdAppToPort.clear();
	}

	m_installedAppDataToHash.clear();
}

void Manager::HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	for (const auto& [id, value] : parametersUpdate) {
		if (MSAPI::Application::IsRunning()) {
			if (id == 1001) {
				LOG_WARNING_NEW(
					"Change web sources path from: {} is interrupted, because of application is in a running state",
					m_webSourcesPath);
				continue;
			}
		}

		MSAPI::Application::MergeParameter(id, value);
	}

	if (!MSAPI::Application::AreParametersValid()) {
		HandlePauseRequest();
	}
}

void Manager::HandleParameters(const int connection, const std::map<size_t, std::variant<standardTypes>>& parameters)
{
	if (const auto portIt{ parameters.find(1000009) }; portIt != parameters.end()) {
		const uint16_t* port{ std::get_if<uint16_t>(&portIt->second) };
		if (port == nullptr) {
			LOG_ERROR("Port type is unexpected");
			return;
		}

		auto createdAppDataIt{ m_createdAppToPort.find(*port) };
		if (createdAppDataIt == m_createdAppToPort.end()) {
			LOG_ERROR("App with port: " + _S(port) + " is not found");
			return;
		}

		if (createdAppDataIt->second.connection == 0) {
			LOG_DEBUG("First parameters update from app: " + createdAppDataIt->second.appData->type
				+ ", id: " + _S(createdAppDataIt->first) + ", port: " + _S(port));
			createdAppDataIt->second.connection = connection;

			if (createdAppDataIt->second.appData->metadata.empty()) {
				MSAPI::StandardProtocol::SendMetadataRequest(connection);
			}
		}
		else {
			LOG_DEBUG("Parameters update from app: " + createdAppDataIt->second.appData->type
				+ ", id: " + _S(createdAppDataIt->first) + ", port: " + _S(port));
		}

		MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_parametersRequestsLock };
		if (auto requestsIt{ m_parametersRequestsToPort.find(*port) }; requestsIt != m_parametersRequestsToPort.end()) {
			const auto& requestInfo{ requestsIt->second.front() };
			const auto requestType{ requestInfo.GetType() };
			switch (requestType) {
			case RequestInfo::Type::Parameters: {
				const auto parseParameters{ [&parameters, this]() {
					std::string response{ "{\"status\":true,\"parameters\":{" };
					for (const auto& [id, value] : parameters) {
						response += "\"" + _S(id) + "\":";
						std::visit(
							[&response, &id, this](auto&& arg) {
								using T = std::remove_const_t<std::remove_reference_t<std::decay_t<decltype(arg)>>>;
								if constexpr (MSAPI::is_integer_type<T>) {
									response += std::format("{}", arg);
								}
								else if constexpr (std::is_same_v<T, bool>) {
									response += _S(arg);
								}
								else if constexpr (MSAPI::is_integer_type_optional<T>) {
									if (arg.has_value()) {
										response += std::format("{}", arg.value());
									}
									else {
										response += "null";
									}
								}
								else if constexpr (MSAPI::is_float_type<T>) {
									response += _S(arg);
								}
								else if constexpr (MSAPI::is_float_type_optional<T>) {
									if (arg.has_value()) {
										response += _S(arg.value());
									}
									else {
										response += "null";
									}
								}
								else if constexpr (std::is_same_v<T, std::string>) {
									response += "\"" + arg + "\"";
								}
								else if constexpr (std::is_same_v<T, MSAPI::Timer>) {
									response += std::format("{}", arg.GetNanoseconds());
								}
								else if constexpr (std::is_same_v<T, MSAPI::Timer::Duration>) {
									response += std::format("{}", arg.GetNanoseconds());
								}
								else if constexpr (std::is_same_v<T, MSAPI::TableData>) {
									if (const auto it{ m_columnsToTableId.find(id) }; it != m_columnsToTableId.end()) {
										const std::string json{ arg.LookUpToJson(it->second) };
										if (json.empty()) {
											response += "\"\"";
										}
										else {
											response += json;
										}
									}
									else {
										LOG_ERROR("Columns for table with id: " + _S(id) + " are not found");
										response += "\"\"";
									}
								}
								else {
									static_assert(sizeof(T) + 1 == 0, "Unsupported type of parameter");
								}
							},
							value);
						response += ",";
					}
					response.pop_back();
					return response + "}}\n\n";
				}() };
				//* Send a response exactly to an initial request
				const auto& requestData{ requestInfo.GetData().value() };
				requestData.data.SendResponse(requestData.connection, parseParameters);
			} break;

#define TMP_MSAPI_MANAGER_HANDLE_ACTION_RESPONSE(container, expectedState)                                             \
	if (state != nullptr) {                                                                                            \
		auto [current, end] = container.equal_range(*port);                                                            \
		const std::string message{ "{\"status\":true,\"result\":" + _S(*state == expectedState) + "}\n\n" };           \
		while (current != end) {                                                                                       \
			const auto& requestData{ current->second.GetData().value() };                                              \
			requestData.data.SendResponse(requestData.connection, message);                                            \
			++current;                                                                                                 \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		LOG_ERROR("State of application is unexpected");                                                               \
	}                                                                                                                  \
                                                                                                                       \
	container.erase(*port);

			case RequestInfo::Type::Pause:
				if (const auto stateIt{ parameters.find(2000002) }; stateIt != parameters.end()) {
					const auto* state{ std::get_if<std::underlying_type_t<MSAPI::Application::State>>(
						&stateIt->second) };
					TMP_MSAPI_MANAGER_HANDLE_ACTION_RESPONSE(m_pauseRequestToPort, MSAPI::Application::State::Paused);

					m_runRequestToPort.erase(*port);
				}
				break;
			case RequestInfo::Type::Run:
				if (const auto stateIt{ parameters.find(2000002) }; stateIt != parameters.end()) {
					const auto* state{ std::get_if<std::underlying_type_t<MSAPI::Application::State>>(
						&stateIt->second) };
					TMP_MSAPI_MANAGER_HANDLE_ACTION_RESPONSE(m_runRequestToPort, MSAPI::Application::State::Running);

					m_runRequestToPort.erase(*port);
				}
				break;

#undef TMP_MSAPI_MANAGER_HANDLE_ACTION_RESPONSE

			default:
				LOG_ERROR("Unexpected type of parameters request: " + _S(U(requestType)));
				break;
			}

			requestsIt->second.pop_front();
			if (requestsIt->second.empty()) {
				m_parametersRequestsToPort.erase(requestsIt);
			}
		}

		return;
	}

	LOG_WARNING("Parameters update without port, connection: " + _S(connection));
}

void Manager::HandleHello(const int connection) { MSAPI::StandardProtocol::SendParametersRequest(connection); }

void Manager::HandleMetadata(const int connection, const std::string_view metadata)
{
	auto it{ std::ranges::find_if(m_createdAppToPort,
		[connection](const auto& createdAppData) { return createdAppData.second.connection == connection; }) };
	if (it == m_createdAppToPort.end()) {
		LOG_ERROR("Metadata update from unknown app, connection: " + _S(connection));
		return;
	}

	LOG_DEBUG("Metadata update for app: " + it->second.appData->type);
	auto appDataIt{ m_installedAppDataToHash.find(MSAPI::Helper::stringHasher(it->second.appData->type)) };
	if (appDataIt == m_installedAppDataToHash.end()) {
		LOG_ERROR("App: " + it->second.appData->type + " is not found");
		return;
	}

	if (!appDataIt->second.metadata.empty()) {
		LOG_WARNING("Metadata for app: " + it->second.appData->type + " is already handled");
		return;
	}

	appDataIt->second.metadata = metadata;
	appDataIt->second.metadataJson.Construct(metadata);
	if (!appDataIt->second.metadataJson.Valid()) {
		LOG_ERROR("Metadata for app: " + it->second.appData->type + " is not valid");
		return;
	}

	const auto parseTables{ [this, connection](const MSAPI::Json* const parameters) {
		for (const auto& [keyStr, node] : parameters->GetKeysAndValues()) {
			const auto* nodeValue{ std::get_if<MSAPI::Json>(&node.GetValue()) };
			if (nodeValue == nullptr) {
				continue;
			}

			const auto* typeNode{ nodeValue->GetValue("type") };
			if (typeNode == nullptr) {
				continue;
			}

			const auto* type{ std::get_if<std::string>(&typeNode->GetValue()) };
			if (type == nullptr || *type != "TableData") {
				continue;
			}

			const auto* columnsNode{ nodeValue->GetValue("columns") };
			if (columnsNode == nullptr) {
				continue;
			}

			const auto* columns{ std::get_if<MSAPI::Json>(&columnsNode->GetValue()) };
			if (columns == nullptr) {
				continue;
			}

			size_t tableId{};
			const auto error{ std::from_chars(keyStr.data(), keyStr.data() + keyStr.size(), tableId).ec };
			if (error != std::errc{}) {
				LOG_ERROR("Broken metadata, table id cannot be converted properly. Error: "
					+ std::make_error_code(error).message());
				continue;
			}

			if (m_columnsToTableId.find(tableId) != m_columnsToTableId.end()) {
				continue;
			}

			std::vector<MSAPI::StandardType::Type> columnTypes;
			columnTypes.reserve(columns->GetKeysAndValues().size());
			for (const auto& [columnId, columnMetadataNode] : columns->GetKeysAndValues()) {
				const auto* columnMetadata{ std::get_if<MSAPI::Json>(&columnMetadataNode.GetValue()) };
				if (columnMetadata == nullptr) {
					LOG_ERROR("Broken metadata, impossible to find metadata of column id " + columnId
						+ " in table id: " + _S(tableId) + ", connection: " + _S(connection)
						+ ", columns metadata: " + columns->ToString());
					continue;
				}

				const auto* type{ columnMetadata->GetValue("type") };
				if (type == nullptr) {
					LOG_ERROR("Broken metadata, impossible to find type of column id " + columnId
						+ " in table id: " + _S(tableId) + ", connection: " + _S(connection)
						+ ", column metadata: " + columnMetadata->ToString());
					continue;
				}

				const auto* typeValue{ std::get_if<std::string>(&type->GetValue()) };
				if (typeValue == nullptr) {
					LOG_ERROR("Broken metadata, wrong type of column type, table id: " + _S(tableId)
						+ ", connection: " + _S(connection) + ", column metadata: " + columnMetadata->ToString());
					continue;
				}

				if (*typeValue == "Int8") {
					columnTypes.push_back(MSAPI::StandardType::Type::Int8);
				}
				else if (*typeValue == "Int16") {
					columnTypes.push_back(MSAPI::StandardType::Type::Int16);
				}
				else if (*typeValue == "Int32") {
					columnTypes.push_back(MSAPI::StandardType::Type::Int32);
				}
				else if (*typeValue == "Int64") {
					columnTypes.push_back(MSAPI::StandardType::Type::Int64);
				}
				else if (*typeValue == "Float") {
					columnTypes.push_back(MSAPI::StandardType::Type::Float);
				}
				else if (*typeValue == "Timer") {
					columnTypes.push_back(MSAPI::StandardType::Type::Timer);
				}
				else if (*typeValue == "Double") {
					columnTypes.push_back(MSAPI::StandardType::Type::Double);
				}
				else if (*typeValue == "String") {
					columnTypes.push_back(MSAPI::StandardType::Type::String);
				}
				else if (*typeValue == "Duration") {
					columnTypes.push_back(MSAPI::StandardType::Type::Duration);
				}
				else if (*typeValue == "Uint8") {
					columnTypes.push_back(MSAPI::StandardType::Type::Uint8);
				}
				else if (*typeValue == "OptionalInt8") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalInt8);
				}
				else if (*typeValue == "Uint16") {
					columnTypes.push_back(MSAPI::StandardType::Type::Uint16);
				}
				else if (*typeValue == "OptionalInt16") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalInt16);
				}
				else if (*typeValue == "Uint32") {
					columnTypes.push_back(MSAPI::StandardType::Type::Uint32);
				}
				else if (*typeValue == "Uint64") {
					columnTypes.push_back(MSAPI::StandardType::Type::Uint64);
				}
				else if (*typeValue == "OptionalInt32") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalInt32);
				}
				else if (*typeValue == "OptionalFloat") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalFloat);
				}
				else if (*typeValue == "OptionalDouble") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalDouble);
				}
				else if (*typeValue == "OptionalInt64") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalInt64);
				}
				else if (*typeValue == "OptionalUint8") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalUint8);
				}
				else if (*typeValue == "OptionalUint16") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalUint16);
				}
				else if (*typeValue == "OptionalUint32") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalUint32);
				}
				else if (*typeValue == "OptionalUint64") {
					columnTypes.push_back(MSAPI::StandardType::Type::OptionalUint64);
				}
				else if (*typeValue == "Bool") {
					columnTypes.push_back(MSAPI::StandardType::Type::Bool);
				}
				else {
					LOG_ERROR("Broken metadata, unknown or unsupported type of column \"" + *typeValue + "\" id "
						+ columnId + " in table id: " + _S(tableId) + ", connection: " + _S(connection)
						+ ", column metadata: " + columnMetadata->ToString());
				}
			}

			if (!columnTypes.empty()) {
				m_columnsToTableId.emplace(tableId, std::move(columnTypes));
				LOG_DEBUG("Columns for table with id: " + _S(tableId) + " are found, connection: " + _S(connection));
			}
			else {
				LOG_ERROR("Broken metadata, impossible to find any column type in table id: " + _S(tableId)
					+ ", connection: " + _S(connection) + ", JSON: " + node.ToString());
			}
		}
	} };

	if (const auto* mutableParameters{ appDataIt->second.metadataJson.GetValue("mutable") };
		mutableParameters != nullptr) {

		if (const auto* parametersValue{ std::get_if<MSAPI::Json>(&mutableParameters->GetValue()) };
			parametersValue != nullptr) {

			parseTables(parametersValue);
		}
		else {
			LOG_ERROR("Mutable parameters for app: " + it->second.appData->type + " are not JSON");
		}
	}

	if (const auto* constParameters{ appDataIt->second.metadataJson.GetValue("const") }; constParameters != nullptr) {
		if (const auto* parametersValue{ std::get_if<MSAPI::Json>(&constParameters->GetValue()) };
			parametersValue != nullptr) {

			parseTables(parametersValue);
		}
		else {
			LOG_ERROR("Const parameters for app: " + it->second.appData->type + " are not JSON");
		}
	}

	if (auto range{ m_metadataRequestsToHash.equal_range(appDataIt->first) }; range.first != range.second) {
		const std::string message{ "{\"status\":true,\"metadata\":" + appDataIt->second.metadata + "}\n\n" };
		while (range.first != range.second) {
			const auto& requestData{ range.first->second.GetData().value() };
			requestData.data.SendResponse(requestData.connection, message);
			++range.first;
		}
		m_metadataRequestsToHash.erase(appDataIt->first);
	}
}

uint16_t Manager::CreateApp(
	const std::map<size_t, InstalledAppData>::iterator& appDataIt, const MSAPI::HTTP::Data& data, std::string& error)
{
	LOG_DEBUG(
		"Creating app: " + appDataIt->second.type + ", id: " + _S(appDataIt->first) + " from " + appDataIt->second.bin);

	unsigned int ip{ htonl(INADDR_LOOPBACK) };
	if (const auto* ipStr{ data.GetValue("ip") }; ipStr != nullptr && !ipStr->empty()) {
		if (!MSAPI::Helper::ValidateIpv4(ipStr->c_str())) {
			error = "Invalid ip in http request: " + *ipStr;
			return 0;
		}

		ip = inet_addr(ipStr->c_str());
		if (ip == INADDR_NONE) {
			error = "Invalid IP address: " + *ipStr;
			return 0;
		}
	}

	uint16_t port{ 0 };
	if (const auto* portStr{ data.GetValue("port") }; portStr != nullptr && *portStr != "0" && !portStr->empty()) {
		const auto result{ std::from_chars(portStr->data(), portStr->data() + portStr->size(), port).ec };
		if (result != std::errc{}) {
			error = "Broken port in http request: " + *portStr + ". Error: " + std::make_error_code(result).message();
			return 0;
		}
	}
	else {
		size_t counter{ 0 };
		do {
			port = static_cast<uint16_t>(MSAPI::Identifier::mersenne() % (65535 - 3000) + 3000);
			if (m_createdAppToPort.find(port) == m_createdAppToPort.end()) {
				break;
			}

			if (++counter >= 50000) {
				error = "Cannot generate an unique port for app: " + appDataIt->second.type
					+ ", id: " + _S(appDataIt->first);
				LOG_ERROR(error);
				return 0;
			}
		} while (true);
	}

	std::string parentPath;
	if (const auto parentPathStr{ data.GetValue("parentPath") }; parentPathStr != nullptr && !parentPathStr->empty()) {
		parentPath = *parentPathStr;
	}
	else {
		if (std::string::size_type pos = appDataIt->second.bin.rfind("build/"); pos != std::string::npos) {
			parentPath = std::string{ appDataIt->second.bin.begin(), appDataIt->second.bin.begin() + INT64(pos) };
		}
		else {
			pos = appDataIt->second.bin.rfind("/");
			if (pos != std::string::npos) {
				parentPath = std::string{ appDataIt->second.bin.begin(), appDataIt->second.bin.begin() + INT64(pos) };
			}
			else {
				error = "Invalid bin path in http request: " + appDataIt->second.bin;
				return 0;
			}
		}
	}

	short logLevel{ static_cast<short>(MSAPI::Log::Level::WARNING) };
	if (const auto logLevelStr{ data.GetValue("logLevel") }; logLevelStr != nullptr && !logLevelStr->empty()) {

		const auto result{
			std::from_chars(logLevelStr->data(), logLevelStr->data() + logLevelStr->size(), logLevel).ec
		};
		if (result != std::errc{}) {
			error = "Broken log level in http request: " + *logLevelStr
				+ ". Error: " + std::make_error_code(result).message();
			return 0;
		}

		if (logLevel < static_cast<short>(MSAPI::Log::Level::ERROR)
			|| logLevel > static_cast<short>(MSAPI::Log::Level::PROTOCOL)) {

			error = "Invalid log level in http request: " + *logLevelStr;
			return 0;
		}
	}

	bool logInConsole{ false };
	if (const auto logInConsoleStr{ data.GetValue("logInConsole") }; logInConsoleStr != nullptr) {
		logInConsole = *logInConsoleStr == "true";
	}

	bool logInFile{ false };
	if (const auto logInFileStr{ data.GetValue("logInFile") }; logInFileStr != nullptr) {
		logInFile = *logInFileStr == "true";
	}

	bool separateDaysLogging{ true };
	if (const auto* separateDaysLoggingStr{ data.GetValue("separateDaysLogging") }; separateDaysLoggingStr != nullptr) {
		separateDaysLogging = *separateDaysLoggingStr == "false";
	}

	std::string name;
	if (const auto* nameStr{ data.GetValue("name") }; nameStr != nullptr) {
		name = *nameStr;
	}
	else {
		name = appDataIt->second.type;
	}

	MSAPI::Json parameters{ "{\"name\":\"" + name + "\",\"ip\":\"" + _S(ip) + "\",\"port\":\"" + _S(port)
		+ "\",\"managerPort\":\"" + _S(GetListenedPort()) + "\"	,\"parentPath\":\"" + parentPath + "\",\"logLevel\":\""
		+ _S(logLevel) + "\",\"logInConsole\":\"" + _S(logInConsole) + "\",\"logInFile\":\"" + _S(logInFile)
		+ "\",\"separateDaysLogging\":\"" + _S(separateDaysLogging) + "\"}" };

	if (!parameters.Valid()) {
		error = "Parameters are not valid";
		LOG_ERROR(error);
		return 0;
	}
	LOG_DEBUG("Parameters: " + parameters.ToString());

	const pid_t pid{ vfork() };
	if (pid < 0) {
		error = "Can't vfork for app: " + appDataIt->second.type + ", id: " + _S(appDataIt->first) + " from "
			+ appDataIt->second.bin + ". Error №" + _S(errno) + ": " + std::strerror(errno);
		LOG_ERROR(error);
		return 0;
	}

	//* Parent process
	if (pid > 0) {
		LOG_INFO("App: " + appDataIt->second.type + ", id: " + _S(appDataIt->first) + " created with pid: " + _S(pid));
		MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_createdAppToPortLock };
		m_createdAppToPort.emplace(port, CreatedAppData{ appDataIt->first, pid, &appDataIt->second });
		return port;
	}

	//* Child process
	if (pid == 0) {
		if (setsid() == -1) {
			perror("setsid");
			exit(EXIT_FAILURE);
		}
		//* Close all file descriptors from 3 to maximum open file descriptors. 0 (stdin), 1 (stdout), 2 (stderr)
		const auto max{ sysconf(_SC_OPEN_MAX) };
		for (int fd{ 3 }; fd < max; fd++) {
			if (close(fd) == -1 && errno != EBADF) {
				//* If close() failed for a reason other than the file descriptor not being open
				perror("close");
				exit(EXIT_FAILURE);
			}
		}

		if (execl("/bin/bash", "/bin/bash", "-c",
				std::string{ appDataIt->second.bin + " '" + parameters.ToJson() + "'" }.c_str(), nullptr)
			== -1) {

			perror("execl");
			exit(EXIT_FAILURE);
		}
	}

	//* This return statement should never be reached, but it's here to prevent a warning
	return 0;
}

void Manager::CheckVforkedApps()
{
	pid_t result;
	int status;

	MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_createdAppToPortLock };
	for (auto it{ m_createdAppToPort.begin() }; it != m_createdAppToPort.end();) {
		result = waitpid(it->second.pid, nullptr, WNOHANG);
		if (result == 0) {
			++it;
			continue;
		}

		if (result == -1) {
			LOG_WARNING("Waitpid for app: " + it->second.appData->type + ", port: " + _S(it->first)
				+ ", pid: " + _S(it->second.pid) + ". Error №" + _S(errno) + ": " + std::strerror(errno));
		}
		else {
			if (WIFEXITED(status)) {
				LOG_INFO("App: " + it->second.appData->type + ", port: " + _S(it->first)
					+ " with pid: " + _S(it->second.pid) + " is terminated with status: " + _S(WEXITSTATUS(status)));
			}
			else {
				LOG_INFO("App: " + it->second.appData->type + ", port: " + _S(it->first)
					+ " with pid: " + _S(it->second.pid) + " is terminated");
			}
		}

		{
			MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_parametersRequestsLock };
			if (auto requestsIt{ m_parametersRequestsToPort.find(it->first) };
				requestsIt != m_parametersRequestsToPort.end()) {
				const std::string message{ "{\"status\":false,\"message\":\"App is terminated\"}\n\n" };
				while (!requestsIt->second.empty()) {
					const auto& data{ requestsIt->second.front().GetData() };
					if (data.has_value()) {
						const auto& dataValue{ data.value() };
						dataValue.data.SendResponse(dataValue.connection, message);
					}
					requestsIt->second.pop_front();
				}
				m_parametersRequestsToPort.erase(requestsIt);
			}

			{
				auto [current, end] = m_pauseRequestToPort.equal_range(it->first);
				if (current != end) {
					do {
						const auto& requestData{ current->second.GetData().value() };
						requestData.data.SendResponse(requestData.connection, "{\"status\":false}\n\n");
					} while (++current != end);
					m_pauseRequestToPort.erase(it->first);
				}
			}

			{
				auto [current, end] = m_runRequestToPort.equal_range(it->first);
				if (current != end) {
					do {
						const auto& requestData{ current->second.GetData().value() };
						requestData.data.SendResponse(requestData.connection, "{\"status\":false}\n\n");
					} while (++current != end);
					m_runRequestToPort.erase(it->first);
				}
			}
		}

		{
			MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_deleteRequestsLock };
			auto [current, end] = m_deleteRequestToPort.equal_range(it->first);
			if (current != end) {
				do {
					const auto& requestData{ current->second.GetData().value() };
					requestData.data.SendResponse(requestData.connection, "{\"status\":true}\n\n");
				} while (++current != end);
				m_deleteRequestToPort.erase(it->first);
			}
		}

		size_t typeHash{ it->second.hash };
		it = m_createdAppToPort.erase(it);
		{
			if (std::none_of(m_createdAppToPort.begin(), m_createdAppToPort.end(),
					[typeHash](const auto& createdAppData) { return createdAppData.second.hash == typeHash; })) {

				MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_metadataRequestsLock };
				auto [current, end] = m_metadataRequestsToHash.equal_range(typeHash);
				if (current != end) {
					do {
						const auto& requestData{ current->second.GetData().value() };
						requestData.data.SendResponse(requestData.connection,
							"{\"status\":false,\"message\":\"App is terminated, metadata is not available\"}\n\n");
						++current;
					} while (current != end);
					m_metadataRequestsToHash.erase(typeHash);
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------------
CreatedAppData
---------------------------------------------------------------------------------*/

Manager::InstalledAppData::InstalledAppData(const std::string& type, const std::string& bin)
	: hasView{ false }
	, viewPortParameter{}
	, type{ type }
	, bin{ bin }
{
}

/*---------------------------------------------------------------------------------
CreatedAppData
---------------------------------------------------------------------------------*/

Manager::CreatedAppData::CreatedAppData(const size_t hash, const int pid, const InstalledAppData* appData)
	: hash{ hash }
	, pid{ pid }
	, appData{ appData }
{
}