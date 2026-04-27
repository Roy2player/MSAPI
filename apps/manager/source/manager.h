/**************************
 * @file        manager.h
 * @version     6.0
 * @date        2024-02-19
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

#ifndef MSAPI_APP_MANAGER_H
#define MSAPI_APP_MANAGER_H

#include "../../../library/source/help/json.h"
#include "../../../library/source/help/pthread.hpp"
#include "../../../library/source/protocol/http.h"
#include "../../../library/source/protocol/object.h"
#include "../../../library/source/protocol/webSocket.inl"
#include "../../../library/source/protocol/webSocketEvents.inl"
#include "../../../library/source/server/authorization.inl"
#include "../../../library/source/server/server.h"

/**************************
 * @brief Manager app provides functionality to create, modify, delete, run, pause and monitor any another app based on
 * MSAPI library. Manager app can be used via web interface. Manager registers apps on its startup from source file
 * "apps.json" which is must be located in same directory as bin file.
 *
 * Format of apps.json file is:
 * {
 *		"Apps": [
 * 			{
 *				"Type": "Custom1",
 *				"Bin": "/path/to/custom1App/bin"
 *			},
 *			{
 *				"Type": "Custom2",
 *				"Bin": "/path/to/custom2App/bin",
 *				"View" : 30001
 *			}
 *		]
 * }
 *
 * In that case Manager will know about Custom1 and Custom2 apps and will be able to manage them.
 * Type is a string which identifies app type.
 * Bin is a path to app bin file, it must be executable.
 * View is an optional parameter, it is a parameter where port where app view can be accessed is stored.
 *
 * @brief Parameter 1001 "Web sources path" is a path to directory contains web files.
 *
 * @brief The idea of metadata in MSAPI Manager frontend can be described as "FE must know about metadata item only when
 * it can be needed". That is why an app metadata is asked only when app instance is available in the system and
 * populated by app itself. FE can use metadata items which are not a part of any app, they can be registered there
 * directly. There is no metadata parameters synchronization, if some of parameters from different apps have same id and
 * they both are used in FE, one of them overwrites another one. In case, when parameter contains scalar value which
 * represents a string, like hash of instrument name for example, then custom string interpretations are used.
 *
 * @brief Manager can interract with FE via HTTP or WebSocket protocols. In case of web socket, manager controls rights
 * to data access and uses events based abstraction with ability to respond on single events or provide data in a stream
 * manner by filter for subscribers.
 */
class Manager : public MSAPI::Server, MSAPI::Protocol::HTTP::IHandler, MSAPI::Protocol::WebSocket::IHandler {
private:
	/**
	 * @brief Synched generator of unique ports.
	 */
	class PortGenerator {
	private:
		std::set<uint16_t> m_ports;
		MSAPI::Pthread::AtomicLock m_portsLock;
		std::mt19937 m_mersenne;
		const int32_t m_limit{ 50000 };
		int32_t m_counter{};

	public:
		/**
		 * @brief Get the Port object
		 *
		 * @return FORCE_INLINE
		 */
		FORCE_INLINE [[nodiscard]] uint16_t Get()
		{
			m_mersenne.seed(static_cast<uint64_t>(MSAPI::Timer{}.GetNanoseconds()));
			uint16_t port;
			m_counter = 0;

			MSAPI::Pthread::AtomicLock::ExitGuard _{ m_portsLock };
			do {
				port = static_cast<uint16_t>(m_mersenne() % (65535 - 3000) + 3000);
				if (m_ports.find(port) == m_ports.end()) {
					m_ports.emplace(port);
					return port;
				}
			} while (++m_counter < m_limit);

			return 0;
		}

		/**
		 * @brief Erase port from range of used.
		 *
		 * @param port Port to be erased.
		 *
		 * @test Add unit test.
		 */
		FORCE_INLINE void Erase(const uint16_t port) noexcept
		{
			MSAPI::Pthread::AtomicLock::ExitGuard _{ m_portsLock };
			m_ports.erase(port);
		}

		/**
		 * @brief Clear range of used ports.
		 *
		 * @test Add unit test.
		 */
		FORCE_INLINE void Clear() noexcept
		{
			MSAPI::Pthread::AtomicLock::ExitGuard _{ m_portsLock };
			m_ports.clear();
		}
	};

	/**************************
	 * @brief Contains information about installed app.
	 */
	struct InstalledAppData {
		bool hasView;
		int32_t viewPortParameter;
		std::string type;
		std::string bin;
		std::string metadata;
		MSAPI::Json metadataJson;

		/**************************
		 * @brief Construct a new Installed App Data object without view.
		 *
		 * @param type App type.
		 * @param bin Path to app bin.
		 */
		InstalledAppData(const std::string& type, const std::string& bin);

		/**************************
		 * @brief Construct a new Installed App Data object with view.
		 *
		 * @param type App type.
		 * @param bin Path to app bin.
		 * @param viewPortParameter Parameter where port for view is stored.
		 */
		InstalledAppData(const std::string& type, const std::string& bin, const int32_t viewPortParameter)
			: hasView{ true }
			, viewPortParameter{ viewPortParameter }
			, type{ type }
			, bin{ bin }
		{
		}
	};

	/**************************
	 * @brief Contains information about created app.
	 */
	struct CreatedAppData {
		const size_t hash;
		const int pid;
		const MSAPI::Timer created;
		const std::shared_ptr<InstalledAppData> appData;
		int connection{ 0 };

		/**************************
		 * @brief Construct a new Created App Data object.
		 *
		 * @param hash Hash of app type.
		 * @param pid Process id.
		 * @param appData Pointer to installed app data.
		 */
		CreatedAppData(size_t hash, int pid, std::shared_ptr<InstalledAppData> appData);
	};

private:
	std::string m_webSourcesPath;
	std::map<size_t, std::shared_ptr<InstalledAppData>> m_hashToInstalledAppData;
	MSAPI::Pthread::AtomicRWLock m_hashToInstalledAppDataLock;
	std::map<uint16_t, std::shared_ptr<CreatedAppData>> m_portToCreatedApp;
	MSAPI::Pthread::AtomicRWLock m_portToCreatedAppLock;
	std::map<size_t, std::shared_ptr<std::vector<MSAPI::StandardType::Type>>> m_tableIdToColumns;
	MSAPI::Pthread::AtomicRWLock m_tableIdToColumnsLock;
	MSAPI::Authorization::Base::Module<> m_authorizationModule;
	MSAPI::Protocol::WebSocket::Events::SinglesDistributor<MSAPI::Authorization::Base::Module<>> m_singlesDistributor{
		m_authorizationModule
	};
	MSAPI::Protocol::WebSocket::Events::StreamsDistributor<MSAPI::Authorization::Base::Module<>> m_streamsDistributor{
		m_authorizationModule
	};
	PortGenerator m_portGenerator;

public:
	/**************************
	 * @brief Construct a new Manager object, check access to /bin/bash, register parameters.
	 */
	Manager();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::Application
	void HandleRunRequest() final;
	void HandlePauseRequest() final;
	void HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate) final;
	void HandleParameters(int connection, const std::map<size_t, std::variant<standardTypes>>& parameters) final;
	void HandleHello(int connection) final;
	void HandleMetadata(int connection, std::string_view metadata) final;
	void HandleOutcomeDisconnect(int id, int32_t connection) final;
	void HandleIncomeDisconnect(int id, int32_t connection) final;
	//* MSAPI::Protocol::HTTP::IHandler
	void HandleHttp(int connection, const MSAPI::Protocol::HTTP::Data& data) final;
	//* MSAPI::Protocol::WebSocket::IHandler
	void HandleWebSocket(int connection, MSAPI::Protocol::WebSocket::Data&& data) final;

	/**************************
	 * @brief Read the execution status of vforked apps to prevent zombie processes and answer related requests in
	 * pending state. Should be setted as handler for SIGCHLD.
	 */
	void CheckVforkedApps();

private:
	FORCE_INLINE [[nodiscard]] MSAPI::Protocol::WebSocket::Events::HandleResult FillInstalledApps(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		auto backIt{ std::back_inserter(out) };
		std::format_to(backIt, "[");
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_hashToInstalledAppDataLock };
			if (!m_hashToInstalledAppData.empty()) {
				auto it{ m_hashToInstalledAppData.begin() };
				const auto end{ m_hashToInstalledAppData.end() };

				std::format_to(backIt, "{{\"type\":\"{}\"", it->second->type);
				if (it->second->hasView) {
					std::format_to(backIt, ",\"viewPortParameter\":\"{}\"", it->second->viewPortParameter);
				}
				std::format_to(backIt, "}}");

				for (++it; it != end; ++it) {
					std::format_to(backIt, ",{{\"type\":\"{}\"", it->second->type);
					if (it->second->hasView) {
						std::format_to(backIt, ",\"viewPortParameter\":\"{}\"", it->second->viewPortParameter);
					}
					std::format_to(backIt, "}}");
				}
			}
		}
		std::format_to(backIt, "]");

		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE [[nodiscard]] MSAPI::Protocol::WebSocket::Events::HandleResult FillMetadata(
		std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto* appType{ single.GetJson().GetValueType<uint64_t>("appType") };
		if (appType == nullptr) {
			out = "Metadata request contains incorrect appType field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_hashToInstalledAppDataLock };
		const auto it{ m_hashToInstalledAppData.find(*appType) };
		if (it == m_hashToInstalledAppData.end()) {
			out = std::format("Metadata request contains unknown appType: {}", *appType);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		if (it->second->metadata.empty()) {
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Delay;
		}

		std::format_to(std::back_inserter(out), "{}", it->second->metadata);
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult Register(
		std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto& json{ single.GetJson() };

		const auto* login{ json.GetValueType<std::string>("login") };
		if (login == nullptr) {
			out = "Registration request contains incorrect login field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		const auto* password{ json.GetValueType<std::string>("password") };
		if (password == nullptr) {
			out = "Registration request contains incorrect password field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		std::string error;
		if (!m_authorizationModule.RegisterAccount(*login, *password, error)) {
			out = error;
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		(void)m_authorizationModule.ModifyAccountGrade(*login, MSAPI::Authorization::Base::Grade::User);
		(void)m_authorizationModule.SetAccountActivatedState(*login, true);

		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult Login(
		std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto& json{ single.GetJson() };

		const auto* login{ json.GetValueType<std::string>("login") };
		if (login == nullptr) {
			out = "Login request contains incorrect login field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		const auto* password{ json.GetValueType<std::string>("password") };
		if (password == nullptr) {
			out = "Login request contains incorrect password field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		std::string error;
		if (!m_authorizationModule.LogonConnection(single.GetConnection(), *login, *password, error)) {
			out = error;
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult Logout(
		std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		m_authorizationModule.LogoutConnection(single.GetConnection());
		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult ModifyAccount(
		std::string& out, const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto& json{ single.GetJson() };

		const auto* login{ json.GetValueType<std::string>("login") };
		if (login == nullptr) {
			out = "Modify account request contains incorrect login field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		out += "{";

		bool passwordModifySuccess{ true };
		std::string passwordModifyError;
		const auto* newPassword{ json.GetValueType<std::string>("newPassword") };
		if (newPassword != nullptr) {
			do {
				if (m_authorizationModule.ModifyAccountPassword(*login, *newPassword, passwordModifyError)) {
					std::format_to(std::back_inserter(out), "\"password\":true");
					break;
				}

				passwordModifySuccess = false;
				std::format_to(std::back_inserter(out), "\"password\":false");
			} while (false);
		}

		bool loginModifySuccess{ true };
		std::string loginModifyError;
		const auto* newLogin{ json.GetValueType<std::string>("newLogin") };
		if (newLogin != nullptr) {
			do {
				if (newPassword != nullptr) {
					out += ",";
				}

				if (m_authorizationModule.ModifyAccountLogin(*login, *newLogin, loginModifyError)) {
					std::format_to(std::back_inserter(out), "\"login\":true");
					break;
				}

				loginModifySuccess = false;
				std::format_to(std::back_inserter(out), "\"login\":false");
			} while (false);
		}

		if (passwordModifySuccess && loginModifySuccess) {
			out += "}";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
		}

		if (passwordModifyError.empty()) {
			passwordModifyError = loginModifyError;
		}
		else if (!loginModifyError.empty()) {
			std::format_to(std::back_inserter(passwordModifyError), ", {}", loginModifyError);
		}

		if (passwordModifySuccess || loginModifySuccess) {
			std::format_to(std::back_inserter(out), ",\"error\":\"{}\"}}", passwordModifyError);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
		}

		out = passwordModifyError;
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult CreateApp(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto& json{ single.GetJson() };

		const auto* appType{ json.GetValueType<uint64_t>("appType") };
		if (appType == nullptr) {
			out = "Create app request contains incorrect appType field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		const auto* parameters{ json.GetValueType<MSAPI::Json>("parameters") };
		if (appType == nullptr) {
			out = "Create app request contains incorrect parameters field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		const auto port{ CreateApp(*appType, *parameters, out) };
		if (port == 0) {
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		std::format_to(std::back_inserter(out), "{{\"port\":{}}}", port);
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult PauseApp(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto* port{ single.GetJson().GetValueType<uint64_t>("port") };
		if (port == nullptr) {
			out = "Pause app request contains incorrect port field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		int32_t connection;
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
			const auto it{ m_portToCreatedApp.find(static_cast<uint16_t>(*port)) };
			if (it == m_portToCreatedApp.end()) {
				out = std::format("App on port {} is not found", *port);
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}
			connection = it->second->connection;
		}

		if (connection == 0) {
			out = std::format("App on port {} is not connected yet", *port);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		MSAPI::Protocol::Standard::SendActionPause(connection);
		MSAPI::Protocol::Standard::SendParametersRequest(connection);
		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult RunApp(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto* port{ single.GetJson().GetValueType<uint64_t>("port") };
		if (port == nullptr) {
			out = "Run app request contains incorrect port field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		int32_t connection;
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
			const auto it{ m_portToCreatedApp.find(static_cast<uint16_t>(*port)) };
			if (it == m_portToCreatedApp.end()) {
				out = std::format("App on port {} is not found", *port);
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}
			connection = it->second->connection;
		}

		if (connection == 0) {
			out = std::format("App on port {} is not connected yet", *port);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		MSAPI::Protocol::Standard::SendActionRun(connection);
		MSAPI::Protocol::Standard::SendParametersRequest(connection);
		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult DeleteApp(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto* port{ single.GetJson().GetValueType<uint64_t>("port") };
		if (port == nullptr) {
			out = "Delete app request contains incorrect port field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		int32_t connection;
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
			const auto it{ m_portToCreatedApp.find(static_cast<uint16_t>(*port)) };
			if (it == m_portToCreatedApp.end()) {
				out = std::format("App on port {} is not found", *port);
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}
			connection = it->second->connection;
		}

		if (connection == 0) {
			out = std::format("App on port {} is not connected yet", *port);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		MSAPI::Protocol::Standard::SendActionDelete(connection);
		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult ModifyApp(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Single& single)
	{
		const auto& json{ single.GetJson() };

		const auto* port{ json.GetValueType<uint64_t>("port") };
		if (port == nullptr) {
			out = "Modify app request contains incorrect port field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		const auto* parameters{ json.GetValueType<MSAPI::Json>("parameters") };
		if (parameters == nullptr) {
			out = "Modify app request does not incorrect parameters field";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		std::shared_ptr<CreatedAppData> createdAppData;
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
			const auto it{ m_portToCreatedApp.find(static_cast<uint16_t>(*port)) };
			if (it == m_portToCreatedApp.end()) {
				out = std::format("App on port {} is not found", *port);
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}

			createdAppData = it->second;
		}

		if (createdAppData->connection == 0) {
			out = std::format("App on port {} is not connected yet", *port);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		if (!createdAppData->appData->metadataJson.Valid()) {
			out = std::format("Metadata for app {} type is not valid", createdAppData->hash);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		const auto* mutableParameters{ createdAppData->appData->metadataJson.GetValueType<MSAPI::Json>("mutable") };
		if (mutableParameters == nullptr) {
			out = std::format("Metadata for app {} type does not contain mutable parameters", createdAppData->hash);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		if (mutableParameters->GetKeysAndValues().empty()) {
			out = std::format("Metadata for app {} type contains empty mutable parameters", createdAppData->hash);
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		MSAPI::Protocol::Standard::Data parametersUpdate{ MSAPI::Protocol::Standard::cipherActionModify };
		size_t key;
		for (const auto& [keyStr, node] : parameters->GetKeysAndValues()) {
			const auto error{ std::from_chars(keyStr.data(), keyStr.data() + keyStr.size(), key).ec };
			if (error != std::errc{}) {
				out = std::format("Key {} in parameters cannot be converted into id", keyStr);
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}

			const auto* metadataItem{ mutableParameters->GetValueType<MSAPI::Json>(keyStr) };
			if (metadataItem == nullptr) {
				LOG_DEBUG_NEW("Metadata item for {} is not found or it is const, app with port: {}", keyStr, *port);
				continue;
			}

			const auto* parameterType{ metadataItem->GetValueType<std::string>("type") };
			if (parameterType == nullptr) {
				LOG_ERROR_NEW("Metadata item for {} has incorrect type, app with port: {}", keyStr, *port);
				continue;
			}

			const auto* value{ &node.GetValue() };

#define TMP_MANAGER_TRY_SET_DATA_PARAMETER(type, jsonType)                                                             \
	if (const auto* param{ std::get_if<jsonType>(value) }; param != nullptr) {                                         \
		parametersUpdate.SetData(key, static_cast<type>(*param));                                                      \
		continue;                                                                                                      \
	}

#define TMP_MANAGER_CONTINUE_WITH_ERROR                                                                                \
	LOG_ERROR_NEW("Update for parameter {} is not a valid type, parameter type: {}, app with port: {}", keyStr,        \
		*parameterType, *port);                                                                                        \
	continue;

			if (*parameterType == "Int8") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int8_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int8_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Int16") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int16_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int16_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Bool") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(bool, bool);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Int32") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int32_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int32_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Float") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(float, double);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(float, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(float, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Timer") {
				if (const auto* param{ std::get_if<uint64_t>(value) }; param != nullptr) {
					parametersUpdate.SetData(
						key, MSAPI::Timer{ INT64(*param) / 1000000000, INT64(*param) % 1000000000 });
					continue;
				}
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Double") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(double, double);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(double, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(double, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "String") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(std::string, std::string);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Duration") {
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
			if (*parameterType == "TableData") {
				if (const auto* param{ std::get_if<std::list<MSAPI::JsonNode>>(value) }; param != nullptr) {
					if (param->empty()) {
						parametersUpdate.SetData(key, MSAPI::TableData{});
						continue;
					}

					std::shared_ptr<std::vector<MSAPI::StandardType::Type>> columns;
					{
						MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_tableIdToColumnsLock };
						const auto it{ m_tableIdToColumns.find(key) };
						if (it == m_tableIdToColumns.end()) [[unlikely]] {
							LOG_ERROR_NEW("Columns for table with id: {} are not found, app with port: {}", key, *port);
							continue;
						}

						columns = it->second;
					}

					const MSAPI::TableData table{ *param, *columns };
					if (table.GetBufferSize() == sizeof(size_t) /* something went wrong */) {
						continue;
					}

					parametersUpdate.SetData(key, std::move(table));
					continue;
				}
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Int64") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int64_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(int64_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalInt8") {

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
			if (*parameterType == "Uint8") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint8_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalInt16") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int16_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int16_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int16_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Uint16") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint16_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Uint32") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint32_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalInt32") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int32_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int32_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int32_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalFloat") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(float);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(float, double);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(float, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(float, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalDouble") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(double);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(double, double);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(double, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(double, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "Uint64") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER(uint64_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalInt64") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(int64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int64_t, uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(int64_t, int64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalUint8") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint8_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint8_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalUint16") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint16_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint16_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalUint32") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint32_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint32_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			if (*parameterType == "OptionalUint64") {
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL(uint64_t);
				TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL(uint64_t, uint64_t);
				TMP_MANAGER_CONTINUE_WITH_ERROR;
			}
			LOG_ERROR_NEW(
				"Broken metadata, unknown or unsupported type of parameter \"{}\" in metadata for app with port: {}",
				*parameterType, *port);
		}

#undef TMP_MANAGER_TRY_SET_DATA_PARAMETER_OPTIONAL
#undef TMP_MANAGER_TRY_SET_DATA_PARAMETER_EMPTY_OPTIONAL
#undef TMP_MANAGER_CONTINUE_WITH_ERROR
#undef TMP_MANAGER_TRY_SET_DATA_PARAMETER

		if (parametersUpdate.GetBufferSize() > sizeof(size_t) * 2) {
			MSAPI::Protocol::Standard::Send(createdAppData->connection, parametersUpdate);
			MSAPI::Protocol::Standard::SendParametersRequest(createdAppData->connection);
		}

		out += "\"\"";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult FillCreatedApps(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Stream& stream)
	{
		auto backIt{ std::back_inserter(out) };
		std::format_to(backIt, "{{\"created\":[");
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_tableIdToColumnsLock };
			if (!m_portToCreatedApp.empty()) {
				auto it{ m_portToCreatedApp.begin() };
				const auto end{ m_portToCreatedApp.end() };

				std::format_to(backIt, "{{\"type\":\"{}\",\"port\":{},\"pid\":{},\"creation time\":\"{}\"}}",
					it->second->appData->type, it->first, it->second->pid, it->second->created.ToString());
				for (++it; it != end; ++it) {
					std::format_to(backIt, ",{{\"type\":\"{}\",\"port\":{},\"pid\":{},\"creation time\":\"{}\"}}",
						it->second->appData->type, it->first, it->second->pid, it->second->created.ToString());
				}
			}
		}
		std::format_to(backIt, "]}}");
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	FORCE_INLINE MSAPI::Protocol::WebSocket::Events::HandleResult FillAppParameters(
		std::string& out, [[maybe_unused]] const MSAPI::Protocol::WebSocket::Events::Stream& stream)
	{
		const auto& json{ stream.GetJson() };

		const auto* port{ json.GetValueType<uint64_t>("port") };
		if (port == nullptr) {
			out = "To subscribe for app parameters the port must be provided";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
		}

		int32_t connection;
		{
			MSAPI::Pthread::AtomicRWLock::ExitGuard<MSAPI::Pthread::read> _{ m_portToCreatedAppLock };
			const auto createdAppDataIt{ m_portToCreatedApp.find(static_cast<uint16_t>(*port)) };
			if (createdAppDataIt == m_portToCreatedApp.end()) {
				out = std::format("App on port {} does not exist", *port);
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}

			connection = createdAppDataIt->second->connection;
		}

		if (connection == 0) {
			out += "{}";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
		}

		MSAPI::Protocol::Standard::SendParametersRequest(connection);
		out += "{}";
		return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
	}

	/**************************
	 * @brief Creates an app from installed app data and parameters Json. Provides error message if failed.
	 * @brief Parameter ip is application listening ip. INADDR_LOOPBACK by default.
	 * @brief Parameter port is application listening port. Random from 3000 by default. 0 is not allowed.
	 * @brief Parameter parentPath is parent directory for logger. Root of build directory or executable directory by
	 * default (512 bytes are reserved).
	 * @brief Parameter logLevel is WARNING by default.
	 * @brief Parameter logInConsole is managing logging in console. False by default.
	 * @brief Parameter logInFile is managing logging in file. False by default.
	 * @brief Parameter separateDaysLogging is managing separating log files by days. True by default.
	 * @brief Parameter name is name of app. Type by default.
	 *
	 * @param hash Type hash.
	 * @param parameters Application parameters in Json.
	 * @param error Error message.
	 *
	 * @return Application port or 0 if failed.
	 */
	[[nodiscard]] uint16_t CreateApp(uint64_t hash, const MSAPI::Json& parameters, std::string& error);
};

#endif //* MSAPI_APP_MANAGER_H