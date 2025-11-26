/**************************
 * @file        manager.h
 * @version     6.0
 * @date        2024-02-19
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

#ifndef MSAPI_APP_MANAGER_H
#define MSAPI_APP_MANAGER_H

#include "../../../library/source/help/json.h"
#include "../../../library/source/help/pthread.hpp"
#include "../../../library/source/protocol/http.h"
#include "../../../library/source/protocol/object.h"
#include "../../../library/source/server/authorization.hpp"
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
 * represents a string, like hash of instrument name for example, then custom string interpretation is used.
 */
class Manager : public MSAPI::Server, MSAPI::HTTP::IHandler {
private:
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
		const InstalledAppData* appData;
		int connection{ 0 };

		/**************************
		 * @brief Construct a new Created App Data object.
		 *
		 * @param hash Hash of app type.
		 * @param pid Process id.
		 * @param appData Pointer to installed app data.
		 */
		CreatedAppData(size_t hash, int pid, const InstalledAppData* appData);
	};

private:
	std::string m_webSourcesPath;
	//* type hash, appData
	std::map<size_t, InstalledAppData> m_installedAppDataToHash;
	std::map<uint16_t, CreatedAppData> m_createdAppToPort;
	MSAPI::Pthread::AtomicLock m_metadataRequestsLock;
	MSAPI::Pthread::AtomicLock m_parametersRequestsLock;
	MSAPI::Pthread::AtomicLock m_deleteRequestsLock;
	MSAPI::Pthread::AtomicLock m_createdAppToPortLock;

	//* Authorization module
	MSAPI::Authorization::Module<> m_authorization;

	/**************************
	 * @brief Allows postpone response to request when app response is required first.
	 *
	 * Scheme is next: Pure request info object is an indicator in queue of requests, that at this point were requested
	 * parameters to confirm some action (pause, run). And if more requests to same action is reserved before an answer
	 * is gotten, response for first such request will be the answer for each.
	 */
	class RequestInfo {
	public:
		enum class Type : int8_t { Metadata, Parameters, Pause, Run, Delete };

		/**************************
		 * @brief Data for managing income requests.
		 */
		struct Data {
			const int connection;
			const MSAPI::HTTP::Data data;

			/**************************
			 * @brief Construct a new Data object, start timer inside.
			 *
			 * @param connection Connection id.
			 * @param data Data of HTTP request.
			 */
			FORCE_INLINE Data(const int connection, const MSAPI::HTTP::Data& data) noexcept
				: connection{ connection }
				, data{ data }
			{
			}
		};

	private:
		const Type m_type;
		size_t m_identifier; // hash or app port (uint16_t)
		std::optional<Data> m_data;

		static constexpr time_t m_requestTimeout{ 120 };

	public:
		/**************************
		 * @brief Construct a new Request Info object, start timer inside data object and fill event map in manager.
		 *
		 * @param type Type of request.
		 * @param identifier Identifier: uint16_t for app port or size_t for app type hash.
		 * @param connection Connection id.
		 * @param data Data of HTTP request.
		 *
		 * @todo Add event for timeout.
		 */
		FORCE_INLINE RequestInfo(
			const Type type, const size_t identifier, const int connection, const MSAPI::HTTP::Data& data) noexcept
			: m_type{ type }
			, m_identifier{ identifier }
			, m_data{ Data{ connection, data } }
		{
		}

		/**************************
		 * @brief Construct a new Request Info object for identifying that one response is provided for all requests.
		 *
		 * @param type Type of request.
		 * @param identifier Identifier: uint16_t for app port or size_t for app type hash.
		 */
		FORCE_INLINE RequestInfo(const Type type, const size_t identifier) noexcept
			: m_type{ type }
			, m_identifier{ identifier }
		{
		}

		RequestInfo& operator==(const RequestInfo& other) = delete;
		const RequestInfo& operator=(const RequestInfo& other) = delete;
		RequestInfo(RequestInfo& other) = delete;
		RequestInfo(RequestInfo&& other) noexcept = default;

		/**************************
		 * @return Type of request.
		 */
		FORCE_INLINE Type GetType() const noexcept { return m_type; }

		/**************************
		 * @return Data of request.
		 */
		FORCE_INLINE const std::optional<Data>& GetData() const noexcept { return m_data; }

		/**************************
		 * @return App type hash.
		 */
		FORCE_INLINE size_t GetHash() const noexcept { return m_identifier; }

		/**************************
		 * @return App port.
		 */
		FORCE_INLINE uint16_t GetAppPort() const noexcept { return static_cast<uint16_t>(m_identifier); }
	};

	std::multimap<size_t, RequestInfo> m_metadataRequestsToHash;
	std::map<uint16_t, std::list<RequestInfo>> m_parametersRequestsToPort;
	std::multimap<uint16_t, RequestInfo> m_pauseRequestToPort;
	std::multimap<uint16_t, RequestInfo> m_runRequestToPort;
	std::multimap<uint16_t, RequestInfo> m_deleteRequestToPort;
	std::map<size_t, std::vector<MSAPI::StandardType::Type>> m_columnsToTableId;

public:
	/**************************
	 * @brief Construct a new Manager object, check access to /bin/bash, register parameters.
	 */
	Manager();

	/**************************
	 * @brief Lock all locks.
	 */
	~Manager();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::Application
	void HandleRunRequest() final;
	void HandlePauseRequest() final;
	void HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate) final;
	void HandleParameters(int connection, const std::map<size_t, std::variant<standardTypes>>& parameters) final;
	void HandleHello(int connection) final;
	void HandleMetadata(int connection, std::string_view metadata) final;
	//* MSAPI::HTTP::IHandler
	void HandleHttp(int connection, const MSAPI::HTTP::Data& data) final;

	/**************************
	 * @brief Read the execution status of vforked apps to prevent zombie processes and answer related requests in
	 * pending state. Should be setted as handler for SIGCHLD.
	 */
	void CheckVforkedApps();

private:
	/**************************
	 * @brief Create app from installed app data and parameters from HTTP request. Provide error message if failed.
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
	 * @param appDataIt Iterator to installed app data.
	 * @param data Data of HTTP request.
	 * @param error Error message.
	 *
	 * @return Application port or 0 if failed.
	 */
	uint16_t CreateApp(const std::map<size_t, InstalledAppData>::iterator& appDataIt, const MSAPI::HTTP::Data& data,
		std::string& error);

	/**************************
	 * @brief Send parameters request if application already connected and save data for future response.
	 *
	 * @param appPort Application port.
	 * @param appConnection Application connection.
	 * @param requestInfo Request info.
	 */
	template <bool Lock>
	void SendParametersRequest(const uint16_t appPort, const int appConnection, RequestInfo&& requestInfo)
	{
#define SAVE_REQUEST_INFO_TO_PORT                                                                                      \
	if (auto requestsIt{ m_parametersRequestsToPort.find(appPort) }; requestsIt != m_parametersRequestsToPort.end()) { \
                                                                                                                       \
		requestsIt->second.emplace_back(std::move(requestInfo));                                                       \
	}                                                                                                                  \
	else {                                                                                                             \
		m_parametersRequestsToPort.emplace(appPort, std::list<RequestInfo>{})                                          \
			.first->second.emplace_back(std::move(requestInfo));                                                       \
	}

		if constexpr (Lock) {
			MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_parametersRequestsLock };
			SAVE_REQUEST_INFO_TO_PORT
		}
		else {
			SAVE_REQUEST_INFO_TO_PORT
		}

		if (appConnection != 0) {
			MSAPI::StandardProtocol::SendParametersRequest(appConnection);
		}
#undef SAVE_REQUEST_INFO_TO_PORT
	}
};

#endif //* MSAPI_APP_MANAGER_H