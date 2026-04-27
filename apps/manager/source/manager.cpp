/**************************
 * @file        manager.cpp
 * @version     6.0
 * @date        2024-02-21
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
#include "../../../library/source/help/io.inl"
#include <algorithm>
#include <sys/wait.h>

Manager::Manager()
	: MSAPI::Protocol::HTTP::IHandler(this)
	, MSAPI::Protocol::WebSocket::IHandler(this)
{
	if (access("/bin/bash", X_OK) != 0) {
		LOG_ERROR("Can't access to /bin/bash, terminate application");
		Server::Stop();
		return;
	}

	RegisterParameter(1001, { "Web sources path", &m_webSourcesPath });

	m_singlesDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("installedApp"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->FillInstalledApps(out, single);
		});
	m_singlesDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("getMetadata"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->FillMetadata(out, single);
		});
	m_singlesDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("register"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->Register(out, single);
		});
	m_singlesDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("login"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->Login(out, single);
		});
	m_singlesDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("logout"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->Logout(out, single);
		});
	m_singlesDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("modifyAccount"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->ModifyAccount(out, single);
		});
	m_singlesDistributor.SetHandlerWithPermissions(
		MSAPI::Helper::StringHashDjb2("createApp"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->CreateApp(out, single);
		},
		MSAPI::Authorization::Base::Grade::User);
	m_singlesDistributor.SetHandlerWithPermissions(
		MSAPI::Helper::StringHashDjb2("pause"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->PauseApp(out, single);
		},
		MSAPI::Authorization::Base::Grade::User);
	m_singlesDistributor.SetHandlerWithPermissions(
		MSAPI::Helper::StringHashDjb2("run"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->RunApp(out, single);
		},
		MSAPI::Authorization::Base::Grade::User);
	m_singlesDistributor.SetHandlerWithPermissions(
		MSAPI::Helper::StringHashDjb2("delete"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->DeleteApp(out, single);
		},
		MSAPI::Authorization::Base::Grade::User);
	m_singlesDistributor.SetHandlerWithPermissions(
		MSAPI::Helper::StringHashDjb2("modify"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single) {
			return this->ModifyApp(out, single);
		},
		MSAPI::Authorization::Base::Grade::User);

	m_streamsDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("createdApps"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Stream& stream) {
			return this->FillCreatedApps(out, stream);
		});
	m_streamsDistributor.SetHandlerWithoutPermissions(MSAPI::Helper::StringHashDjb2("parameters"),
		[this](std::string& out, const MSAPI::Protocol::WebSocket::Events::Stream& stream) {
			return this->FillAppParameters(out, stream);
		});
}

void Manager::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI_HANDLER_WEBSOCKET_PRESET;

	MSAPI::DataHeader header{ *recvBufferInfo->buffer };
	MSAPI_HANDLER_HTTP_PRESET;

	LOG_ERROR("Unknown protocol: " + header.ToString());
}

void Manager::HandleHttp(const int connection, const MSAPI::Protocol::HTTP::Data& data)
{
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

	data.Send404(connection);
}

void Manager::HandleWebSocket(const int32_t connection, MSAPI::Protocol::WebSocket::Data&& data)
{
	auto payload{ data.GetPayload() };
	MSAPI::Json json{ std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()) };
	if (!json.Valid()) {
		return;
	}

	const auto* uid{ json.GetValueType<uint64_t>("uid") };
	if (uid == nullptr) {
		LOG_DEBUG_NEW("Json does not contain \"uid\" {}", json.ToString());
		return;
	}

	const auto* event{ json.GetValueType<uint64_t>("event") };
	if (event == nullptr) {
		LOG_DEBUG_NEW("Json does not contain \"event\" {}", json.ToString());
		return;
	}

	const auto* type{ json.GetValueType<uint64_t>("type") };
	if (type == nullptr) {
		LOG_DEBUG_NEW("Json does not contain \"type\" {}", json.ToString());
		return;
	}

	switch (static_cast<MSAPI::Protocol::WebSocket::Events::Type>(*type)) {
	case MSAPI::Protocol::WebSocket::Events::Type::Single:
		m_singlesDistributor.Collect(*uid, *event, connection, std::move(json));
		return;
	case MSAPI::Protocol::WebSocket::Events::Type::Stream:
		m_streamsDistributor.Collect(*uid, *event, connection, std::move(json));
		return;
	default:
		LOG_WARNING_NEW("Unexpected type event is reserved {}",
			MSAPI::Protocol::WebSocket::Events::EnumToString(
				static_cast<MSAPI::Protocol::WebSocket::Events::Type>(*type)));
		return;
	}
}

void Manager::HandleRunRequest()
{
	MSAPI_HANDLE_RUN_REQUEST_PRESET

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	path += "apps.json";

	std::string apps;
	if (!MSAPI::IO::ReadStr(apps, path.c_str())) {
		HandlePauseRequest();
		return;
	}
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

		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_hashToInstalledAppDataLock };
		appId = MSAPI::Helper::StringHashDjb2(*appValue);
		auto it{ m_hashToInstalledAppData.find(appId) };
		if (it == m_hashToInstalledAppData.end()) {
			if (viewValue == nullptr) {
				LOG_INFO("New app registered: " + *appValue + ", id: " + _S(appId) + ", bin: " + *binValue);
				m_hashToInstalledAppData.emplace(
					std::move(appId), std::make_shared<InstalledAppData>(*appValue, *binValue));
				continue;
			}

			LOG_INFO("New app registered: " + *appValue + ", id: " + _S(appId) + ", bin: " + *binValue
				+ ", parameter with port for view: " + _S(static_cast<int32_t>(*viewValue)));
			m_hashToInstalledAppData.emplace(std::move(appId),
				std::make_shared<InstalledAppData>(*appValue, *binValue, static_cast<int32_t>(*viewValue)));
			continue;
		}

		// Is not supported
		if (it->second->bin != *binValue) {
			LOG_INFO("For app: " + *appValue + " bin path changed from " + it->second->bin + " to " + *binValue);
			it->second->bin = *binValue;
			continue;
		}

		if (viewValue != nullptr && it->second->viewPortParameter != static_cast<int32_t>(*viewValue)) {
			LOG_INFO("For app: " + *appValue + " view port changed from " + _S(it->second->viewPortParameter) + " to "
				+ _S(static_cast<int32_t>(*viewValue)));
			it->second->viewPortParameter = static_cast<int32_t>(*viewValue);
		}
	}

	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_hashToInstalledAppDataLock };
		if (m_hashToInstalledAppData.empty()) {
			LOG_ERROR("No apps registered, manager is going to end its work");
			HandlePauseRequest();
			Server::Stop();
			return;
		}
	}

	if (!m_authorizationModule.Start()) {
		HandlePauseRequest();
		return;
	}
}

void Manager::HandlePauseRequest()
{
	MSAPI_HANDLE_PAUSE_REQUEST_PRESET

	m_singlesDistributor.FailActiveEvents("Manager is paused");
	m_streamsDistributor.FailActiveEvents("Manager is paused");

	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_portToCreatedAppLock };
		for (const auto& [port, createdAppData] : m_portToCreatedApp) {
			if (createdAppData->connection == 0) [[unlikely]] {
				LOG_WARNING_NEW("Created app on port {} still did not connect and cannot be deleted", port);
				continue;
			}
			MSAPI::Protocol::Standard::SendActionDelete(createdAppData->connection);
		}
		m_portToCreatedApp.clear();
		m_portGenerator.Clear();
	}

	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_hashToInstalledAppDataLock };
		m_hashToInstalledAppData.clear();
	}

	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_tableIdToColumnsLock };
		m_tableIdToColumns.clear();
	}

	m_authorizationModule.Stop();
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
	const auto portIt{ parameters.find(1000009) };
	if (portIt == parameters.end()) {
		LOG_WARNING("Parameters update without port, connection: " + _S(connection));
		return;
	}

	const uint16_t* port{ std::get_if<uint16_t>(&portIt->second) };
	if (port == nullptr) {
		LOG_ERROR("Port type is unexpected");
		return;
	}

	std::shared_ptr<CreatedAppData> createdAppData;
	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
		auto createdAppDataIt{ m_portToCreatedApp.find(*port) };
		if (createdAppDataIt == m_portToCreatedApp.end()) {
			LOG_ERROR("App with port: " + _S(port) + " is not found");
			return;
		}

		createdAppData = createdAppDataIt->second;
	}

	if (createdAppData->connection == 0) {
		LOG_DEBUG("First parameters update from app: " + createdAppData->appData->type + ", port: " + _S(port));
		createdAppData->connection = connection;

		if (createdAppData->appData->metadata.empty()) {
			MSAPI::Protocol::Standard::SendMetadataRequest(connection);
		}
	}
	else {
		LOG_DEBUG("Parameters update from app: " + createdAppData->appData->type + ", port: " + _S(port));
	}

	// That looks like a shared library function, but uses table columns descriptors and can have locking
	// All chain the chain can be abstracted then, to be populated by library itself if needed
	// -> handle metadata -> parse tables -> handle parameters -> serialize
	const auto serialize{ [&parameters, this](std::string& data) {
		data = '{';
		auto backIt{ std::back_inserter(data) };
		for (const auto& [id, value] : parameters) {
			std::format_to(backIt, "\"{}\":", id);
			std::visit(
				[&backIt, id, this](auto&& arg) {
					using T = std::remove_const_t<std::remove_reference_t<std::decay_t<decltype(arg)>>>;
					if constexpr (MSAPI::is_integer_type<T> || std::is_same_v<T, bool>) {
						std::format_to(backIt, "{}", arg);
					}
					else if constexpr (MSAPI::is_integer_type_optional<T>) {
						if (arg.has_value()) {
							std::format_to(backIt, "{}", arg.value());
						}
						else {
							std::format_to(backIt, "null");
						}
					}
					else if constexpr (MSAPI::is_float_type<T>) {
						if constexpr (std::is_same_v<float, T>) {
							std::format_to(backIt, "{:.9f}", arg);
						}
						else if constexpr (std::is_same_v<double, T>) {
							std::format_to(backIt, "{:.17f}", arg);
						}
						else if constexpr (std::is_same_v<long double, T>) {
							std::format_to(backIt, "{:.21Lf}", arg);
						}
						else {
							static_assert(sizeof(T) + 1 == 0, "Unsupported float type of parameter");
						}
					}
					else if constexpr (MSAPI::is_float_type_optional<T>) {
						if (arg.has_value()) {
							if constexpr (std::is_same_v<float, MSAPI::remove_optional_t<T>>) {
								std::format_to(backIt, "{:.9f}", arg.value());
							}
							else if constexpr (std::is_same_v<double, MSAPI::remove_optional_t<T>>) {
								std::format_to(backIt, "{:.17f}", arg.value());
							}
							else if constexpr (std::is_same_v<long double, MSAPI::remove_optional_t<T>>) {
								std::format_to(backIt, "{:.21Lf}", arg.value());
							}
							else {
								static_assert(sizeof(T) + 1 == 0, "Unsupported float type of parameter");
							}
						}
						else {
							std::format_to(backIt, "null");
						}
					}
					else if constexpr (std::is_same_v<T, std::string>) {
						std::format_to(backIt, "\"{}\"", arg);
					}
					else if constexpr (std::is_same_v<T, MSAPI::Timer> || std::is_same_v<T, MSAPI::Timer::Duration>) {
						std::format_to(backIt, "{}", arg.GetNanoseconds());
					}
					else if constexpr (std::is_same_v<T, MSAPI::TableData>) {
						std::shared_ptr<std::vector<MSAPI::StandardType::Type>> tableColumns;
						{
							MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_tableIdToColumnsLock };
							const auto it{ m_tableIdToColumns.find(id) };
							if (it == m_tableIdToColumns.end()) [[unlikely]] {
								LOG_DEBUG("Columns for table with id: " + _S(id) + " are not found");
								std::format_to(backIt, "\"\"");
								return;
							}

							tableColumns = it->second;
						}

						const std::string json{ arg.LookUpToJson(*tableColumns) };
						if (json.empty()) [[unlikely]] {
							std::format_to(backIt, "\"\"");
							return;
						}

						std::format_to(backIt, "{}", json);
					}
					else {
						static_assert(sizeof(T) + 1 == 0, "Unsupported type of parameter");
					}
				},
				value);
			std::format_to(backIt, ",");
		}
		data.pop_back();
		data += '}';

		return true;
	} };

	(void)m_streamsDistributor.SendData(
		{ MSAPI::Helper::StringHashDjb2("parameters"),
			MSAPI::Protocol::WebSocket::Events::IdentityFilter{ static_cast<uint64_t>(*port) } },
		serialize);
}

void Manager::HandleHello(const int connection) { MSAPI::Protocol::Standard::SendParametersRequest(connection); }

void Manager::HandleMetadata(const int connection, const std::string_view metadata)
{
	std::shared_ptr<CreatedAppData> createdAppData;
	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
		if (std::ranges::none_of(m_portToCreatedApp, [connection, &createdAppData](const auto& data) {
				if (data.second->connection == connection) {
					createdAppData = data.second;
					return true;
				}
				return false;
			})) {

			LOG_ERROR("Metadata update from unknown app, connection: " + _S(connection));
			return;
		}
	}

	LOG_DEBUG("Metadata update for app: " + createdAppData->appData->type);
	if (!createdAppData->appData->metadata.empty()) {
		LOG_WARNING_NEW("Metadata for app: {} is already handled", createdAppData->hash);
		return;
	}

	createdAppData->appData->metadata = metadata;
	createdAppData->appData->metadataJson.Construct(metadata);
	if (!createdAppData->appData->metadataJson.Valid()) {
		LOG_ERROR("Metadata for app: " + createdAppData->appData->type + " is not valid");
		return;
	}

	const auto parseTables{ [this, connection](const MSAPI::Json* const parameters) {
		for (const auto& [keyStr, node] : parameters->GetKeysAndValues()) {
			const auto* nodeValue{ std::get_if<MSAPI::Json>(&node.GetValue()) };
			if (nodeValue == nullptr) {
				continue;
			}

			const auto* type{ nodeValue->GetValueType<std::string>("type") };
			if (type == nullptr || *type != "TableData") {
				continue;
			}

			const auto* columns{ nodeValue->GetValueType<MSAPI::Json>("columns") };
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

			{
				MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_tableIdToColumnsLock };
				if (m_tableIdToColumns.find(tableId) != m_tableIdToColumns.end()) {
					continue;
				}
			}

			auto columnTypes{ std::make_shared<std::vector<MSAPI::StandardType::Type>>() };
			columnTypes->reserve(columns->GetKeysAndValues().size());
			for (const auto& [columnId, columnMetadataNode] : columns->GetKeysAndValues()) {
				const auto* columnMetadata{ std::get_if<MSAPI::Json>(&columnMetadataNode.GetValue()) };
				if (columnMetadata == nullptr) {
					LOG_ERROR("Broken metadata, impossible to find metadata of column id " + columnId
						+ " in table id: " + _S(tableId) + ", connection: " + _S(connection)
						+ ", columns metadata: " + columns->ToString());
					continue;
				}

				const auto* type{ columnMetadata->GetValueType<std::string>("type") };
				if (type == nullptr) {
					LOG_ERROR("Broken metadata, absent or wrong type of column id " + columnId
						+ " in table id: " + _S(tableId) + ", connection: " + _S(connection)
						+ ", column metadata: " + columnMetadata->ToString());
					continue;
				}

				if (*type == "Int8") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Int8);
				}
				else if (*type == "Int16") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Int16);
				}
				else if (*type == "Int32") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Int32);
				}
				else if (*type == "Int64") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Int64);
				}
				else if (*type == "Float") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Float);
				}
				else if (*type == "Timer") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Timer);
				}
				else if (*type == "Double") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Double);
				}
				else if (*type == "String") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::String);
				}
				else if (*type == "Duration") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Duration);
				}
				else if (*type == "Uint8") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Uint8);
				}
				else if (*type == "OptionalInt8") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalInt8);
				}
				else if (*type == "Uint16") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Uint16);
				}
				else if (*type == "OptionalInt16") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalInt16);
				}
				else if (*type == "Uint32") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Uint32);
				}
				else if (*type == "Uint64") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Uint64);
				}
				else if (*type == "OptionalInt32") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalInt32);
				}
				else if (*type == "OptionalFloat") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalFloat);
				}
				else if (*type == "OptionalDouble") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalDouble);
				}
				else if (*type == "OptionalInt64") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalInt64);
				}
				else if (*type == "OptionalUint8") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalUint8);
				}
				else if (*type == "OptionalUint16") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalUint16);
				}
				else if (*type == "OptionalUint32") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalUint32);
				}
				else if (*type == "OptionalUint64") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::OptionalUint64);
				}
				else if (*type == "Bool") {
					columnTypes->emplace_back(MSAPI::StandardType::Type::Bool);
				}
				else {
					LOG_ERROR("Broken metadata, unknown or unsupported type of column \"" + *type + "\" id " + columnId
						+ " in table id: " + _S(tableId) + ", connection: " + _S(connection)
						+ ", column metadata: " + columnMetadata->ToString());
				}
			}

			if (columnTypes->empty()) [[unlikely]] {
				LOG_ERROR("Broken metadata, impossible to find any column type in table id: " + _S(tableId)
					+ ", connection: " + _S(connection) + ", JSON: " + node.ToString());
				continue;
			}

			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_tableIdToColumnsLock };
			m_tableIdToColumns.emplace(tableId, std::move(columnTypes));
			LOG_DEBUG("Columns for table with id: " + _S(tableId) + " are found, connection: " + _S(connection));
		}
	} };

	if (const auto* mutableParameters{ createdAppData->appData->metadataJson.GetValueType<MSAPI::Json>("mutable") };
		mutableParameters != nullptr) {

		parseTables(mutableParameters);
	}

	if (const auto* constParameters{ createdAppData->appData->metadataJson.GetValueType<MSAPI::Json>("const") };
		constParameters != nullptr) {

		parseTables(constParameters);
	}

	m_singlesDistributor.CheckDelayed({ MSAPI::Helper::StringHashDjb2("getMetadata"),
										  MSAPI::Protocol::WebSocket::Events::IdentityFilter{ createdAppData->hash } },
		metadata);
}

void Manager::HandleOutcomeDisconnect([[maybe_unused]] const int32_t id, const int32_t connection)
{
	m_authorizationModule.LogoutConnection(connection);
	m_singlesDistributor.ClearActiveEventsForConnection(connection);
	m_streamsDistributor.ClearActiveEventsForConnection(connection);
}

void Manager::HandleIncomeDisconnect([[maybe_unused]] const int32_t id, const int32_t connection)
{
	m_authorizationModule.LogoutConnection(connection);
	m_singlesDistributor.ClearActiveEventsForConnection(connection);
	m_streamsDistributor.ClearActiveEventsForConnection(connection);
}

uint16_t Manager::CreateApp(const uint64_t hash, const MSAPI::Json& parameters, std::string& error)
{
	std::shared_ptr<InstalledAppData> installedAppData;
	{
		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_hashToInstalledAppDataLock };
		auto it{ m_hashToInstalledAppData.find(hash) };
		if (it == m_hashToInstalledAppData.end()) {
			error = std::format("Unknow app type hash {}", hash);
			return 0;
		}

		installedAppData = it->second;
	}

	LOG_DEBUG("Creating app: " + installedAppData->type + ", id: " + _S(hash) + " from " + installedAppData->bin);

	auto ip{ htobe32(INADDR_LOOPBACK) };
	const auto* parametersIp{ parameters.GetValue("ip") };
	if (parametersIp != nullptr) {
		auto* parametersIpValue{ std::get_if<std::string>(&parametersIp->GetValue()) };
		if (parametersIpValue != nullptr && !parametersIpValue->empty()) {
			if (!MSAPI::Helper::ValidateIpv4(parametersIpValue->c_str())) {
				error = std::format("Invalid ip address: {}", *parametersIpValue);
				return 0;
			}

			ip = inet_addr(parametersIpValue->c_str());
			if (ip == INADDR_NONE) {
				error = std::format("Invalid IP address: {}", *parametersIpValue);
				return 0;
			}
			ip = htobe32(ip);
		}
	}

	uint16_t port{ 0 };
	const auto* parametersPort{ parameters.GetValue("port") };
	if (parametersPort != nullptr) {
		const auto* parametersPortValue{ std::get_if<std::string>(&parametersPort->GetValue()) };
		if (parametersPortValue != nullptr && *parametersPortValue != "0" && !parametersPortValue->empty()) {
			const auto result{ std::from_chars(
				parametersPortValue->data(), parametersPortValue->data() + parametersPortValue->size(), port)
								   .ec };
			if (result != std::errc{}) {
				error = "Broken port in app parameters: " + *parametersPortValue
					+ ". Error: " + std::make_error_code(result).message();
				return 0;
			}
		}
	}

	if (port == 0) {
		port = m_portGenerator.Get();
		if (port == 0) [[unlikely]] {
			error = "Cannot generate an unique port for app: " + installedAppData->type + ", id: " + _S(hash);
			LOG_ERROR(error);
			return 0;
		}
	}

	int16_t logLevel{ static_cast<int16_t>(MSAPI::Log::Level::WARNING) };
	const auto* parametersLogLevel{ parameters.GetValue("logLevel") };
	if (parametersLogLevel != nullptr) {
		const auto* parametersLogLevelValue{ std::get_if<std::string>(&parametersLogLevel->GetValue()) };
		if (parametersLogLevelValue != nullptr && !parametersLogLevelValue->empty()) {
			const auto result{ std::from_chars(parametersLogLevelValue->data(),
				parametersLogLevelValue->data() + parametersLogLevelValue->size(), logLevel)
								   .ec };
			if (result != std::errc{}) {
				error = "Broken log level in app parameters: " + *parametersLogLevelValue
					+ ". Error: " + std::make_error_code(result).message();
				m_portGenerator.Erase(port);
				return 0;
			}

			if (logLevel < static_cast<int16_t>(MSAPI::Log::Level::ERROR)
				|| logLevel > static_cast<int16_t>(MSAPI::Log::Level::PROTOCOL)) {

				error = "Invalid log level in app parameters: " + *parametersLogLevelValue;
				m_portGenerator.Erase(port);
				return 0;
			}
		}
	}

	bool logInConsole{ false };
	if (const auto logInConsoleStr{ parameters.GetValueType<std::string>("logInConsole") };
		logInConsoleStr != nullptr) {
		logInConsole = *logInConsoleStr == "true";
	}

	bool logInFile{ false };
	if (const auto logInFileStr{ parameters.GetValueType<std::string>("logInFile") }; logInFileStr != nullptr) {
		logInFile = *logInFileStr == "true";
	}

	bool separateDaysLogging{ true };
	if (const auto* separateDaysLoggingStr{ parameters.GetValueType<std::string>("separateDaysLogging") };
		separateDaysLoggingStr != nullptr) {
		separateDaysLogging = *separateDaysLoggingStr == "false";
	}

	std::string name;
	if (const auto* nameStr{ parameters.GetValueType<std::string>("name") }; nameStr != nullptr) {
		name = *nameStr;
	}
	else {
		name = installedAppData->type;
	}

	auto normalizedParameters{ std::format(
		"{{\"name\":\"{}\",\"ip\":\"{}\",\"port\":\"{}\",\"managerPort\":\"{}\",\"logLevel\":\"{}\",\"logInConsole\":"
		"\"{}\",\"logInFile\":\"{}\",\"separateDaysLogging\":\"{}\"}}",
		name, ip, port, GetListenedPort(), logLevel, logInConsole, logInFile, separateDaysLogging) };

	LOG_DEBUG("Parameters: " + normalizedParameters);

	const pid_t pid{ vfork() };
	if (pid < 0) {
		error = "Can't vfork for app: " + installedAppData->type + ", id: " + _S(hash) + " from "
			+ installedAppData->bin + ". Error №" + _S(errno) + ": " + std::strerror(errno);
		LOG_ERROR(error);
		m_portGenerator.Erase(port);
		return 0;
	}

	//* Parent process
	if (pid > 0) {
		LOG_INFO("App: " + installedAppData->type + ", id: " + _S(hash) + " created with pid: " + _S(pid));
		auto createdAppData{ std::make_shared<CreatedAppData>(hash, pid, installedAppData) };
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_portToCreatedAppLock };
			m_portToCreatedApp.emplace(port, createdAppData);
		}
		const auto serialize{ [&installedAppData, &createdAppData, port, pid](std::string& data) {
			data = std::format("{{\"created\":[{{\"type\":\"{}\",\"port\":{},\"pid\":{},\"creation time\":\"{}\"}}]}}",
				installedAppData->type, port, pid, createdAppData->created.ToString());
			return true;
		} };

		(void)m_streamsDistributor.SendData(
			{ MSAPI::Helper::StringHashDjb2("createdApps"), MSAPI::Protocol::WebSocket::Events::IdentityFilter{} },
			serialize);
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
				std::string{ installedAppData->bin + " '" + normalizedParameters + "'" }.c_str(), nullptr)
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
	pid_t pid;
	int status;
	uint16_t port;
	std::shared_ptr<CreatedAppData> createdAppData;

	while (true) {
		pid = waitpid(-1, nullptr, WNOHANG);
		if (pid == 0) {
			return;
		}

		if (pid == -1) {
			if (errno == ECHILD) {
				return;
			}

			if (errno == EINTR) {
				LOG_DEBUG("Waitpid error №" + _S(errno) + ": " + std::strerror(errno));
				continue;
			}

			LOG_WARNING("Waitpid error №" + _S(errno) + ": " + std::strerror(errno));
			return;
		}

		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
			if (std::ranges::none_of(m_portToCreatedApp, [pid, &port, &createdAppData](const auto& data) {
					if (data.second->pid == pid) {
						port = data.first;
						createdAppData = data.second;
						return true;
					}

					return false;
				})) {

				LOG_WARNING_NEW("Termination of unrecognized pthread is reserved, pid {}", pid);
				continue;
			}
		}

		if (WIFEXITED(status)) {
			LOG_INFO("App: " + createdAppData->appData->type + ", port: " + _S(port)
				+ " with pid: " + _S(createdAppData->pid) + " is terminated with status: " + _S(WEXITSTATUS(status)));
		}
		else {
			LOG_INFO("App: " + createdAppData->appData->type + ", port: " + _S(port)
				+ " with pid: " + _S(createdAppData->pid) + " is terminated");
		}

		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::write> _{ m_portToCreatedAppLock };
			m_portToCreatedApp.erase(port);
			m_portGenerator.Erase(port);
		}

		const auto serialize{ [port](std::string& data) {
			data = std::format("{{\"deleted\":[{}]}}", port);
			return true;
		} };
		(void)m_streamsDistributor.SendData(
			{ MSAPI::Helper::StringHashDjb2("createdApps"), MSAPI::Protocol::WebSocket::Events::IdentityFilter{} },
			serialize);
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

Manager::CreatedAppData::CreatedAppData(
	const size_t hash, const int pid, const std::shared_ptr<InstalledAppData> appData)
	: hash{ hash }
	, pid{ pid }
	, appData{ appData }
{
}