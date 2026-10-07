/**************************
 * @file        server.inl
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

#ifndef MSAPI_SERVER_INL
#define MSAPI_SERVER_INL

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

namespace Test {

namespace Unit {

class ServerObserver;

} // namespace Unit

} // namespace Test

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Asynchronous communication ability provider with independent connections lifecycle management.
 *
 * Basic class with own state. Its main blocking function Start sets running state and accepts income TCP connections.
 * Server can listen only one IPv4 address. It has ability to open new outcome TCP connections. Outcome connection can
 * be marked as "reconnectable" and in this case server will try to re-open connection if it was closed not by server
 * itself. Outcome connection can be opened with manager privileges to allow lifecycle control. Each connection has its
 * own pthread with recv cycle.  Income data is provided via HandleBuffer virtual callback and can be accessed as
 * RecvBuffer abstraction.
 *
 * Server is based on the Application class and overrides HandleRunRequest, HandlePauseRequest,
 * HandleModifyRequest and HandleDeleteRequest methods with default logic defined in MSAPI_HANDLE_RUN_REQUEST_PRESET,
 * MSAPI_HANDLE_PAUSE_REQUEST_PRESET, MSAPI_HANDLE_MODIFY_REQUEST_PRESET macros accordingly, which can be overridden.
 * More info in Application class.
 *
 * Parameters:
 * - Parameter 1000001 "Seconds between try to connect" is used for opening new connections or reconnection, default is
 * 5, minimum is 1.
 * - Parameter 1000002 "Limit of attempts to connection" is used for open new connections or reconnection, default is
 * 1000. If limit is reached, connect function will return false, minimum is 1.
 * - Parameter 1000003 "Limit of connections from one IP" is used for limit number of connections from one IP, default
 * is 5, minimum is 1.
 * - Const parameter 1000004 "Recv buffer size limit" is a limit of buffer for recv function inside connection
 * request handler. Default is 8 megabytes, minimum is 1024 bytes. Will be applied for newly allocated buffers.
 * - Const parameter 1000005 "Server state" is a state of server.
 * - Const parameter 1000006 "Listen IP" is a IP address of server to listen after starting.
 * - Const parameter 1000007 "Listen port" is a port of server to listen after starting.
 *
 * Server state is internal variable which can be used for check server state and can't be managed outside. Each time
 * server becomes Stopped, the internal stopped state counter is increased.
 * - Running state, server is ready to accept and open new connections. Income data will be processed.
 * - Stopped state, server is stopped and can't accept or open new connections, will lead to end of main accepting
 * process.
 * - Stopping state, server is finishing its main accepting loop.
 *
 * @attention Each TCP socket is opened with SO_REUSEADDR=true, SO_REUSEPORT=false if supported and TCP_NODELAY=true
 * options.
 * @attention SetMlockallCurrentFuture function can be used to lock all current and future memory of the process.
 * @attention Pthreads created by the server are not cancelable, cancellation is disabled at the beginning of each
 * pthread and is never enabled.
 *
 * @see RecvBuffer for recv behavior.
 * @see Connection for connection behavior.
 * @see Application for more information about related callbacks and parameters.
 *
 * @concurrency Yes.
 *
 * @todo Application class should be based on the Server class, not vice versa.
 * @todo m_listenIpStr should be replaced by SString.
 * @todo Parameters m_maxConnectionsOneIp, m_limitConnectAttempts, m_recvBufferSizeLimit, m_secondsBetweenTryToConnect
 * and m_listenPort should be atomic.
 * @todo m_stateTmp should be removed when application parameters will be atomic values.
 * @todo Add global connections limit.
 * @todo Parameter 1000003 "Limit of connections from one IP" is applied only to IPs seen for the first time,
 * existing IP limits keep the value they were created with.
 * @todo m_ipToLimits keeps an entry for every distinct client IP forever. Entries are small and kept on purpose, so
 * the next connection from a known IP requires only read lock of the container. If the count of distinct IPs becomes a
 * problem, empty entries can be purged when the container size exceeds a limit.
 */
class Server : public Application {
public:
	enum State : int8_t { Undefined, Running, Stopped, Stopping, Max };

	/**************************
	 * @brief Attributes of pthreads created by the library: detached state, as created pthreads are never joined.
	 * Scheduling and stack size are inherited defaults. Attributes are initialized once on construction and are only
	 * read after it.
	 *
	 * @attention If initialization fails, attributes are invalid and pthreads must not be created with them, validity
	 * is checked by IsValid.
	 *
	 * @concurrency Yes. Attributes are only read after construction, so pthreads can be created with them concurrently.
	 */
	class PthreadAttributes {
	private:
		pthread_attr_t m_attributes{};
		bool m_isValid{};

	public:
		/**************************
		 * @brief Construct a new PthreadAttributes object, initialize attributes and set detached state. Attributes are
		 * destroyed if setting fails, and stay invalid on any failure.
		 *
		 * @locking Is not required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE PthreadAttributes() noexcept;

		/**************************
		 * @brief Destroy the PthreadAttributes object, destroy attributes if they are valid.
		 *
		 * @locking Is not required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE ~PthreadAttributes() noexcept;

		PthreadAttributes(const PthreadAttributes&) = delete;
		PthreadAttributes(PthreadAttributes&&) = delete;
		PthreadAttributes& operator=(const PthreadAttributes&) = delete;
		PthreadAttributes& operator=(PthreadAttributes&&) = delete;

		/**************************
		 * @locking Is not required.
		 *
		 * @return True if attributes are initialized and pthreads can be created with them, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept;

		/**************************
		 * @pre IsValid().
		 *
		 * @locking Is not required.
		 *
		 * @return Const reference to attributes.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const pthread_attr_t& Get() const noexcept;
	};

	/**************************
	 * @locking Is not required.
	 *
	 * @return String interpretation of server state enum.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static constexpr std::string_view EnumToString(State state) noexcept;

private:
	/**************************
	 * @brief Data structure to store information about limits per one IP address. Minimum of max connections is 1.
	 *
	 * @concurrency Yes.
	 */
	class IpLimits {
	private:
		std::unordered_set<uint64_t> m_connectionsId;
		mutable Lock::AtomicRW m_lock;
		uint64_t m_maxConnections{ 1 };

	public:
		/**************************
		 * @brief Construct a new IpLimits object.
		 *
		 * @param maxConnections Maximum number of connections.
		 *
		 * @locking Is not required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit IpLimits(uint64_t maxConnections) noexcept;

		/**************************
		 * @brief Add a new connection ID to the set of connections.
		 *
		 * @param id New connection ID to add.
		 *
		 * @locking External write lock is required.
		 *
		 * @return True if the connection ID was added, false if the limit has been reached or the ID already exists.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool AddConnectionId(uint64_t id) noexcept;

		/**************************
		 * @brief Remove a connection ID from the set of connections.
		 *
		 * @param id Connection ID to remove.
		 *
		 * @locking External write lock is required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void RemoveConnectionId(uint64_t id) noexcept;

		/**************************
		 * @locking External read lock is required.
		 *
		 * @return Current number of connections.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] size_t GetConnectionsCount() const noexcept;

		/**************************
		 * @brief Set the maximum number of connections. In case the new value is less than the current number of
		 * connections, it will not close existing connections.
		 *
		 * @param value New maximum number of connections.
		 *
		 * @locking External write lock is required.
		 *
		 * @return True if the maximum was updated, false if the value is invalid or no change was needed.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool SetMaxConnections(uint64_t value) noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Internal lock for external management.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Lock::AtomicRW& GetLock() const noexcept;
	};

private:
	std::unordered_map<uint64_t, std::shared_ptr<Connection::Data>> m_idToConnectionData;
	mutable Lock::AtomicRW m_idToConnectionDataRWLock;
	std::unordered_map<uint64_t, std::shared_ptr<IpLimits>> m_ipToLimits;
	mutable Lock::AtomicRW m_ipToLimitsRWLock;

	mutable Lock::AtomicRW m_closingConnectionsLock;
	mutable Lock::Atomic m_serverAcceptingLoop;
	mutable Lock::AtomicRW m_alivePthreadsRWLock;

	SString<16> m_listenIp;
	std::string m_listenIpStr; // TODO: SString should be instead
	uint64_t m_maxConnectionsOneIp{ 5 }; // TODO: Atomic
	uint64_t m_limitConnectAttempts{ 1000 }; // TODO: Atomic

	uint64_t m_recvBufferSizeLimit{ 1024 * 1024 * 10 /* 10 megabytes */ }; // TODO: Atomic
	uint32_t m_secondsBetweenTryToConnect{ 1 }; // TODO: Atomic
	int32_t m_listeningSocket{ -1 };
	sockaddr_in m_addr{ 0, 0, 0, 0 };
	std::atomic<int32_t> m_connectionIdGenerator{};
	uint16_t m_listenPort{}; // TODO: Atomic
	std::atomic<uint64_t> m_stoppedStateCount{};
	std::atomic<State> m_state{ State::Stopped };
	// TODO: remove when application parameters will be atomic values
	State m_stateTmp{ State::Stopped };
	PthreadAttributes m_pthreadAttributes;

public:
	/**************************
	 * @brief Construct a new Server object, registration parameters.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Server() noexcept;

	/**************************
	 * @brief Destroy the Server object, call Stop(), ensure main accepting loop and all recv loop pthreads are
	 * finished.
	 *
	 * @locking Lock m_serverAcceptingLoop, write lock m_closingConnectionsLock and m_alivePthreadsRWLock inside.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE virtual ~Server() noexcept;

	Server(const Server&) = delete;
	Server(Server&&) = delete;
	Server& operator=(const Server&) = delete;
	Server& operator=(Server&&) = delete;

	// Application
	FORCE_INLINE void HandleRunRequest() override;
	FORCE_INLINE void HandlePauseRequest() override;
	FORCE_INLINE void HandleModifyRequest(const std::map<uint64_t, std::variant<standardTypes>>& parametersUpdate);
	FORCE_INLINE void HandleDeleteRequest() override;

	/**************************
	 * @brief Blocking start the main accepting loop to listen income connections. Set state as running. Send hello to
	 * all connections. Wait for all pthreads to be finished on interruption, increase stopped state counter and set
	 * state to Stopped.
	 *
	 * @attention Interrupted when the server enters the Stopping state or socket initialization fails. Startup errors
	 * leave the server restartable.
	 *
	 * @note If the server encounters too many open files (EMFILE or ENFILE), it will log a warning and retry after
	 * 100ms.
	 *
	 * @locking Holds lock on m_serverAcceptingLoop, read lock on m_idToConnectionDataRWLock on hello sending and write
	 * lock m_alivePthreadsRWLock on interruption.
	 *
	 * @param ip Address to listen.
	 * @param port Port to listen.
	 *
	 * @test Yes.
	 *
	 * @todo Make a waiting time on EMFILE and ENFILE errors configurable.
	 */
	FORCE_INLINE void Start(uint32_t ip, uint16_t port) noexcept;

	/**************************
	 * @brief Close connections, cancel child pthreads and clear containers. If was in running state - set state to
	 * Stopping and close main listening socket which is an interrupt condition for main accepting loop.
	 *
	 * @attention This function does not wait for pthreads to be finished as it can be called inside one.
	 *
	 * @locking Write lock m_closingConnectionsLock and read lock m_idToConnectionDataRWLock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Stop() noexcept;

	/**************************
	 * @locking Is not required.
	 *
	 * @return State of server.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] State GetState() const noexcept;

	/**************************
	 * @locking Is not required.
	 *
	 * @return Count of how many times server was stopped.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetStoppedStateCount() const noexcept;

	/**************************
	 * @brief Open new outcome connection.
	 *
	 * @attention Can block in attempts to connect until limit.
	 *
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param doReconnection If true, server will try to reconnect if connection was closed.
	 *
	 * @locking Perform locking in OpenConnectionImpl call.
	 *
	 * @return Created connection data on success, nullptr otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE
	[[nodiscard]] std::shared_ptr<Connection::Data> OpenConnection(
		uint32_t ip, uint16_t port, bool doReconnection) noexcept;

	/**************************
	 * @brief Open new manager connection. Manager connection has lifecycle control.
	 *
	 * @attention Can block in attempts to connect until limit.
	 *
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param doReconnection If true, server will try to reconnect if connection was closed.
	 *
	 * @locking Perform locking in OpenConnectionImpl call.
	 *
	 * @return Created connection data on success, nullptr otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE
	[[nodiscard]] std::shared_ptr<Connection::Data> OpenManagerConnection(
		uint32_t ip, uint16_t port, bool doReconnection) noexcept;

	/**************************
	 * @brief Blocking wait data from connection, calls HandleBuffer when data is ready to be processed further. On
	 * connection closing, effectively when recv return 0, call disconnection callback and close connection if server is
	 * still running.
	 *
	 * @note Is called inside own pthread.
	 * @note The readiness of data to be processed further is defined as minimum number of bytes to be read, by default
	 * is equal to MSAPI internal protocol header - 16 bytes.
	 *
	 * @see RecvBuffer for recv behavior.
	 * @see DataHeader for internal protocols identification.
	 *
	 * @tparam Type Connection type.
	 *
	 * @param connectionData Connection data structure.
	 *
	 * @locking Read lock m_closingConnectionsLock at loop exiting.
	 *
	 * @test Yes.
	 */
	template <Connection::Type Type>
	FORCE_INLINE void RecvLoop(const std::shared_ptr<Connection::Data>& connectionData);

	/**************************
	 * @locking Read lock m_idToConnectionDataRWLock inside.
	 *
	 * @return Count of opened connections.
	 *
	 * @todo When parameters become atomic, the connections counter should the one instead of that getter.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetConnectionsCount() const noexcept;

	/**************************
	 * @brief Try to set soft and hard RLIMIT_MEMLOCK limits as RLIM_INFINITY and set mlockall as MCL_CURRENT and
	 * MCL_FUTURE. Locks all memory of the calling process into RAM.
	 *
	 * @attention This is a privileged operation (requires the CAP_IPC_LOCK capability).
	 *
	 * @locking Is not required.
	 *
	 * @return True on success, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static bool SetMlockallCurrentFuture() noexcept;

protected:
	/**************************
	 * @locking Read lock m_idToConnectionDataRWLock inside.
	 *
	 * @return Connection data by its id if exist, nullptr otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE std::shared_ptr<Connection::Data> GetConnectionData(uint64_t id) const noexcept;

	/**************************
	 * @brief Handler for income data to be processed further.
	 *
	 * @note Due to the fact that each connection has its own pthread with recv loop and Stop can potentially be called
	 * inside the server abstraction cannot guaranty their finishing. That is a reason to pure virtual call exception in
	 * case: App based on server is going to be destructed, App destruction -> callback -> virtual table does not have
	 * handler -> exception. Provide default handler is the solution.
	 *
	 * @param recvBuffer Recv buffer object.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	virtual void HandleBuffer(RecvBuffer& recvBuffer);

	/**************************
	 * @locking Is not required.
	 *
	 * @return Listen port.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint16_t GetListenPort() const noexcept;

private:
	static inline constexpr bool UNIQUE{ true };
	static inline constexpr bool RECONNECTION{ false };

	static inline constexpr bool USUAL{ true };
	static inline constexpr bool MANAGER{ false };

	/**************************
	 * @brief Open new connection. Send hello if server is running.
	 *
	 * @attention Can block in attempts to connect until limit.
	 *
	 * @tparam IsUnique Whether the connection is unique or a reconnection.
	 * @tparam IsUsual Whether the connection is usual or a manager.
	 *
	 * @param ip IP address to connect.
	 * @param port Port to connect.
	 * @param doReconnection If true, server will try to reconnect if connection was closed.
	 * @param oldId Id of connection on reconnection.
	 *
	 * @locking Lock in CreatePthread call.
	 *
	 * @return Created connection data on success, nullptr otherwise.
	 *
	 * @test Yes.
	 */
	template <bool IsUnique, bool IsUsual>
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> OpenConnectionImpl(
		uint32_t ip, uint16_t port, bool doReconnection, uint64_t oldId) noexcept;

	/**************************
	 * @brief Create new pthread for recv loop.
	 *
	 * @attention If pthread is not created, connection will be closed. Pthread is not created if pthread attributes are
	 * not valid.
	 *
	 * @tparam Type Type of connection.
	 *
	 * @param connection Connection data structure.
	 * @param ipStr IP address of connection.
	 * @param ip IP address of connection.
	 * @param port Port of connection.
	 * @param doReconnection If reconnection is required for this connection.
	 *
	 * @pre connection != nullptr, must point to a valid Connection object.
	 *
	 * @locking Keep read lock m_closingConnectionsLock since the beginning.
	 * @locking Write lock m_idToConnectionDataRWLock.
	 * @locking Read lock m_alivePthreadsRWLock. Lock is remained until RecvLoop function is finished or pthread is not
	 * created.
	 *
	 * @return True if pthread is created successfully, false otherwise.
	 *
	 * @test Yes.
	 */
	template <Connection::Type Type>
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> CreatePthread(std::unique_ptr<Connection>&& connection,
		SString<16>&& ipStr, uint32_t ip, uint16_t port, bool doReconnection) noexcept;

	/**************************
	 * @brief Bind socket.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Address to bind.
	 *
	 * @locking Is not required.
	 *
	 * @return True if socket was bind, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static bool Bind(int32_t socket, const sockaddr_in* addr) noexcept;

	/**************************
	 * @brief Listen socket.
	 *
	 * @param socket Socket descriptor.
	 *
	 * @locking Is not required.
	 *
	 * @return True if socket was listen, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static bool Listen(int32_t socket) noexcept;

	/**************************
	 * @brief Accept income connection.
	 *
	 * @param socket Socket descriptor.
	 * @param addr Address of income connection.
	 *
	 * @locking Is not required.
	 *
	 * @return Accepted connection, empty if interrupted or any error.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] std::unique_ptr<Connection> Accept(int32_t socket, sockaddr_in* addr) noexcept;

	static inline constexpr bool RECONNECTION_IS_POSSIBLE{ true };
	static inline constexpr bool RECONNECTION_IS_NOT_POSSIBLE{ false };

	/**************************
	 * @brief Clear containers, shutdown and close connection. Perform attempt to reconnection for outcome connection
	 * and call reconnection callback on success.
	 *
	 * @attention Related to connection recv loop pthread is canceled asynchronously.
	 *
	 * @tparam HasReconnectionPath If the reconnection path is included in function. This compilation time parameter is
	 * required to simplify code inlining in CreatePthread path, where recursive inlining is a problem. There is
	 * constant false parameter is provided, but that is not recognized by compiler as the reason for branch
	 * optimization.
	 *
	 * @param connectionData Connection data structure.
	 * @param doReconnection If reconnection is required for this connection.
	 *
	 * @locking Write lock m_idToConnectionDataRWLock on erasing connection.
	 * @locking Lock in OpenConnectionImpl call on reconnection path.
	 * @locking Read lock m_ipToLimitsRWLock and write lock limits lock on income connection path.
	 *
	 * @test Yes.
	 */
	template <bool HasReconnectionPath>
	FORCE_INLINE void Close(const std::shared_ptr<Connection::Data>& connectionData,
		bool doReconnection /* separate to make overridable */) noexcept;

	/**************************
	 * @brief Register connection from ip in respect to limits.
	 *
	 * @param id Id of connection.
	 * @param ip Ip address of connection.
	 *
	 * @locking Read lock on limits lookup and further write lock on new ip limits creation. Write lock limits
	 * structure.
	 *
	 * @return True if all limits are passed and connection is registered, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool RegisterConnectionFromIp(uint64_t id, const SString<16>& ip) noexcept;

	/**************************
	 * @brief Unregister connection from ip limits, registered by RegisterConnectionFromIp. Limits of the ip are kept
	 * even if its last connection is unregistered.
	 *
	 * @param id Id of connection.
	 * @param ip Ip address of connection.
	 *
	 * @locking Read lock on limits lookup and further write lock limits structure.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void UnregisterConnectionFromIp(uint64_t id, std::string_view ip) noexcept;

	/**************************
	 * @brief Recv loop function for new pthread.
	 *
	 * @attention Disables cancellation of the pthread as the first action, the pthread is finished only by the end of
	 * the recv loop, when the connection is closed or the server is stopping.
	 *
	 * @tparam Type Type of connection processing.
	 *
	 * @param connectionData Connection data structure.
	 *
	 * @locking Read unlock m_alivePthreadsRWLock on exiting.
	 *
	 * @return Always nullptr.
	 *
	 * @test Yes.
	 */
	template <Connection::Type Type>
	FORCE_INLINE void* PthreadRecvLoop(const std::shared_ptr<Connection::Data>& connectionData);

	/**************************
	 * @brief Create socket and set SO_REUSEADDR=true, SO_REUSEPORT=false if supported and TCP_NODELAY=true options.
	 *
	 * @param domain Domain of socket.
	 * @param type Type of socket.
	 * @param protocol Protocol of socket.
	 *
	 * @locking Is not required.
	 *
	 * @return Socket descriptor if socket was created, -1 otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static int32_t Socket(int32_t domain, int32_t type, int32_t protocol) noexcept;

	// Direct access to IP limits in unit test
	friend class MSAPI::Test::Unit::ServerObserver;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
Server::PthreadAttributes
---------------------------------------------------------------------------------*/

FORCE_INLINE Server::PthreadAttributes::PthreadAttributes() noexcept
{
	//  - pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS) always fails with ENOTSUP on Linux, only
	//  PTHREAD_SCOPE_SYSTEM is supported, so it had no effect.
	//  - pthread_attr_setschedpolicy(&attr, SCHED_RR) succeeds, but is ignored, because default inherit scheduler
	//  attribute is PTHREAD_INHERIT_SCHED and new pthread takes policy of the creating one (SCHED_OTHER).
	//  - pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) was not usable with SCHED_RR, because SCHED_RR
	//  requires priority in [1, 99] (default 0 leads to EINVAL in pthread_create) and CAP_SYS_NICE or non-zero
	//  RLIMIT_RTPRIO (otherwise EPERM). Real-time round-robin for each connection pthread can also starve the system.
	//  - pthread_attr_setstacksize(&attr, 2 * PTHREAD_STACK_MIN) is too small, PTHREAD_STACK_MIN is only a POSIX lower
	//  bound and does not include guard page and runtime needs, so it leads to stack overflow. Default stack size of
	//  glibc (RLIMIT_STACK, usually 8 megabytes) is used.

	if (const auto result{ pthread_attr_init(&m_attributes) }; result != 0) [[unlikely]] {
		LOG_ERROR_NEW("Failed to initialize pthread attributes. Error №{}: {}", result, std::strerror(result));
		return;
	}

	// Created pthreads are never joined
	if (const auto result{ pthread_attr_setdetachstate(&m_attributes, PTHREAD_CREATE_DETACHED) }; result != 0)
		[[unlikely]] {

		LOG_ERROR_NEW(
			"Failed to set detached state of pthread attributes. Error №{}: {}", result, std::strerror(result));
		(void)pthread_attr_destroy(&m_attributes);
		return;
	}

	m_isValid = true;
}

FORCE_INLINE Server::PthreadAttributes::~PthreadAttributes() noexcept
{
	if (m_isValid) {
		(void)pthread_attr_destroy(&m_attributes);
	}
}

FORCE_INLINE [[nodiscard]] bool Server::PthreadAttributes::IsValid() const noexcept { return m_isValid; }

FORCE_INLINE [[nodiscard]] const pthread_attr_t& Server::PthreadAttributes::Get() const noexcept
{
	return m_attributes;
}

/*---------------------------------------------------------------------------------
Server::IpLimits
---------------------------------------------------------------------------------*/

FORCE_INLINE Server::IpLimits::IpLimits(const uint64_t maxConnections) noexcept
{
	(void)SetMaxConnections(maxConnections);
}

FORCE_INLINE [[nodiscard]] bool Server::IpLimits::AddConnectionId(const uint64_t id) noexcept
{
	if (m_connectionsId.size() >= m_maxConnections) [[unlikely]] {
		LOG_WARNING_NEW("Max connections limit reached: {}. Cannot add connection id: {}", m_maxConnections, id);
		return false;
	}

	if (m_connectionsId.insert(id).second) [[likely]] {
		return true;
	}

	LOG_WARNING_NEW("Connection id: {} already exists for this IP. Cannot add duplicate", id);
	return false;
}

FORCE_INLINE void Server::IpLimits::RemoveConnectionId(const uint64_t id) noexcept { m_connectionsId.erase(id); }

FORCE_INLINE [[nodiscard]] size_t Server::IpLimits::GetConnectionsCount() const noexcept
{
	return m_connectionsId.size();
}

FORCE_INLINE [[nodiscard]] bool Server::IpLimits::SetMaxConnections(const uint64_t value) noexcept
{
	if (value == 0) [[unlikely]] {
		LOG_WARNING_NEW("Max connections cannot be less than 1, provided: {}", value);
		return false;
	}

	if (value == m_maxConnections) [[unlikely]] {
		LOG_DEBUG_NEW("Max connections is already set to: {}, no change needed", value);
		return false;
	}

	if (value >= m_connectionsId.size()) [[likely]] {
		LOG_DEBUG_NEW("Max connections is changed from: {} to: {}", m_maxConnections, value);
	}
	else {
		LOG_DEBUG_NEW("Max connections is changed from: {} to: {} and less than current connections count: {}",
			m_maxConnections, value, m_connectionsId.size());
	}

	m_maxConnections = value;
	return true;
}

FORCE_INLINE [[nodiscard]] Lock::AtomicRW& Server::IpLimits::GetLock() const noexcept { return m_lock; }

/*---------------------------------------------------------------------------------
Server
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] constexpr std::string_view Server::EnumToString(const State state) noexcept
{
	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.
	static_assert(U(State::Max) == 4, "Missed server state to string interpretation");

	switch (state) {
	case State::Undefined:
		return "Undefined";
	case State::Running:
		return "Running";
	case State::Stopped:
		return "Stopped";
	case State::Stopping:
		return "Stopping";
	case State::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown server state: {}", U(state));
		return "Unknown";
	}
}

FORCE_INLINE Server::Server() noexcept
{
	static_assert(CHAR_BIT == 8, "CHAR_BIT is not 8");

	RegisterParameter(1000001, { "Seconds between try to connect", &m_secondsBetweenTryToConnect, 1 });
	RegisterParameter(1000002, { "Limit of attempts to connection", &m_limitConnectAttempts, 1 });
	RegisterParameter(1000003, { "Limit of connections from one IP", &m_maxConnectionsOneIp, 1 });
	RegisterParameter(1000004, { "Recv buffer size limit", &m_recvBufferSizeLimit, 1024 });
	RegisterConstParameter(1000005, { "Server state", &m_stateTmp, &EnumToString });
	RegisterConstParameter(1000006, { "Listen IP", &m_listenIpStr });
	RegisterConstParameter(1000007, { "Listen port", &m_listenPort });
}

FORCE_INLINE Server::~Server() noexcept
{
	Stop();
	m_serverAcceptingLoop.Lock();
	m_closingConnectionsLock.WriteLock();
	m_alivePthreadsRWLock.WriteLock();
}

FORCE_INLINE void Server::HandleRunRequest() { MSAPI_HANDLE_RUN_REQUEST_PRESET; }

FORCE_INLINE void Server::HandlePauseRequest() { MSAPI_HANDLE_PAUSE_REQUEST_PRESET; }

FORCE_INLINE void Server::HandleModifyRequest(const std::map<uint64_t, std::variant<standardTypes>>& parametersUpdate)
{
	MSAPI_HANDLE_MODIFY_REQUEST_PRESET;
}

FORCE_INLINE void Server::HandleDeleteRequest()
{
	HandlePauseRequest();
	Stop();
}

FORCE_INLINE void Server::Start(const uint32_t ip, const uint16_t port) noexcept
{
	MSAPI::Lock::Atomic::Guard _{ m_serverAcceptingLoop };

	auto state{ m_state.load(std::memory_order_acquire) };
	if (state != State::Stopped) [[unlikely]] {
		LOG_DEBUG_NEW("Server is not in stopped state and cannot be started, current state: {}", EnumToString(state));
		return;
	}

	if (!m_pthreadAttributes.IsValid()) [[unlikely]] {
		LOG_ERROR("Pthread attributes are not valid, starting is interrupted");
		m_stoppedStateCount.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	m_addr.sin_addr.s_addr = htobe32(ip);
	if (!Helper::GetStringIp(m_addr.sin_addr, m_listenIp)) [[unlikely]] {
		(void)m_listenIp.Copy(std::string_view{ "unknown" });
	}

	m_listenIpStr = m_listenIp.Get();
	m_listenPort = port;
	m_addr.sin_port = htobe16(port);
	m_addr.sin_family = AF_INET;

	LOG_INFO_NEW("Starting server on {}:{}", m_listenIp.Get(), port);

	m_listeningSocket = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listeningSocket == -1) [[unlikely]] {
		m_listenIp.Clear();
		m_listenIpStr = "";
		m_listenPort = 0;
		LOG_ERROR("Socket constructor error, starting is interrupted");
		m_stoppedStateCount.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	if (!Bind(m_listeningSocket, &m_addr)) [[unlikely]] {
		(void)close(m_listeningSocket);
		m_listeningSocket = -1;
		m_listenIp.Clear();
		m_listenIpStr = "";
		m_listenPort = 0;
		LOG_ERROR("Bind constructor error, starting is interrupted");
		m_stoppedStateCount.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	if (!Listen(m_listeningSocket)) [[unlikely]] {
		(void)close(m_listeningSocket);
		m_listeningSocket = -1;
		m_listenIp.Clear();
		m_listenIpStr = "";
		m_listenPort = 0;
		LOG_ERROR("Listen constructor error, starting is interrupted");
		m_stoppedStateCount.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	LOG_INFO("Successfully server start");
	m_state.store(State::Running, std::memory_order_release);
	m_stateTmp = State::Running;

	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_idToConnectionDataRWLock };
		for (const auto& [id, data] : m_idToConnectionData) {
			Protocol::Standard::SendActionHello(data->GetConnection());
		}
	}

	sockaddr_in clientAddr{ 0, 0, 0, 0 };
	SString<16> clientIp;
	while (true) {
		auto newConnection{ Accept(m_listeningSocket, &clientAddr) };
		state = m_state.load(std::memory_order_acquire);

		// Not handed over connection is closed by its destructor
		if (state == State::Stopping) [[unlikely]] {
			break;
		}

		if (state != State::Running) [[unlikely]] {
			LOG_DEBUG_NEW("Server state: {}, continue to accept new connections", EnumToString(state));
			continue;
		}

		if (newConnection == nullptr) [[unlikely]] {
			if (errno == EMFILE || errno == ENFILE) [[unlikely]] {
				LOG_DEBUG("Too many open files, cannot accept new connection. Retrying in 100ms");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			continue;
		}

		if (!Helper::GetStringIp(clientAddr.sin_addr, clientIp)) [[unlikely]] {
			(void)clientIp.Copy(std::string_view{ "unknown" });
		}

		(void)CreatePthread<Connection::Type::Income>(
			std::move(newConnection), std::move(clientIp), ip, port, /*doReconnection=*/false);
	}

	m_listenIp.Clear();
	m_listenIpStr = "";
	m_listenPort = 0;
	LOG_DEBUG("Server state is Stopping, wait for pthreads to be finished");
	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_alivePthreadsRWLock };
	LOG_DEBUG("All pthreads are finished, server is stopped");
	m_stoppedStateCount.fetch_add(1, std::memory_order_relaxed);
	m_state.store(State::Stopped, std::memory_order_release);
	m_stateTmp = State::Stopped;
}

FORCE_INLINE void Server::Stop() noexcept
{
	auto state{ m_state.load(std::memory_order_acquire) };
	if (state != State::Running) [[unlikely]] {
		LOG_DEBUG_NEW("Server is not running, current state: {}", EnumToString(state));
		// To interrupt OpenConnectionImpl as it can be called on Stopped server state
		m_stoppedStateCount.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	LOG_INFO("Server is stopping");

	m_state.store(State::Stopping, std::memory_order_release);
	m_stateTmp = State::Stopping;

	if (m_listeningSocket != -1) [[likely]] {
		if (shutdown(m_listeningSocket, SHUT_RDWR) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Listen socket shutdown is failed. Error №{}: {}", errno, std::strerror(errno));
		}
		if (close(m_listeningSocket) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Listen socket close is failed. Error №{}: {}", errno, std::strerror(errno));
		}
		m_listeningSocket = -1;
	}

	{
		const MSAPI::Lock::AtomicRW::Guard<Lock::WRITE> _{ m_closingConnectionsLock };
		std::shared_ptr<Connection::Data> connectionData;
		do {
			{
				const Lock::AtomicRW::Guard<Lock::READ> _{ m_idToConnectionDataRWLock };
				if (m_idToConnectionData.empty()) {
					break;
				}

				connectionData = m_idToConnectionData.begin()->second;
			}

			Close<RECONNECTION_IS_NOT_POSSIBLE>(connectionData, /*doReconnection=*/false);
		} while (true);
	}

	LOG_INFO("Connections are closed");
}

FORCE_INLINE [[nodiscard]] Server::State Server::GetState() const noexcept
{
	return m_state.load(std::memory_order_acquire);
}

FORCE_INLINE [[nodiscard]] uint64_t Server::GetStoppedStateCount() const noexcept
{
	return m_stoppedStateCount.load(std::memory_order_relaxed);
}

FORCE_INLINE
[[nodiscard]] std::shared_ptr<Connection::Data> Server::OpenConnection(
	const uint32_t ip, const uint16_t port, const bool doReconnection) noexcept
{
	return OpenConnectionImpl<UNIQUE, USUAL>(ip, port, doReconnection, /*oldId=*/0);
}

FORCE_INLINE
[[nodiscard]] std::shared_ptr<Connection::Data> Server::OpenManagerConnection(
	const uint32_t ip, const uint16_t port, const bool doReconnection) noexcept
{
	return OpenConnectionImpl<UNIQUE, MANAGER>(ip, port, doReconnection, /*oldId=*/0);
}

template <Connection::Type Type>
FORCE_INLINE void Server::RecvLoop(const std::shared_ptr<Connection::Data>& connectionData)
{
	RecvBuffer recvBuffer{ connectionData, m_recvBufferSizeLimit, sizeof(uint64_t) * 2 };

	if (recvBuffer.GetData() != nullptr) [[likely]] {
		LOG_DEBUG_NEW("Recv loop is started for connection id: {}", connectionData->GetConnectionId());
		while (true) {
			const auto result{ recvBuffer.Recv() };
			if (result.GetBufferSize() == 0) [[unlikely]] {
				break;
			}

			const auto checkServerProtocol{ [this, &recvBuffer, &connectionData](const uint64_t limit) {
				const auto* data{ recvBuffer.GetData() };
				uint64_t cipher [[indeterminate]];
				memcpy(&cipher, data, sizeof(uint64_t));

				if (cipher >= 934875930 && cipher < 934875940 && cipher % 934875930 < limit) {
					uint64_t size [[indeterminate]];
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

	// Recv loop is always part of pthread, if it is interrupted due to server is stopping, it is already closed
	if (m_state.load(std::memory_order_acquire) == State::Stopping) {
		return;
	}

	const MSAPI::Lock::AtomicRW::Guard<Lock::READ> _{ m_closingConnectionsLock };
	Close<RECONNECTION_IS_POSSIBLE>(connectionData, connectionData->GetDoReconnection());
}

FORCE_INLINE [[nodiscard]] uint64_t Server::GetConnectionsCount() const noexcept
{
	const Lock::AtomicRW::Guard<Lock::READ> _{ m_idToConnectionDataRWLock };
	return m_idToConnectionData.size();
}

FORCE_INLINE [[nodiscard]] bool Server::SetMlockallCurrentFuture() noexcept
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

FORCE_INLINE std::shared_ptr<Connection::Data> Server::GetConnectionData(const uint64_t id) const noexcept
{
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_idToConnectionDataRWLock };
		const auto it{ m_idToConnectionData.find(id) };
		if (it != m_idToConnectionData.end()) [[likely]] {
			return it->second;
		}
	}

	LOG_DEBUG_NEW("Connection with id {} is not found", id);
	return {};
}

FORCE_INLINE void Server::HandleBuffer(RecvBuffer& recvBuffer)
{
	if (m_state.load(std::memory_order_acquire) == State::Stopping) [[likely]] {
		LOG_PROTOCOL_NEW(
			"Buffer from connection id: {} is dropped as server is stopping", recvBuffer.GetConnectionId());
		return;
	}

	LOG_ERROR("Pure virtual function is called");
}

FORCE_INLINE [[nodiscard]] uint16_t Server::GetListenPort() const noexcept { return m_listenPort; }

template <bool IsUnique, bool IsUsual>
FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> Server::OpenConnectionImpl(
	const uint32_t ip, const uint16_t port, const bool doReconnection, const uint64_t oldId) noexcept
{
	sockaddr_in addr{ 0, 0, 0, 0 };
	addr.sin_addr.s_addr = htobe32(ip);
	addr.sin_port = htobe16(port);
	addr.sin_family = AF_INET;

	SString<16> ipStr;
	if (!Helper::GetStringIp(addr.sin_addr, ipStr)) [[unlikely]] {
		(void)ipStr.Copy(std::string_view{ "unknown" });
	}

	if (m_state.load(std::memory_order_acquire) == State::Stopping) [[unlikely]] {
		LOG_INFO_NEW("Connecting to: {}:{} is interrupted, server is Stopping", ipStr, port);
		return {};
	}

	const auto stoppedStateCount{ m_stoppedStateCount.load(std::memory_order_relaxed) };

	const int32_t socket{ Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
	if (socket == -1) [[unlikely]] {
		LOG_ERROR_NEW("Connecting to: {}:{} is failed", ipStr, port);
		return {};
	}

	uint64_t attempt{};
	do {
		if (connect(socket, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_in)) == 0) [[likely]] {
			break;
		}

		if (++attempt >= m_limitConnectAttempts) [[unlikely]] {
			LOG_ERROR_NEW(
				"Limit of attempts: {} is reached during connection to: {}:{}", m_limitConnectAttempts, ipStr, port);
			(void)close(socket);
			return {};
		}

		LOG_WARNING_NEW("Failed to connect to: {}:{}. Error №{}: {}", ipStr, port, errno, std::strerror(errno));
		std::this_thread::sleep_for(std::chrono::seconds(m_secondsBetweenTryToConnect));

		if (stoppedStateCount != m_stoppedStateCount.load(std::memory_order_relaxed)) [[unlikely]] {
			LOG_INFO_NEW("Connecting to: {}:{} is interrupted, stopped state count is increased", ipStr, port);
			(void)close(socket);
			return {};
		}
	} while (true);

	std::unique_ptr<Connection> newConnection;
	if constexpr (IsUnique) {
		newConnection = std::make_unique<Connection>(socket);
	}
	else {
		newConnection = std::make_unique<Connection>(oldId, socket);
	}

	LOG_DEBUG_NEW("New connection id: {} to: {}:{} is just established", newConnection->GetId(), ipStr, port);

	const auto connectionData{ CreatePthread < IsUsual
			? Connection::Type::Outcome
			: Connection::Type::Manager > (std::move(newConnection), std::move(ipStr), ip, port, doReconnection) };

	if (connectionData != nullptr) [[likely]] {
		if (m_state.load(std::memory_order_acquire) == State::Running) {
			Protocol::Standard::SendActionHello(connectionData->GetConnection());
		}

		return connectionData;
	}

	return {};
}

template <Connection::Type Type>
FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> Server::CreatePthread(
	std::unique_ptr<Connection>&& connection, SString<16>&& ipStr, const uint32_t ip, const uint16_t port,
	const bool doReconnection) noexcept
{
	if (!m_pthreadAttributes.IsValid()) [[unlikely]] {
		LOG_ERROR_NEW("Pthread is not created, pthread attributes are not valid, {} connection id: {}",
			Connection::EnumToString(Type), connection->GetId());
		return {};
	}

	const MSAPI::Lock::AtomicRW::Guard<Lock::READ> _{ m_closingConnectionsLock };

	const auto id{ connection->GetId() };

	if constexpr (Type == Connection::Type::Income) {
		if (!RegisterConnectionFromIp(id, ipStr)) [[unlikely]] {
			connection->Close();
			return {};
		}
	}

	const auto connectionData{ std::make_shared<Connection::Data>(
		std::move(connection), std::move(ipStr), ip, port, Type, doReconnection) };
	// Connection is closed by its destructor on failure paths, as connectionData is the only owner of the connection
	// data. Recv loop function receives connection data from pthread to not own it, see Connection::Data::Trampoline
	if (!connectionData->SetPthreadRecvLoop([this](const std::shared_ptr<Connection::Data>& data) {
			(void)PthreadRecvLoop<Type>(data);
		})) [[unlikely]] {

		if constexpr (Type == Connection::Type::Income) {
			UnregisterConnectionFromIp(id, connectionData->GetIpStr());
		}
		return {};
	}

	bool isSaved [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_idToConnectionDataRWLock };
		isSaved = m_idToConnectionData.emplace(id, connectionData).second;
	}

	if (!isSaved) [[unlikely]] {
		LOG_ERROR_NEW("Failed attempt to save data for {} connection id: {}", Connection::EnumToString(Type), id);
		if constexpr (Type == Connection::Type::Income) {
			UnregisterConnectionFromIp(id, connectionData->GetIpStr());
		}
		return {};
	}

	LOG_INFO_NEW("New {} connection id: {}", Connection::EnumToString(Type), id);
	uint64_t pthreadId [[indeterminate]];
	// Pthread owns a reference on connection data until its recv loop is finished, ownership is transferred on success
	auto pthreadConnectionData{ std::make_unique<std::shared_ptr<Connection::Data>>(connectionData) };
	while (true) {
		m_alivePthreadsRWLock.ReadLock();
		const auto result{ pthread_create(
			&pthreadId, &m_pthreadAttributes.Get(), &Connection::Data::Trampoline, pthreadConnectionData.get()) };

		if (result == 0) [[likely]] {
			(void)pthreadConnectionData.release();
			connectionData->SetPthreadId(pthreadId);
			LOG_DEBUG_NEW("Pthread is created successfully, {} connection id: {}", Connection::EnumToString(Type), id);
			return connectionData;
		}

		m_alivePthreadsRWLock.ReadUnlock();
		if (result == EAGAIN) {
			LOG_DEBUG_NEW("Pthread create returned EAGAIN, {} connection id: {}", Connection::EnumToString(Type), id);
			continue;
		}

		LOG_ERROR_NEW("Pthread is not created, {} connection id: {}. Error №{}: {}", Connection::EnumToString(Type), id,
			result, std::strerror(result));
		Close<RECONNECTION_IS_NOT_POSSIBLE>(connectionData, /*doReconnection=*/false);
		return {};
	}
}

FORCE_INLINE [[nodiscard]] bool Server::Bind(const int32_t socket, const sockaddr_in* const addr) noexcept
{
	if (bind(socket, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in)) == -1) [[unlikely]] {
		LOG_ERROR_NEW("Socket is not bound. Error №{}: {}", errno, std::strerror(errno));
		return false;
	}

	LOG_DEBUG("Socket is bound successfully");
	return true;
}

FORCE_INLINE [[nodiscard]] bool Server::Listen(const int32_t socket) noexcept
{
	if (listen(socket, SOMAXCONN) == -1) [[unlikely]] {
		LOG_ERROR_NEW("Socket is not listened. Error №{}: {}", errno, std::strerror(errno));
		return false;
	}

	LOG_DEBUG("Socket is listened successfully");
	return true;
}

FORCE_INLINE [[nodiscard]] std::unique_ptr<Connection> Server::Accept(
	const int32_t socket, sockaddr_in* const addr) noexcept
{
	auto sizeAddr{ static_cast<uint32_t>(sizeof(sockaddr_in)) };
	const auto result{ accept(socket, reinterpret_cast<sockaddr*>(addr), &sizeAddr) };
	if (result != -1) [[likely]] {
		auto connection{ std::make_unique<Connection>(result) };
		LOG_DEBUG_NEW("New connection id: {} is just accepted", connection->GetId());
		return connection;
	}

	if (m_state.load(std::memory_order_acquire) == State::Stopping) [[likely]] {
		LOG_DEBUG("Socket accepting is interrupted, server state is Stopping");
		return {};
	}

	LOG_ERROR_NEW("Socket accepting is interrupted {}. Error №{}: {}", socket, errno, std::strerror(errno));
	return {};
}

template <bool HasReconnectionPath>
FORCE_INLINE void Server::Close(const std::shared_ptr<Connection::Data>& connectionData,
	const bool doReconnection /* separate to make overridable */) noexcept
{
	auto& conenction{ connectionData->GetConnection() };
	conenction.Close();
	const auto id{ conenction.GetId() };

	switch (connectionData->GetType()) {
	case Connection::Type::Outcome:
	case Connection::Type::Manager: {
		{
			const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_idToConnectionDataRWLock };
			m_idToConnectionData.erase(id);
		}

		if constexpr (HasReconnectionPath == RECONNECTION_IS_POSSIBLE) {
			LOG_INFO_NEW("{} connection is closed, id: {}, {}:{}, do reconnection: {}. Active connections "
						 "counter: {}",
				Connection::EnumToString(connectionData->GetType()), id, connectionData->GetIpStr(),
				connectionData->GetPort(), doReconnection, GetConnectionsCount());

			if (doReconnection) {
				std::this_thread::sleep_for(std::chrono::seconds(m_secondsBetweenTryToConnect));

				std::shared_ptr<Connection::Data> newConnectionData;
				if (connectionData->GetType() == Connection::Type::Outcome) {
					newConnectionData = OpenConnectionImpl<RECONNECTION, USUAL>(
						connectionData->GetIp(), connectionData->GetPort(), /*doReconnection=*/true, id);
				}
				else {
					newConnectionData = OpenConnectionImpl<RECONNECTION, MANAGER>(
						connectionData->GetIp(), connectionData->GetPort(), /*doReconnection=*/true, id);
				}

				if (newConnectionData != nullptr) [[likely]] {
					HandleReconnect(newConnectionData);
				}
			}
		}
		else {
			LOG_INFO_NEW("{} connection is closed, id: {}, {}:{}, do reconnection: false. Active connections "
						 "counter: {}",
				Connection::EnumToString(connectionData->GetType()), id, connectionData->GetIpStr(),
				connectionData->GetPort(), GetConnectionsCount());
		}
	}
		return;
	case Connection::Type::Income: {
		{
			const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_idToConnectionDataRWLock };
			m_idToConnectionData.erase(id);
		}

		{
			const auto ipStr{ connectionData->GetIpStr() };
			UnregisterConnectionFromIp(id, ipStr);
			LOG_INFO_NEW("Income connection is closed, id: {}, {}:{}. Active connections counter: {}", id, ipStr,
				connectionData->GetPort(), GetConnectionsCount());
		}
	}
		return;
	default: {
		{
			const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_idToConnectionDataRWLock };
			m_idToConnectionData.erase(id);
		}
		LOG_WARNING_NEW("Unexpected type of connection is closed, id: {}, ip: {}. Active connections counter: {}", id,
			connectionData->GetIpStr(), GetConnectionsCount());
	}
		return;
	};
}

FORCE_INLINE [[nodiscard]] bool Server::RegisterConnectionFromIp(const uint64_t id, const SString<16>& ip) noexcept
{
	std::shared_ptr<IpLimits> ipLimits;
	do {
		const auto ipHash{ ip.Hash() };
		{
			const Lock::AtomicRW::Guard<Lock::READ> _{ m_ipToLimitsRWLock };
			const auto it{ m_ipToLimits.find(ipHash) };
			if (it != m_ipToLimits.end()) {
				ipLimits = it->second;
				break;
			}
		}

		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_ipToLimitsRWLock };
		// Limits can be created by another registration from the same ip between the read locked lookup above and
		// taking this write lock, then the existing limits are used
		const auto [it, isInserted] = m_ipToLimits.try_emplace(ipHash);
		if (isInserted) {
			it->second = std::make_shared<IpLimits>(m_maxConnectionsOneIp);
		}
		ipLimits = it->second;
	} while (false);

	bool result [[indeterminate]];
	uint64_t connectionsCount [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ ipLimits->GetLock() };
		result = ipLimits->AddConnectionId(id);
		connectionsCount = ipLimits->GetConnectionsCount();
	}

	if (result) [[likely]] {
		LOG_INFO_NEW(
			"Connection is allowed, ip: {}, connection id: {}. Connections per ip: {}", ip, id, connectionsCount);
		return true;
	}

	LOG_INFO_NEW("Connection is denied, ip: {}, connection id: {}. Connections per ip: {}", ip, id, connectionsCount);
	return false;
}

FORCE_INLINE void Server::UnregisterConnectionFromIp(const uint64_t id, const std::string_view ip) noexcept
{
	std::shared_ptr<IpLimits> ipLimits;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_ipToLimitsRWLock };
		const auto it{ m_ipToLimits.find(std::hash<std::string_view>{}(ip)) };
		if (it != m_ipToLimits.end()) [[likely]] {
			ipLimits = it->second;
		}
	}

	if (ipLimits == nullptr) [[unlikely]] {
		LOG_WARNING_NEW("Ip limits are not found for ip: {}, connection id: {}", ip, id);
		return;
	}

	const Lock::AtomicRW::Guard<Lock::WRITE> _{ ipLimits->GetLock() };
	ipLimits->RemoveConnectionId(id);
}

template <Connection::Type Type>
FORCE_INLINE void* Server::PthreadRecvLoop(const std::shared_ptr<Connection::Data>& connectionData)
{
	// Pthread is not cancelable, it is finished cooperatively and cleans up its resources itself
	if (const auto result{ pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr) }; result != 0) [[unlikely]] {
		LOG_ERROR_NEW("Failed to disable pthread cancellation. Error №{}: {}", result, std::strerror(result));
	}

	// Read lock is incremented before attempting to create pthread and decremented on create failure
	// In bad path there is no locking, on good path this guard is a guarantee it will be unlocked
	struct Guard {
		Lock::AtomicRW& rwLock;

		FORCE_INLINE Guard(Lock::AtomicRW& rwLock) noexcept
			: rwLock{ rwLock }
		{
		}

		FORCE_INLINE ~Guard() noexcept { rwLock.ReadUnlock(); }
	};
	const Guard _{ m_alivePthreadsRWLock };

	const auto id{ connectionData->GetConnectionId() };
	const auto pid{ gettid() };

	LOG_DEBUG_NEW("Called the pthread function for connection id: {}, type: {}, PID: {}", id,
		Connection::EnumToString(Type), pid);
	RecvLoop<Type>(connectionData);
	LOG_DEBUG_NEW("Finished the pthread function for connection id: {}, type: {}, PID: {}", id,
		Connection::EnumToString(Type), pid);

	return nullptr;
}

FORCE_INLINE [[nodiscard]] int32_t Server::Socket(
	const int32_t domain, const int32_t type, const int32_t protocol) noexcept
{
	const int32_t socketListen{ socket(domain, type, protocol) };
	if (socketListen == -1) [[unlikely]] {
		LOG_ERROR("Socket is not opened");
		return -1;
	}

	{
		int32_t enable{ 1 };
		// This socket option tells the kernel to reuse a local socket in TIME_WAIT state, without waiting for its
		// natural timeout to expire. If you're developing a server, setting this option can be useful, because it
		// allows the server to restart without waiting for the timeout to expire when it has been shut down.
		if (setsockopt(socketListen, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int32_t)) < 0) [[unlikely]] {
			LOG_ERROR("Failed to set SO_REUSEADDR option to socket");
		}
	}
#ifdef SO_REUSEPORT
	{
		int32_t enable{ 0 };
		// This is a more recent addition that allows multiple sockets on the same host to bind to the same port.
		// This can be useful for programs that want to do multicast or need to have multiple processes listening on
		// the same port. Note that this option is not available on all systems, which is why it's wrapped in an
		// #ifdef in your code.
		if (setsockopt(socketListen, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int32_t)) < 0) [[unlikely]] {
			LOG_ERROR("Failed to set SO_REUSEPORT option to socket");
		}
	}
#endif
	{
		int32_t enable{ 1 };
		// This option is used to control the Nagle's algorithm for a socket. When enabled (set to 1), the
		// algorithm is disabled and the TCP stack will send out small packets without waiting to see if more data
		// is coming that could be included in the packets. This can reduce latency but may increase bandwidth
		// usage.
		if (setsockopt(socketListen, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int32_t)) < 0) [[unlikely]] {
			LOG_ERROR("Failed to set TCP_NODELAY option to socket");
		}
	}

	return socketListen;
}

} // namespace MSAPI

#endif // MSAPI_SERVER_INL