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

#include "../help/pthread.hpp"
#include "application.h"
#include "recvBuffer.inl"
#include <list>
#include <optional>

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
 * @note Macros MSAPI_MLOCKALL_CURRENT_FUTURE can be placed in the beginning of a main function to lock all current
 * and future memory of the process, return 1 if failed.
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
	enum State : int16_t { Undefined, Initialization, Running, Stopped, Max };

	/**************************
	 * @brief Structure for automatic closing main listen socket and race protecting between accept cycle and function
	 * of stopping. If socket created failed, destructor will not try to close socket.
	 */
	struct AutoFreeSocket {
		const int socket;
		bool* socketCheck;

		/**************************
		 * @brief Construct a new Auto Free Socket object, empty constructor.
		 *
		 * @param socket Socket.
		 * @param socketCheck Writable and readable pointer to socket check.
		 */
		AutoFreeSocket(int socket, bool* socketCheck);

		/**************************
		 * @brief Destroy the Auto Free Socket object and close socket if was created successfully.
		 */
		~AutoFreeSocket();
	};

	/**************************
	 * @brief Data for managing outcome connections.
	 */
	struct ConnectionInfo {
		int id;
		in_addr_t ip;
		in_port_t port;
		const int connection;
		std::string textIp;
		bool needReconnection;

		/**************************
		 * @brief Construct a new Connection Info object, empty constructor.
		 */
		ConnectionInfo(int id, const in_addr_t& ip, const in_port_t& port, int connection, const std::string& textIp,
			bool needReconnection);
	};

private:
	enum class RecvProcessingType : short { Outcome, Income, Manager };

private:
	Pthread::AtomicLock m_closingConnectionLocks;
	Pthread::AtomicLock m_serverAcceptingLoop;
	Pthread::AtomicRWLock m_alivePthreadsRWLock;
	State m_state{ State::Initialization };
	sockaddr_in m_addr{ 0, 0, 0, 0 };
	in_port_t m_listeningPort{};
	std::string m_listeningIp;
	socklen_t m_sizeAddr{ sizeof(sockaddr_in) };
	//* Accepted connections
	std::map<int, int> m_connectionToId;
	std::map<int, pthread_t> m_pthreadToId;
	int m_connectionsCounter{};
	std::map<const std::string, std::list<int>> m_connectionsToIp;
	std::map<int, const std::string> m_IpToConnection;
	size_t m_maxConnectionsOneIp{ 5 };
	std::map<int, ConnectionInfo> m_infoToConnection;
	std::map<int, std::pair<Server*, const int*>> m_dataToPthreads;
	AutoFreeSocket* m_socketListen{ nullptr };
	unsigned int m_secondsBetweenTryToConnect{ 1 };
	size_t m_limitConnectAttempts{ 1000 };
	size_t m_recvBufferSize{ 1024 };
	size_t m_recvBufferSizeLimit{ 1024 * 1024 * 10 /* 10 megabytes */ };
	std::atomic<int32_t> m_connectionIdGenerator{};

	static constexpr int m_somaxconn{ SOMAXCONN };

public:
	/**************************
	 * @brief Construct a new Server object, registration parameters.
	 */
	Server();

	/**************************
	 * @brief Destroy the Server object, call Stop() inside and ensure main accepting loop is finished.
	 */
	virtual ~Server();

	//* Application
	void HandleRunRequest() override;
	void HandlePauseRequest() override;
	void HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate) override;
	void HandleDeleteRequest() override;

	/**************************
	 * @brief Blocking start the main accepting loop to listen incoming connections. Wait for all pthreads to be
	 * finished on interruption.
	 *
	 * @note Interrupted if Stop() is called, if socket initialization failed or limit of listen connections reached.
	 *
	 * @param ip IP address to listen.
	 * @param port Port to listen.
	 */
	void Start(in_addr_t ip, in_port_t port);

	/**************************
	 * @brief Close all accepted and outcome connections, cancel all pthreads, clear all containers, set state to
	 * Stopped and close main listening socket which is an interrupt condition for main accepting loop. This function
	 * does not wait for pthreads to be finished as it can be called inside one.
	 */
	void Stop();

	/**************************
	 * @return True if server is in a running state, false otherwise.
	 */
	bool IsRunning() const noexcept;

	/**************************
	 * @return IP address of connection by id, empty if connection is unknown.
	 */
	std::string GetIp(int connection) const;

	/**************************
	 * @return State of server.
	 */
	State GetState() const noexcept;

	/**************************
	 * @return Seconds between try to connect.
	 */
	unsigned int GetSecondsBetweenTryToConnect() const noexcept;

	/**************************
	 * @return Limit connect attempts.
	 */
	size_t GetLimitConnectAttempts() const noexcept;

	/**************************
	 * @brief Close connection by id.
	 *
	 * @param id Id of connection.
	 */
	void CloseConnect(int id);

	/**************************
	 * @brief Open new connection to IP and port.
	 *
	 * @param id Id of connection.
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param needReconnection If true, server will try to reconnect if connection was closed.
	 *
	 * @return True if connection was opened, false otherwise.
	 */
	bool OpenConnect(int id, in_addr_t ip, in_port_t port, bool needReconnection = true);

	/**************************
	 * @brief Check if connection by id is open.
	 *
	 * @param id Id of connection.
	 *
	 * @return True if connection by id is open, false otherwise.
	 *
	 * @todo Same function for pair IP and port.
	 */
	bool ConnectIsOpen(int id);

	/**************************
	 * @brief Waiting for income data from connection by id, blocking function. Called inside separate pthread of
	 * accepted connection. Read size is defined by read data size of recv buffer. Default value of recv buffer reading
	 * is sizeof(size_t) * 2 bytes for MSAPI::DataHeader, it can be changed if required to allow handle another
	 * protocols.
	 *
	 * @tparam Type Type of connection processing.
	 *
	 * @param id Id of connection.
	 */
	template <RecvProcessingType Type> FORCE_INLINE void ConnectionRecvProcessing(const int id)
	{
		int connection;

		if constexpr (Type == RecvProcessingType::Income) {
			if (const auto connectionIt{ m_connectionToId.find(id) }; connectionIt != m_connectionToId.end())
				[[likely]] {
				connection = connectionIt->second;
			}
			else {
				LOG_ERROR("Income connection is not found, id: " + _S(id));
				return;
			}
		}
		else if constexpr (Type == RecvProcessingType::Outcome || Type == RecvProcessingType::Manager) {
			if (const auto connectionIt{ m_infoToConnection.find(id) }; connectionIt != m_infoToConnection.end())
				[[likely]] {
				connection = connectionIt->second.connection;
			}
			else {
				LOG_ERROR("Outcome connection is not found, id: " + _S(id));
				return;
			}
		}
		else {
			static_assert(sizeof(Type) + 1 == 0, "Unknown type of recv processing");
		}

		RecvBuffer recvBuffer{ &m_recvBufferSizeLimit, sizeof(uint64_t) * 2, connection, id };
		LOG_DEBUG_NEW("Recv loop is started for connection {} id {}", connection, id);

		while (true) {
			const auto action{ recvBuffer.Recv() };
			if (action.bufferSize == 0) [[unlikely]] {
				break;
			}

			const auto checkServerProtocol{ [this, &recvBuffer, connection](const size_t limit) {
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

					Application::Collect(connection,
						Protocol::Standard::Data{ DataHeader{ recvBuffer.GetBuffer() }, recvBuffer.GetData() });
					return true;
				}

				return false;
			} };

			if (recvBuffer.GetDataType() == 0 && recvBuffer.GetToProcessSize() >= sizeof(uint64_t) * 2) {
				if constexpr (Type == RecvProcessingType::Manager) {
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

		if constexpr (Type == RecvProcessingType::Outcome || Type == RecvProcessingType::Manager) {
			HandleOutcomeDisconnect(id, connection);
		}
		else {
			HandleIncomeDisconnect(id, connection);
		}

		if (m_state == State::Stopped) {
			return;
		}

		MSAPI::Pthread::AtomicLock::ExitGuard exitGuard{ m_closingConnectionLocks };
		Close(id, connection);
	}

	/**************************
	 * @return String representation of server state enum.
	 *
	 * @example Pause, Running, Stopped, Initialization, Unknown.
	 */
	static std::string_view EnumToString(State state);

protected:
	/**************************
	 * @return Get the Connect object by id, empty optional if connection is unknown.
	 */
	std::optional<int> GetConnect(int id) const;

	/**************************
	 * @brief Handler for income data from connection by id. After calling need to decide which data contained
	 * inside and make decision about specific action.
	 *
	 * @param recvBuffer Recv buffer object.
	 */
	virtual void HandleBuffer(RecvBuffer& recvBuffer) = 0;

	/**************************
	 * @return Number of listened port.
	 */
	in_port_t GetListenedPort() const;

private:
	/**************************
	 * @brief Create socket.
	 *
	 * @param domain Domain of socket.
	 * @param type Type of socket.
	 * @param protocol Protocol of socket.
	 * @param status Writable and readable reference to socket check.
	 *
	 * @return Socket descriptor if socket was created, -1 otherwise. Also set status to true if socket was created
	 * and false otherwise.
	 */
	int Socket(int domain, int type, int protocol, bool& status);

	/**************************
	 * @brief Bind socket to IP and port.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Address to bind.
	 *
	 * @return True if socket was bind, false otherwise.
	 */
	bool Bind(int socket, const sockaddr_in* addr);

	/**************************
	 * @brief Listen socket.
	 *
	 * @param socket Socket descriptor.
	 *
	 * @return True if socket was listen, false otherwise.
	 */
	bool Listen(int socket);

	/**************************
	 * @brief Try to connect to socket.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Readable pointer to sockaddr structure.
	 *
	 * @return True if socket was connected, false otherwise.
	 */
	bool Connect(int socket, const sockaddr_in* addr);

	/**************************
	 * @brief Accept income connection.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Address of income connection.
	 *
	 * @return Id of connection if connection was accepted, empty optional otherwise.
	 */
	std::optional<int> Accept(int socket, sockaddr_in* addr);

	/**************************
	 * @brief Clear containers, shutdown and close connection and run reconnection cycle if need.
	 *
	 * @attention Pthread which is responsible for connection will be cancelled when finished its work.
	 *
	 * @param id Id of connection.
	 * @param connection Connection to clear.
	 */
	void Close(int id, int connection);

	/**************************
	 * @brief Mark connection as non-reconnectable and close it by Close() function.
	 *
	 * @param info Connection info.
	 */
	void CloseConnect(ConnectionInfo& info);

	/**************************
	 * @brief Check if connection is allowed by IP limits.
	 *
	 * @param id Id of connection.
	 * @param ip IP address of connection.
	 *
	 * @return True if connection is allowed, false otherwise.
	 */
	bool IsConnectionAllowed(int id, const std::string& ip);

	/**************************
	 * @brief Interpret recv processing type to string.
	 */
	template <RecvProcessingType type> struct RecvProcessingTypeToString {
		static_assert(type == RecvProcessingType::Manager || type == RecvProcessingType::Outcome
				|| type == RecvProcessingType::Income,
			"Unknown recv processing type");

		static constexpr const char* value = type == RecvProcessingType::Outcome ? "outcome"
			: type == RecvProcessingType::Income								 ? "income"
			: type == RecvProcessingType::Manager								 ? "manager"
																				 : "unknown";
	};

	/**************************
	 * @brief Interpret recv processing type to string.
	 */
	template <RecvProcessingType Type>
	static constexpr const char* RecvProcessingTypeToString_v = RecvProcessingTypeToString<Type>::value;

	/**************************
	 * @brief Handling function for new pthread with recv processing.
	 *
	 * @tparam Type Type of connection processing.
	 *
	 * @param data Readable and writable pointer to data with pointer to the server and its id.
	 *
	 * @return Always nullptr.
	 */
	template <RecvProcessingType Type> static FORCE_INLINE void* PthreadRunner(void* data)
	{
		static_assert(Type == RecvProcessingType::Manager || Type == RecvProcessingType::Outcome
				|| Type == RecvProcessingType::Income,
			"Unknown recv processing type");

		struct PthreadLockGuard {
			Pthread::AtomicRWLock& rwLock;

			FORCE_INLINE PthreadLockGuard(Pthread::AtomicRWLock& rwLock) noexcept
				: rwLock{ rwLock }
			{
			}

			FORCE_INLINE ~PthreadLockGuard() noexcept { rwLock.ReadUnlock(); }
		};

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
		std::pair<Server*, int*> serverAndId = *static_cast<std::pair<Server*, int*>*>(data);
		int id{ *serverAndId.second };
		LOG_DEBUG("Pthread function for " + RecvProcessingTypeToString_v<Type>
			+ " connection " + _S(id) + " is called, PID: " + _S(gettid()));
		Server* server{ serverAndId.first };
		PthreadLockGuard pthreadGuard{ server->m_alivePthreadsRWLock };
		server->ConnectionRecvProcessing<Type>(id);
		server->m_pthreadToId.erase(id);

		LOG_DEBUG("Pthread function for " + RecvProcessingTypeToString_v<Type>
			+ " connection id: " + _S(id) + " is finished, PID: " + _S(gettid()));
		return nullptr;
	}

	//* For SetInitializationState, in testing purposes
	friend class DaemonBase;
};

}; //* namespace MSAPI

/**************************
 * @brief Try to set soft and hard RLIMIT_MEMLOCK limits as RLIM_INFINITY and set mlockall as MCL_CURRENT and
 * MCL_FUTURE. Locks all memory of the calling process into RAM. This is a privileged operation (requires the
 * CAP_IPC_LOCK capability). Prints error message in cerr if failed and return 1.
 */
#define MSAPI_MLOCKALL_CURRENT_FUTURE                                                                                  \
	struct rlimit new_rlimit;                                                                                          \
	new_rlimit.rlim_cur = RLIM_INFINITY;                                                                               \
	new_rlimit.rlim_max = RLIM_INFINITY;                                                                               \
                                                                                                                       \
	if (setrlimit(RLIMIT_MEMLOCK, &new_rlimit) != 0) {                                                                 \
		std::cerr << "Failed to set infinity RLIMIT_MEMLOCK" << std::endl;                                             \
		return 1;                                                                                                      \
	}                                                                                                                  \
                                                                                                                       \
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) [[unlikely]] {                                                       \
		std::cerr << "mlockall failed. Error №" << _S(errno) << ": " << std::strerror(errno) << std::endl;             \
		return 1;                                                                                                      \
	}

#endif //* MSAPI_SERVER_H