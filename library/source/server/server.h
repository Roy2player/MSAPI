/**************************
 * @file        server.h
 * @version     6.0
 * @date        2023-12-11
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

#ifndef MSAPI_SERVER_H
#define MSAPI_SERVER_H

#include "../help/autoClearPtr.inl"
#include "../help/lock.inl"
#include "application.h"
#include "connection.inl"
#include "recvBuffer.inl"
#include <climits>
#include <fcntl.h>
#include <iomanip>
#include <list>
#include <netinet/tcp.h>
#include <optional>
#include <sys/mman.h>
#include <sys/resource.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>

namespace MSAPI {

/**************************
 * @brief Basic class with separate state for creating a server, contains core logic to manage connections. Server main
 * process is accepting income TCP connections. Has ability to open new outcome TCP connections. Outcome connection can
 * be marked as "needed to reconnect" and in this case server will try to re-open connection if it was closed not by
 * server itself. Main pthread listens income connections and create new pthreads for each. Any income data calls
 * HandleBuffer function which must be overridden. Server can listen only one IP and port. If size of buffer for recv
 * less than required it will be increased for particular connection, but can't be greater than limited by special
 * parameter. If income data size is greater than buffer size limit, all related amount of data will be read and dropped
 * from the socket.
 *
 * @brief Server is based on the Application class and overrides HandleRunRequest, HandlePauseRequest,
 * HandleModifyRequest and HandleDeleteRequest methods with default logic defined in MSAPI_HANDLE_RUN_REQUEST_PRESET,
 * MSAPI_HANDLE_PAUSE_REQUEST_PRESET, MSAPI_HANDLE_MODIFY_REQUEST_PRESET macros accordingly, which can be overridden.
 * More info in Application class.
 *
 * @brief Parameter 1000001 "Seconds between try to connect" is used for opening new connections or reconnection,
 * default is 5, minimum is 1.
 * @brief Parameter 1000002 "Limit of attempts to connection" is used for open new connections or reconnection, default
 * is 1000. If limit is reached, connect function will return false, minimum is 1.
 * @brief Parameter 1000003 "Limit of connections from one IP" is used for limit number of connections from one IP,
 * default is 5, minimum is 1.
 * @brief Const parameter 1000004 "Recv buffer size" is a default size of buffer for recv function inside connection
 * request handler. Default is 1024 bytes, minimum is 3 bytes. Will not be applied for already allocated buffers.
 * @brief Const parameter 1000005 "Recv buffer size limit" is a limit of buffer for recv function inside connection
 * request handler. Default is 8 megabytes, minimum is 1024 bytes. Will be applied for already allocated buffers.
 * @brief Const parameter 1000006 "Server state" is a state of server.
 * @brief Const parameter 1000007 "Max connections" is a SOMAXCONN number.
 * @brief Const parameter 1000008 "Listening IP" is a IP address of server to listen after starting.
 * @brief Const parameter 1000009 "Listening port" is a port of server to listen after starting.
 *
 * @brief Server state is internal variable which can be used for check server state and can't be managed outside.
 * @brief Initialization state - server is ready to open new connections. This is the first server state. Income data
 * will be processed.
 * @brief Running state - server is ready to accept and open new connections. Income data will be processed.
 * @brief Stopped state - server is stopped and can't accept or open new connections, will lead to end of main process.
 * All opened connections will be closed.
 *
 * @note Server can't listen more connections then SOMAXCONN number.
 *
 * @note Any TCP socket is opened with SO_REUSEADDR (true), SO_REUSEPORT (if supported - false) and TCP_NODELAY (true)
 * options.
 *
 * @note SetMlockallCurrentFuture function can be used to lock all current and future memory of the process.
 *
 * @todo SOMAXCONN is a system constant, but it is not guaranteed that it will be the same on all systems. Because of
 * MSAPI is a precompiled library, need to find a way make it runtime constant.
 *
 * @todo Application class should be based on the Server class, not vice versa.
 *
 * @todo Improve UID generation. Way with std::atomic counter is thread safe, but performance overhead is sensitive in
 * some cases. Way with int generation + check in container event worse. Probably it should be UID generator with two
 * uint64_t values.
 */
class Server : public Application {
public:
	enum State : int8_t { Undefined, Initialization, Running, Stopped, Max };

	/**************************
	 * @return String interpretation of server state enum.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] static constexpr std::string_view EnumToString(const State state) noexcept
	{
		// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.
		switch (state) {
		case State::Undefined:
			return "Undefined";
		case State::Initialization:
			return "Initialization";
		case State::Running:
			return "Running";
		case State::Stopped:
			return "Stopped";
		case State::Max:
			return "Max";
		default:
			LOG_ERROR_NEW("Unknown server state: {}", U(state));
			return "Unknown";
		}
	}

private:
	/**************************
	 * @brief Data structure to store information about limits per one IP address. Minimum of max connections is 1.
	 *
	 * @concurrency Requires external lock management.
	 */
	class IpLimits {
	private:
		std::unordered_set<uint64_t> m_connectionsId;
		Lock::AtomicRW m_lock;
		uint64_t m_maxConnections{ 1 };

	public:
		/**************************
		 * @brief Construct a new IpLimits object.
		 *
		 * @param maxConnections Maximum number of connections.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE explicit IpLimits(const uint64_t maxConnections) noexcept
		{
			(void)SetMaxConnections(maxConnections);
		}

		/**************************
		 * @brief Add a new connection ID to the set of connections.
		 *
		 * @param id New connection ID to add.
		 *
		 * @return True if the connection ID was added, false if the limit has been reached or the ID already exists.
		 *
		 * @locking Expects external lock management.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool AddConnectionId(const uint64_t id) noexcept
		{
			if (m_connectionsId.size() >= m_maxConnections) [[unlikely]] {
				LOG_WARNING_NEW(
					"Max connections limit reached: {}. Cannot add connection id: {}", m_maxConnections, id);
				return false;
			}

			if (m_connectionsId.insert(id).second) [[likely]] {
				return true;
			}

			LOG_WARNING_NEW("Connection id: {} already exists for this IP. Cannot add duplicate", id);
			return false;
		}

		/**************************
		 * @brief Remove a connection ID from the set of connections.
		 *
		 * @param id Connection ID to remove.
		 *
		 * @locking Expects external lock management.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void RemoveConnectionId(const uint64_t id) noexcept { m_connectionsId.erase(id); }

		/**************************
		 * @return Current number of connections.
		 *
		 * @locking Expects external lock management.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] size_t GetConnectionsCount() const noexcept { return m_connectionsId.size(); }

		/**************************
		 * @brief Set the maximum number of connections. In case the new value is less than the current number of
		 * connections, it will not close existing connections.
		 *
		 * @param value New maximum number of connections.
		 *
		 * @return True if the maximum was updated, false if the value is invalid or no change was needed.
		 *
		 * @locking Expects external lock management.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool SetMaxConnections(const uint64_t value) noexcept
		{
			if (value <= 1) [[unlikely]] {
				LOG_WARNING_NEW("Max connections cannot be less than 1, provided {}", value);
				return false;
			}

			if (value == m_maxConnections) [[unlikely]] {
				LOG_DEBUG_NEW("Max connections is already set to {}, no change needed", value);
				return false;
			}

			if (value >= m_connectionsId.size()) [[likely]] {
				LOG_DEBUG_NEW("Max connections is changed from {} to {}", m_maxConnections, value);
			}
			else {
				LOG_DEBUG_NEW("ax connections is changed from {} to {} and less than current connections count: ",
					m_maxConnections, value, m_connectionsId.size());
			}

			m_maxConnections = value;
			return true;
		}

		/**************************
		 * @return Internal lock for external management.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] Lock::AtomicRW& GetLock() noexcept { return m_lock; }
	};

private:
	std::unordered_map<uint64_t, std::shared_ptr<Connection::Data>> m_idToConnectionData;
	Lock::AtomicRW m_idToConnectionDataRWLock;
	std::map<std::string, std::shared_ptr<IpLimits>, std::less<>> m_ipToLimits;
	Lock::AtomicRW m_ipToLimitsRWLock;

	Lock::Atomic m_closingConnectionLocks;
	Lock::Atomic m_serverAcceptingLoop;
	Lock::AtomicRW m_alivePthreadsRWLock;
	Lock::AtomicRW m_stateLock;

	std::string m_listenIpStr;
	uint64_t m_maxConnectionsOneIp{ 5 };
	uint64_t m_limitConnectAttempts{ 1000 };

	//! Buffer size limit must be separated for each connection and removed from here
	uint64_t m_recvBufferSizeLimit{ 1024 * 1024 * 10 /* 10 megabytes */ };
	uint32_t m_secondsBetweenTryToConnect{ 1 };
	int32_t m_listeningSocket;
	sockaddr_in m_addr{ 0, 0, 0, 0 };
	std::atomic<int32_t> m_connectionIdGenerator{};
	uint16_t m_listenPort{};
	std::atomic<State> m_state{ State::Initialization };

	static inline constexpr int32_t m_somaxconn{ SOMAXCONN };

public:
	/**************************
	 * @brief Construct a new Server object, registration parameters.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Server() noexcept
	{
		static_assert(CHAR_BIT == 8, "CHAR_BIT is not 8");

		RegisterParameter(1000001, { "Seconds between try to connect", &m_secondsBetweenTryToConnect, 1 });
		RegisterParameter(1000002, { "Limit of attempts to connection", &m_limitConnectAttempts, 1 });
		RegisterParameter(1000003, { "Limit of connections from one IP", &m_maxConnectionsOneIp, 1 });
		RegisterParameter(1000005, { "Recv buffer size limit", &m_recvBufferSizeLimit, 1024 });
		// RegisterConstParameter(1000006, { "Server state", &m_state, &EnumToString });
		RegisterConstParameter(1000007, { "Max connections", &m_somaxconn });
		RegisterConstParameter(1000008, { "Listen IP", &m_listenIpStr });
		RegisterConstParameter(1000009, { "Listen port", &m_listenPort });
	}

	/**************************
	 * @brief Destroy the Server object, call Stop() inside and ensure main accepting loop is finished.
	 *
	 * @todo Add unit test.
	 */
	virtual ~Server() noexcept
	{
		Stop();
		m_serverAcceptingLoop.Lock();
		m_closingConnectionLocks.Lock();
		m_alivePthreadsRWLock.WriteLock();
	}

	Server(Server&& other) = delete;
	Server(const Server&) = delete;
	const Server& operator=(const Server&) = delete;
	const Server& operator=(Server&&) = delete;

	// Application
	void HandleRunRequest() override { MSAPI_HANDLE_RUN_REQUEST_PRESET; }
	void HandlePauseRequest() override { MSAPI_HANDLE_PAUSE_REQUEST_PRESET; }
	void HandleModifyRequest(const std::map<uint64_t, std::variant<standardTypes>>& parametersUpdate) override
	{
		MSAPI_HANDLE_MODIFY_REQUEST_PRESET;
	}
	void HandleDeleteRequest() override
	{
		HandlePauseRequest();
		Stop();
	}

	/**************************
	 * @brief Blocking start the main accepting loop to listen incoming connections. Wait for all pthreads to be
	 * finished on interruption.
	 *
	 * @note Interrupted if server gets Stopped state, if socket initialization failed or limit of listen connections is
	 * reached.
	 *
	 * @param ip IP address to listen.
	 * @param port Port to listen.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Start(const uint32_t ip, const uint16_t port) noexcept
	{
		MSAPI::Lock::Atomic::Guard _{ m_serverAcceptingLoop };

		auto state{ m_state.load(std::memory_order_acquire) };
		if (state != State::Initialization) [[unlikely]] {
			LOG_DEBUG_NEW("Server is not in initialization state and cannot be started, current state is {}",
				EnumToString(state));
			return;
		}

		m_addr.sin_addr.s_addr = htobe32(ip);
		m_listenPort = port;
		m_listenIpStr = Helper::GetStringIp(m_addr);
		m_addr.sin_port = htobe16(port);
		m_addr.sin_family = AF_INET;

		LOG_INFO("Starting server, IP: " + Helper::GetStringIp(m_addr) + ", port: " + _S(port));

		m_listeningSocket = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_listeningSocket == -1) [[unlikely]] {
			LOG_ERROR("Force stop. Socket constructor error");
			Stop();
			return;
		}

		if (!Bind(m_listeningSocket, &m_addr)) [[unlikely]] {
			LOG_ERROR("Force stop. Bind constructor throw");
			Stop();
			return;
		}
		if (!Listen(m_listeningSocket)) [[unlikely]] {
			LOG_ERROR("Force stop. Listen constructor throw");
			Stop();
			return;
		}
		LOG_INFO("Successfully server start");
		m_state.store(State::Running, std::memory_order_release);

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		AddPthreadAttributes(&attr);

		std::cout << "-->--> 1" << std::endl;

		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_idToConnectionDataRWLock };
			for (const auto& [id, data] : m_idToConnectionData) {
				std::cout << "-->--> 2 " << id << std::endl;
				Protocol::Standard::SendActionHello(data->GetConnection());
				std::cout << "-->--> 3 " << id << std::endl;
			}
		}

		const auto exit{ [this, &attr]() {
			LOG_DEBUG("Server state is Stopped, wait for pthreads to be finished");
			MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::write> _{ m_alivePthreadsRWLock };
			LOG_DEBUG("Server state is Stopped, all pthreads are finished, return");
			pthread_attr_destroy(&attr);
		} };

		sockaddr_in clientAddr{ 0, 0, 0, 0 };
		do {
			while (GetConnectionsCount() < m_somaxconn) {
				std::cout << "--> accepting start" << std::endl;
				auto newConnection{ Accept(m_listeningSocket, &clientAddr) };
				std::cout << "--> accepting end" << std::endl;
				state = m_state.load(std::memory_order_acquire);

				if (state == State::Stopped) [[unlikely]] {
					exit();
					return;
				}

				if (state != State::Running) [[unlikely]] {
					LOG_DEBUG_NEW("Server state is {}, continue to accept new connections", EnumToString(state));
					continue;
				}

				if (newConnection == nullptr) [[unlikely]] {
					continue;
				}

				(void)CreatePthread<Connection::Type::Income>(std::move(newConnection), Helper::GetStringIp(clientAddr),
					&attr, ip, port, /*doReconnection=*/false);
			}

			LOG_INFO(
				"Server can't accept new connection, limit: " + _S(m_somaxconn) + " reached. Sleep for 10 seconds");
			std::this_thread::sleep_for(std::chrono::seconds(10));
		} while (m_somaxconn >= GetConnectionsCount());

		pthread_attr_destroy(&attr);
		LOG_ERROR_NEW("Unexpected exit from the main accepting loop, server state is {}, connections counter is {}",
			EnumToString(m_state.load(std::memory_order_acquire)), GetConnectionsCount());
	}

	/**************************
	 * @brief Close connections, cancel child pthreads and clear containers. Set state to Stopped and close main
	 * listening socket which is an interrupt condition for main accepting loop.
	 *
	 * This function does not wait for pthreads to be finished as it can be called inside one.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Stop() noexcept
	{
		auto state{ m_state.load(std::memory_order_acquire) };
		if (state == State::Stopped) [[unlikely]] {
			LOG_DEBUG("Server is already stopped");
			return;
		}

		LOG_INFO("Server is stopping");
		m_state.store(State::Stopped, std::memory_order_release);;

		MSAPI::Lock::Atomic::Guard _{ m_closingConnectionLocks };

		if (m_listeningSocket != -1) [[likely]] {
			if (shutdown(m_listeningSocket, SHUT_RDWR) == -1) [[unlikely]] {
				LOG_ERROR_NEW("Listen socket shutdown is failed. Error №{}: {}", errno, std::strerror(errno));
			}
			if (close(m_listeningSocket) == -1) [[unlikely]] {
				LOG_ERROR_NEW("Listen socket close is failed. Error №{}: {}", errno, std::strerror(errno));
			}
		}

		{
			std::shared_ptr<Connection::Data> connectionData;
			do {
				{
					Lock::AtomicRW::Guard<Lock::read> _{ m_idToConnectionDataRWLock };
					if (m_idToConnectionData.empty()) {
						break;
					}

					connectionData = m_idToConnectionData.begin()->second;
				}

				Close(connectionData, /*doReconnection=*/false);
			} while (true);
		}

		LOG_INFO("Server is stopped");
	}

	/**************************
	 * @return State of server.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] State GetState() const noexcept
	{
		return m_state.load(std::memory_order_acquire);
	}

	/**************************
	 * @brief Open new connection and creates new pthread for it.
	 *
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param doReconnection If true, server will try to reconnect if connection was closed.
	 *
	 * @return Created connection data on success, nullptr otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE
	[[nodiscard]] std::shared_ptr<Connection::Data> OpenConnection(
		const uint32_t ip, const uint16_t port, const bool doReconnection) noexcept
	{
		return OpenConnectionImpl<unique, usual>(ip, port, doReconnection, /*oldId=*/0);
	}

	/**************************
	 * @brief Open new connection and creates new pthread for it.
	 *
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param doReconnection If true, server will try to reconnect if connection was closed.
	 *
	 * @return Created connection data on success, nullptr otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE
	[[nodiscard]] std::shared_ptr<Connection::Data> OpenManagerConnection(
		const uint32_t ip, const uint16_t port, const bool doReconnection) noexcept
	{
		return OpenConnectionImpl<unique, manager>(ip, port, doReconnection, /*oldId=*/0);
	}

	/**************************
	 * @brief Waiting for income data from connection by id, blocking function. Called inside separate pthread of
	 * accepted connection. Read size is defined by read data size of recv buffer. Default value of recv buffer reading
	 * is sizeof(uint64_t) * 2 bytes for MSAPI::DataHeader, it can be changed if required to allow handle another
	 * protocols.
	 *
	 * @tparam Type Connection type.
	 *
	 * @param connectionData Connection data structure.
	 *
	 * @todo Add unit test.
	 */
	template <Connection::Type Type> FORCE_INLINE void RecvLoop(const std::shared_ptr<Connection::Data>& connectionData)
	{
		RecvBuffer recvBuffer{ connectionData, &m_recvBufferSizeLimit, sizeof(uint64_t) * 2 };

		if (recvBuffer.GetData() != nullptr) [[likely]] {
			LOG_DEBUG_NEW("Recv loop is started for connection id {}", connectionData->GetConnectionId());
			while (true) {
				const auto action{ recvBuffer.Recv() };
				if (action.bufferSize == 0) [[unlikely]] {
					break;
				}

				const auto checkServerProtocol{ [this, &recvBuffer, &connectionData](const uint64_t limit) {
					const auto* data{ recvBuffer.GetData() };
					uint64_t lastNumber [[gnu::uninitialized]];
					memcpy(&lastNumber, data, sizeof(uint64_t));

					if (lastNumber % 934875930 < limit) {
						uint64_t size [[gnu::uninitialized]];
						memcpy(&size, data + sizeof(uint64_t), sizeof(uint64_t));
						if (size > sizeof(uint64_t) * 2) {
							if (!recvBuffer.RecvAdditional(size)) [[unlikely]] {
								return true;
							}
						}

						Application::Collect(connectionData,
							Protocol::Standard::Data{ DataHeader{ recvBuffer.GetBuffer() }, recvBuffer.GetData() });
						return true;
					}

					return false;
				} };

				if (recvBuffer.GetDataType() == 0 && recvBuffer.GetToProcessSize() >= sizeof(uint64_t) * 2) {
					if constexpr (Type == Connection::Type::Manager) {
						if (checkServerProtocol(10)) {
							continue;
						}
					}
					else {
						if (checkServerProtocol(3)) {
							continue;
						}
					}
				}

				HandleBuffer(recvBuffer);
			}
		}

		if constexpr (Type == Connection::Type::Outcome || Type == Connection::Type::Manager) {
			HandleOutcomeDisconnect(connectionData);
		}
		else {
			HandleIncomeDisconnect(connectionData);
		}

		if (m_state.load(std::memory_order_acquire) == State::Stopped) {
			return;
		}

		MSAPI::Lock::Atomic::Guard _{ m_closingConnectionLocks };
		Close(connectionData, connectionData->GetDoReconnection());
	}

	/**************************
	 * @locking Read locks m_idToConnectionDataRWLock to get the number of opened connections.
	 *
	 * @return Count of opened connections.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetConnectionsCount() noexcept
	{
		//! I do not really like this way to solve concurrency problem
		Lock::AtomicRW::Guard<Lock::read> _{ m_idToConnectionDataRWLock };
		return m_idToConnectionData.size();
	}

	/**************************
	 * @brief Set pthread attributes for new pthreads. This function is called before creating new pthreads and sets the
	 * required attributes for the pthreads to be created.
	 *
	 * @param attr Pointer to pthread attributes object.
	 *
	 * @pre attr != nullptr, must point to a valid pthread_attr_t object.
	 *
	 * @todo Clear description together with understanding of pthread attributes based on pthread intendent behaviour
	 * must here. That is possible, that not each attribute is required for each pthread.
	 */
	FORCE_INLINE static void AddPthreadAttributes(pthread_attr_t* const attr) noexcept
	{
		// The minimum pthread stack is only POSIX requirement, which does not takes into additional requirements, like
		// guard page, bookkeeping/padding and god knows what else.
		// As a side effect, not only pthread_create can return EAGAIN, but also pthread can crashes due to wrong
		// mangling! pthread_attr_setstacksize(&attr, UINT64(2 * PTHREAD_STACK_MIN));
		pthread_attr_setscope(attr, PTHREAD_SCOPE_PROCESS);
		pthread_attr_setschedpolicy(attr, SCHED_RR);
		pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
		//? pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED); Cant use it, because of SCHED_RR, but why?}
	}

	/**************************
	 * @brief Try to set soft and hard RLIMIT_MEMLOCK limits as RLIM_INFINITY and set mlockall as MCL_CURRENT and
	 * MCL_FUTURE. Locks all memory of the calling process into RAM. This is a privileged operation (requires the
	 * CAP_IPC_LOCK capability). Prints error message in cerr if failed and return 1.
	 *
	 * @return True on success, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] static bool SetMlockallCurrentFuture() noexcept
	{
		struct rlimit new_rlimit;
		new_rlimit.rlim_cur = RLIM_INFINITY;
		new_rlimit.rlim_max = RLIM_INFINITY;

		if (setrlimit(RLIMIT_MEMLOCK, &new_rlimit) != 0) {
			LOG_WARNING("Failed to set infinity RLIMIT_MEMLOCK");
			return false;
		}

		if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) [[unlikely]] {
			LOG_WARNING_NEW("mlockall failed. Error №{}: {}", errno, std::strerror(errno));
			return false;
		}

		return true;
	}

protected:
	/**************************
	 * @return Connection data by its id if exist, nullptr otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE std::shared_ptr<Connection::Data> GetConnectionData(const uint64_t id) noexcept
	{
		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_idToConnectionDataRWLock };
			auto it{ m_idToConnectionData.find(id) };
			if (it != m_idToConnectionData.end()) [[likely]] {
				return it->second;
			}
		}

		LOG_DEBUG_NEW("Connection with id {} is not found", id);
		return {};
	}

	/**************************
	 * @brief Handler for income data from connection by id. After calling need to decide which data contained
	 * inside and make decision about specific action.
	 *
	 * @param recvBuffer Recv buffer object.
	 */
	virtual void HandleBuffer(RecvBuffer& recvBuffer) = 0;

	/**************************
	 * @return Listen port.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint16_t GetListenPort() const noexcept { return m_listenPort; }

private:
	static inline constexpr bool unique{ true };
	static inline constexpr bool reconnection{ false };

	static inline constexpr bool usual{ true };
	static inline constexpr bool manager{ false };

	/**************************
	 * @brief Open connection and creates pthread for it.
	 *
	 * @tparam IsUnique Whether the connection is unique or a reconnection.
	 * @tparam IsUsual Whether the connection is usual or a manager.
	 *
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param doReconnection If true, server will try to reconnect if connection was closed.
	 * @param oldId Id of connection on reconnection.
	 *
	 * @return Created connection data on success, nullptr otherwise.
	 *
	 * @todo Add unit test.
	 */
	template <bool IsUnique, bool IsUsual>
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> OpenConnectionImpl(
		const uint32_t ip, const uint16_t port, const bool doReconnection, const uint64_t oldId) noexcept
	{
		sockaddr_in addr{ 0, 0, 0, 0 };
		addr.sin_addr.s_addr = htobe32(ip);
		addr.sin_port = htobe16(port);
		addr.sin_family = AF_INET;

		std::string ipStr{ Helper::GetStringIp(addr) };

		if (m_state.load(std::memory_order_acquire) == State::Stopped) [[unlikely]] {
			LOG_INFO_NEW("Connecting to {}:{} is interrupted, server is stopped", ipStr, port);
			return {};
		}

		const int32_t socket{ Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
		if (socket == -1) [[unlikely]] {
			LOG_ERROR_NEW("Connecting to {}:{} is failed", ipStr, port);
			return {};
		}

		uint64_t attempt{};
		do {
			std::cout << "--> try to connect socket " << socket << std::endl;
			if (connect(socket, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_in)) == 0) [[likely]] {
				break;
			}

			if (++attempt >= m_limitConnectAttempts) [[unlikely]] {
				LOG_ERROR_NEW("Limit of attempts {} connect to {}:{} is reached", m_limitConnectAttempts, ipStr, port);
				return {};
			}

			LOG_WARNING_NEW("Failed to connect to {}:{}. Error №{}: {}", ipStr, port, errno, std::strerror(errno));
			std::this_thread::sleep_for(std::chrono::seconds(m_secondsBetweenTryToConnect));

			if (m_state.load(std::memory_order_acquire) == State::Stopped) [[unlikely]] {
				LOG_INFO_NEW("Connecting to {}:{} is interrupted, server is stopped", ipStr, port);
				return {};
			}
		} while (false);

		std::unique_ptr<Connection> newConnection;
		if constexpr (IsUnique) {
			newConnection = std::make_unique<Connection>(socket);
		}
		else {
			newConnection = std::make_unique<Connection>(oldId, socket);
		}

		LOG_DEBUG_NEW("New connection id {} to {}:{} is just established", newConnection->GetId(), ipStr, port);

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		AddPthreadAttributes(&attr);

		const auto connectionData{ CreatePthread<IsUsual ? Connection::Type::Outcome : Connection::Type::Manager>(
			std::move(newConnection), std::move(ipStr), &attr, ip, port, doReconnection) };
		pthread_attr_destroy(&attr);
		if (connectionData != nullptr) [[likely]] {
			if (m_state.load(std::memory_order_acquire) == State::Running) {
				Protocol::Standard::SendActionHello(connectionData->GetConnection());
			}

			return connectionData;
		}

		return {};
	}

	/**************************
	 * @brief Create new pthread for connection and run RecvLoop function inside it. If pthread is not created,
	 * connection will be closed.
	 *
	 * @tparam Type Type of connection processing.
	 *
	 * @param connection Connection data structure.
	 * @param ipStr IP address of connection.
	 * @param pthreadAttr Pointer to pthread attributes object.
	 * @param ip IP address of connection.
	 * @param port Port of connection.
	 * @param doReconnection If reconnection is required for this connection.
	 *
	 * @pre connection != nullptr, must point to a valid Connection object.
	 * @pre pthreadAttr != nullptr, must point to a valid pthread_attr_t object.
	 *
	 * @locking Locks m_closingConnectionLocks to prevent closing connection while creating pthread.
	 * @locking Read locks m_alivePthreadsRWLock. Lock is remained until RecvLoop function is finished or pthread is not
	 * created.
	 *
	 * @return True if pthread is created successfully, false otherwise.
	 *
	 * @todo Add unit test.
	 */
	template <Connection::Type Type>
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> CreatePthread(std::unique_ptr<Connection>&& connection,
		std::string&& ipStr, const pthread_attr_t* const pthreadAttr, const uint32_t ip, const uint16_t port,
		const bool doReconnection) noexcept
	{
		MSAPI::Lock::Atomic::Guard guard{ m_closingConnectionLocks };

		const auto id{ connection->GetId() };

		if constexpr (Type == Connection::Type::Income) {
			if (!RegisterConnectionFromIp(id, ipStr)) [[unlikely]] {
				connection->Close();
				return {};
			}
		}

		const auto connectionData{ std::make_shared<Connection::Data>(
			std::move(connection), std::move(ipStr), ip, port, Type, doReconnection) };
		if (!connectionData->SetPthreadRecvLoop([this, connectionData]() { PthreadRecvLoop<Type>(connectionData); }))
			[[unlikely]] {
			return {};
		}

		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_idToConnectionDataRWLock };
			const auto [it, isSuccess] = m_idToConnectionData.emplace(id, connectionData);
			if (!isSuccess) [[unlikely]] {
				LOG_ERROR_NEW(
					"Attempt to save data for {} connection {} is failed", Connection::EnumToString(Type), id);
				return {};
			}
		}

		LOG_INFO_NEW("New {} connection id {}", Connection::EnumToString(Type), id);
		uint64_t pthreadId [[gnu::uninitialized]];
		while (true) {
			m_alivePthreadsRWLock.ReadLock();
			const auto result{ pthread_create(
				&pthreadId, pthreadAttr, &Connection::Data::Trampoline, connectionData->GetPthreadRecvLoop()) };

			if (result == 0 && connectionData->SetPthreadId(pthreadId)) [[likely]] {
				LOG_DEBUG_NEW(
					"Pthread is created successfully, {} connection id {}", Connection::EnumToString(Type), id);
				return connectionData;
			}

			m_alivePthreadsRWLock.ReadUnlock();
			if (result == EAGAIN) {
				LOG_DEBUG_NEW(
					"Pthread create returned EAGAIN, {} connection id {}", Connection::EnumToString(Type), id);
				continue;
			}

			LOG_ERROR_NEW("Pthread is not created, {} connection id {}. Error №{}: {}", Connection::EnumToString(Type),
				id, result, std::strerror(result));
			//! Problem with recursive inlining. This function should be alone.
			//! Close(connectionData, /*doReconnection=*/false);
			return {};
		}
	}

	/**************************
	 * @brief Bind socket.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Address to bind.
	 *
	 * @return True if socket was bind, false otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool Bind(const int32_t socket, const sockaddr_in* const addr) noexcept
	{
		if (bind(socket, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in)) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Socket is not bound. Error №{}: {}", errno, std::strerror(errno));
			return false;
		}

		LOG_DEBUG("Socket is bound successfully");
		return true;
	}

	/**************************
	 * @brief Listen socket.
	 *
	 * @param socket Socket descriptor.
	 *
	 * @return True if socket was listen, false otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool Listen(const int32_t socket) noexcept
	{
		if (listen(socket, m_somaxconn) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Socket is not listened. Error №{}: {}", errno, std::strerror(errno));
			return false;
		}

		LOG_DEBUG("Socket is listened successfully");
		return true;
	}

	/**************************
	 * @brief Accept income connection.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Address of income connection.
	 *
	 * @return Accepted connection, empty if interrupted or any error.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::unique_ptr<Connection> Accept(
		const int32_t socket, sockaddr_in* const addr) noexcept
	{
		auto sizeAddr{ static_cast<uint32_t>(sizeof(sockaddr_in)) };
		const auto result{ accept(socket, reinterpret_cast<sockaddr*>(addr), &sizeAddr) };
		if (result != -1) [[likely]] {
			auto connection{ std::make_unique<Connection>(result) };
			LOG_DEBUG_NEW("New connection {} is just accepted", connection->GetId());
			return connection;
		}

		if (m_state.load(std::memory_order_acquire) == State::Stopped) [[likely]] {
			LOG_DEBUG("Socket accepting is interrupted, server state is Stopped");
			return {};
		}

		LOG_ERROR_NEW("Socket accepting is interrupted. Error №{}: {}", errno, std::strerror(errno));
		return {};
	}

	/**************************
	 * @brief Clear containers, shutdown and close connection and run reconnection cycle if need.
	 *
	 * @attention Pthread which is responsible for connection will be cancelled when finished its work.
	 *
	 * @param connectionData Connection data structure.
	 * @param doReconnection If reconnection is required for this connection.
	 *
	 * @blocking Can block on reconnection cycle.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Close(const std::shared_ptr<Connection::Data>& connectionData,
		const bool doReconnection /* separate to make overridable */) noexcept
	{
		auto& conenction{ connectionData->GetConnection() };
		conenction.Close();
		const auto id{ conenction.GetId() };

		switch (connectionData->GetType()) {
		case Connection::Type::Outcome:
		case Connection::Type::Manager: {
			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_idToConnectionDataRWLock };
				m_idToConnectionData.erase(id);
			}

			{
				const auto ipStr{ connectionData->GetIpStr() };
				std::shared_ptr<IpLimits> ipLimits;
				do {
					{
						Lock::AtomicRW::Guard<Lock::read> _{ m_ipToLimitsRWLock };
						const auto it{ m_ipToLimits.find(ipStr) };
						if (it != m_ipToLimits.end()) {
							ipLimits = it->second;
						}
						else {
							break;
						}
					}

					Lock::AtomicRW::Guard<Lock::write> _{ ipLimits->GetLock() };
					(void)ipLimits->RemoveConnectionId(id);
				} while (false);

				LOG_INFO_NEW("Outcome connection is closed, id: {}, IP: {}. Active connections counter is {}", id,
					ipStr, GetConnectionsCount());
			}
		}
			return;
		case Connection::Type::Income: {
			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_idToConnectionDataRWLock };
				m_idToConnectionData.erase(id);
			}

			LOG_INFO_NEW("Income connection is closed, id: {}, IP: {}, do reconnection: {}. Active connections "
						 "counter is {}",
				id, connectionData->GetIpStr(), GetConnectionsCount(), doReconnection);
			if (doReconnection) {
				std::this_thread::sleep_for(std::chrono::seconds(m_secondsBetweenTryToConnect));

				std::shared_ptr<Connection::Data> newConnectionData;
				if (connectionData->GetType() == Connection::Type::Outcome) {
					newConnectionData = OpenConnectionImpl<reconnection, usual>(
						connectionData->GetIp(), connectionData->GetPort(), /*doReconnection=*/true, id);
				}
				else {
					newConnectionData = OpenConnectionImpl<reconnection, manager>(
						connectionData->GetIp(), connectionData->GetPort(), /*doReconnection=*/true, id);
				}
				if (newConnectionData != nullptr) {
					HandleReconnect(newConnectionData);
				}
			}
		}
			return;
		default: {
			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_idToConnectionDataRWLock };
				m_idToConnectionData.erase(id);
			}
			LOG_WARNING_NEW("Unexpected type of connection is closed, id: {}, IP: {}. Active connections counter is {}",
				id, connectionData->GetIpStr(), GetConnectionsCount());
		}
			return;
		};
	}

	/**************************
	 * @brief Register connection from ip in respect to limits.
	 *
	 * @param id Id of connection.
	 * @param ip IP address of connection.
	 *
	 * @return True all limits are passed and connection is registered, false otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool RegisterConnectionFromIp(const uint64_t id, const std::string_view ip) noexcept
	{
		std::shared_ptr<IpLimits> ipLimits;
		do {
			{
				Lock::AtomicRW::Guard<Lock::read> _{ m_ipToLimitsRWLock };
				const auto it{ m_ipToLimits.find(ip) };
				if (it != m_ipToLimits.end()) {
					ipLimits = it->second;
					break;
				}
			}

			Lock::AtomicRW::Guard<Lock::write> _{ m_ipToLimitsRWLock };
			ipLimits = std::make_shared<IpLimits>(m_maxConnectionsOneIp);
			m_ipToLimits[std::string{ ip }] = ipLimits;
		} while (false);

		bool result [[gnu::uninitialized]];
		uint64_t connectionsCount [[gnu::uninitialized]];
		{
			Lock::AtomicRW::Guard<Lock::write> _{ ipLimits->GetLock() };
			result = ipLimits->AddConnectionId(id);
			connectionsCount = ipLimits->GetConnectionsCount();
		}

		if (result) [[likely]] {
			LOG_INFO_NEW("Connection is allowed, IP: {}, id: {}. Connections per IP: {}", ip, id, connectionsCount);
			return true;
		}

		LOG_INFO_NEW("Connection is denied, IP: {}, id: {}. Connections per IP: {}", ip, id, connectionsCount);
		return false;
	}

	/**************************
	 * @brief Handling function for new pthread with recv processing.
	 *
	 * @tparam Type Type of connection processing.
	 *
	 * @param connectionData Connection data structure.
	 *
	 * @return Always nullptr.
	 *
	 * @todo Add unit test.
	 */
	template <Connection::Type Type>
	FORCE_INLINE void* PthreadRecvLoop(const std::shared_ptr<Connection::Data>& connectionData)
	{
		// Read lock is incremented before attempting to create pthread and decremented on failure
		struct Guard {
			Lock::AtomicRW& rwLock;

			FORCE_INLINE Guard(Lock::AtomicRW& rwLock) noexcept
				: rwLock{ rwLock }
			{
			}

			FORCE_INLINE ~Guard() noexcept { rwLock.ReadUnlock(); }
		};

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

		const auto id{ connectionData->GetConnectionId() };
		const auto pid{ gettid() };
		Guard _{ m_alivePthreadsRWLock };

		LOG_DEBUG_NEW("Called the pthread function for connection id: {}, type: {}, PID: {}", id,
			Connection::EnumToString(Type), pid);
		RecvLoop<Type>(connectionData);
		LOG_DEBUG_NEW("Finished the pthread function for connection id: {}, type: {}, PID: {}", id,
			Connection::EnumToString(Type), pid);

		return nullptr;
	}

	/**************************
	 * @brief Create socket.
	 *
	 * @param domain Domain of socket.
	 * @param type Type of socket.
	 * @param protocol Protocol of socket.
	 *
	 * @return Socket descriptor if socket was created, -1 otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] static int32_t Socket(
		const int32_t domain, const int32_t type, const int32_t protocol) noexcept
	{
		const int32_t socketListen{ socket(domain, type, protocol) };
		if (socketListen == -1) [[unlikely]] {
			LOG_ERROR("Socket is not opened");
			return -1;
		}

		{
			int32_t enable{ 1 };
			/*
				This socket option tells the kernel to reuse a local socket in TIME_WAIT state, without waiting for its
				natural timeout to expire. If you're developing a server, setting this option can be useful, because it
				allows the server to restart without waiting for the timeout to expire when it has been shut down.
			*/
			if (setsockopt(socketListen, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int32_t)) < 0) [[unlikely]] {
				LOG_ERROR("Failed to set SO_REUSEADDR option to socket");
			}
		}
#ifdef SO_REUSEPORT
		{
			int32_t enable{ 0 };
			/*
				This is a more recent addition that allows multiple sockets on the same host to bind to the same port.
			   This can be useful for programs that want to do multicast or need to have multiple processes listening on
			   the same port. Note that this option is not available on all systems, which is why it's wrapped in an
			   #ifdef in your code.
			*/
			if (setsockopt(socketListen, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int32_t)) < 0) [[unlikely]] {
				LOG_ERROR("Failed to set SO_REUSEPORT option to socket");
			}
		}
#endif
		{
			int32_t enable{ 1 };
			/*
				This option is used to control the Nagle's algorithm for a socket. When enabled (set to 1), the
			   algorithm is disabled and the TCP stack will send out small packets without waiting to see if more data
			   is coming that could be included in the packets. This can reduce latency but may increase bandwidth
			   usage.
			*/
			if (setsockopt(socketListen, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int32_t)) < 0) [[unlikely]] {
				LOG_ERROR("Failed to set TCP_NODELAY option to socket");
			}
		}

		return socketListen;
	}

	// For SetInitializationState, in testing purposes
	friend class DaemonBase;
};

} // namespace MSAPI

#endif // MSAPI_SERVER_H