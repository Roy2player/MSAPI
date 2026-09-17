/**************************
 * @file        connection.inl
 * @date        2025-05-31
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

#ifndef MSAPI_CONNECTION_INL
#define MSAPI_CONNECTION_INL

#include "../help/basicSString.inl"
#include "../help/lock.inl"
#include <fcntl.h>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Asynchronous connection wrapper, contains related data and allows to change functional behavior.
 *
 * Default recv/send implementations do not close the underlying socket on failure; the owner/caller is expected to
 * handle error recovery and connection lifecycle.
 *
 * @note The dual abstraction design is chosen as a trade off to multiply steps establishing process:
 * - Connection accept or connect, creating Connection with unique id.
 * - Constraints checking and rejecting on any violation.
 * - Storing all information about established connection in one place.
 *
 * That allows to have whole process as clean as possible.
 *
 * @concurrency Yes.
 */
class Connection {
public:
	using RecvFuncT = std::function<int64_t(int32_t, void*, uint64_t, int32_t)>;
	using SendFuncT = std::function<int64_t(int32_t, const void*, uint64_t, int32_t)>;

public:
	enum class Type : int8_t { Undefined, Outcome, Manager, Income, Max };

public:
	/**************************
	 * @brief Connection data structure contains all the related information about established connection.
	 *
	 * @concurrency True.
	 */
	class Data {
	private:
		const std::unique_ptr<Connection> m_connection;
		std::function<void()> m_pthreadRecvLoop;
		SString<16> m_ipStr;
		uint64_t m_pthreadId{};
		const uint32_t m_ip;
		const uint16_t m_port;
		const Type m_type;
		const bool m_doReconnection;

	public:
		/**************************
		 * @brief Construct a new Data object, empty constructor.
		 *
		 * @param connection Connection object.
		 * @param ipStr String IP address of connection.
		 * @param ip IP address of connection.
		 * @param port Port of connection.
		 * @param type Type of connection.
		 * @param doReconnection If reconnection is required for this connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE explicit Data(std::unique_ptr<Connection>&& connection, SString<16>&& ipStr, uint32_t ip,
			uint16_t port, Type type, bool doReconnection) noexcept;

		Data(Data&&) = delete;
		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
		Data& operator=(Data&&) = delete;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return Connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] Connection& GetConnection() const noexcept;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return Connection id.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetConnectionId() const noexcept;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return String IP address of connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] std::string_view GetIpStr() const noexcept;

		/**************************
		 * @locking Synchronization is not required. It is assumed that the pthread ID is set only once right after
		 * pthread is created and does not change during the lifetime of the connection data.
		 *
		 * @return Pthread id.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetPthreadId() const noexcept;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return IP address of connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint32_t GetIp() const noexcept;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return Port of connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint16_t GetPort() const noexcept;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return Type of connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] Type GetType() const noexcept;

		/**************************
		 * @locking Synchronization is not required.
		 *
		 * @return If reconnection is required for this connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool GetDoReconnection() const noexcept;

	private:
		/**************************
		 * @brief Set the Pthread Recv Loop object
		 *
		 * @param func Pthread receive loop function.
		 *
		 * @locking Synchronization is not required. It is assumed that the function is set only once right after
		 * structure is created and does not change during the lifetime of the connection data.
		 *
		 * @return True on success and false if already set.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool SetPthreadRecvLoop(std::function<void()>&& func) noexcept;

		/**************************
		 * @locking Synchronization is not required. It is assumed that the function is set only once right after
		 * structure is created and does not change during the lifetime of the connection data.
		 *
		 * @return Pointer to the pthread receive loop function.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] std::function<void()>* GetPthreadRecvLoop() noexcept;

		/**************************
		 * @brief Set the pthread ID for this connection data. Does not allow changing the ID once it has been set.
		 *
		 * @param id New pthread ID.
		 *
		 * @locking Synchronization is not required. It is assumed that the pthread ID is set only once right after
		 * pthread is created and does not change during the lifetime of the connection data.
		 *
		 * @return True on success and false if already set.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool SetPthreadId(uint64_t id) noexcept;

		/**************************
		 * @brief Callable trampoline function for pthread creation. It is required because pthread_create expects a
		 * function pointer with a specific signature, and we want to use a member function of Data as the
		 * thread entry point.
		 *
		 * @param data Pointer to lambda with PthreadRecvLoop function for specific connection data.
		 *
		 * @pre data != nullptr, must point to a valid std::function<void()> object.
		 * @pre Pointed std::function<void()> life must exceed the lifetime of the pthread.
		 *
		 * @locking Synchronization is not required.
		 *
		 * @return Always nullptr.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] static void* Trampoline(void* data) noexcept;

		// Access to pthread related data: id, loop
		friend class Server;
	};

private:
	const uint64_t m_id;
	const int32_t m_connection;
	RecvFuncT m_recvFunc{ [](const int32_t fd, void* const buffer, const uint64_t size, const int32_t flags) noexcept {
		return static_cast<int64_t>(recv(fd, buffer, size, flags));
	} };
	SendFuncT m_sendFunc{ [](const int32_t fd, const void* const buffer, const uint64_t size,
							  const int32_t flags) noexcept {
		return static_cast<int64_t>(send(fd, buffer, size, flags));
	} };
	Lock::Atomic m_recvLock;
	Lock::Atomic m_sendLock;
	std::atomic<bool> m_isUsable{ true };
	bool m_isClosed{};

	static inline std::atomic<uint64_t> m_counter{};

public:
	/**************************
	 * @brief Construct new connection object with new unique id.
	 *
	 * @param connection Socket connection.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE Connection(int32_t connection) noexcept;

	/**************************
	 * @brief Construct new connection object with specific id.
	 *
	 * @param id Connection id.
	 * @param connection Socket connection.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE Connection(uint64_t id, int32_t connection) noexcept;

	Connection(const Connection& other) = delete;
	Connection(Connection&& other) = delete;
	Connection& operator=(const Connection& other) = delete;
	Connection& operator=(Connection&& other) = delete;

	/**************************
	 * @brief Perform one effective recv from connection. Is concurrency safe and won't be called on closed connection.
	 *
	 * @param buffer Pointer to buffer. It must have enough space.
	 * @param size Number of bytes to be read. It must be greater than 0.
	 * @param flags Flags for recv.
	 *
	 * @locking Lock m_recvLock inside.
	 *
	 * @return Number of read bytes; returns 0 on peer shutdown or on any unrecoverable recv error and connection is no
	 * longer usable.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Recv(void* const buffer, uint64_t size, int32_t flags);

	/**************************
	 * @brief Perform one effective send to connection. Is concurrency safe and won't be called on closed connection.
	 *
	 * @attention In case of connection closing initiated by other side the send can be called on just closed, but not
	 * marked as is closed connection. That is an incredibly rare, but still possible case. There is no connection
	 * access pattern to prevent that behaviour on application side.
	 *
	 * @param buffer Pointer to buffer. It must have enough data.
	 * @param size Number of bytes to be send. It must be greater than 0.
	 * @param flags Flags for send.
	 *
	 * @locking Lock m_sendLock inside.
	 *
	 * @return Number of send bytes; returns 0 on peer shutdown or on any send error and connection is no longer usable.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Send(const void* const buffer, uint64_t size, int32_t flags);

	/**************************
	 * @brief Override the default recv function. Is concurrency safe and won't be called on closed connection.
	 *
	 * @tparam T Recv function.
	 *
	 * @param f New recv function.
	 *
	 * @locking Lock m_recvLock inside.
	 *
	 * @todo Add tests coverage.
	 */
	template <typename T>
		requires std::is_convertible_v<T, RecvFuncT>
	FORCE_INLINE void SetRecv(T&& f) noexcept;

	/**************************
	 * @brief Override the default send function. Is concurrency safe and won't be called on closed connection.
	 *
	 * @tparam T Send function.
	 *
	 * @param f New send function.
	 *
	 * @locking Lock m_sendLock inside.
	 *
	 * @todo Add tests coverage.
	 */
	template <typename T>
		requires std::is_convertible_v<T, SendFuncT>
	FORCE_INLINE void SetSend(T&& f) noexcept;

	/**************************
	 * @locking Is not required.
	 *
	 * @return Id of connection.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetId() const noexcept;

	/**************************
	 * @brief Shutdown and close connection. Is not concurrency safe and won't be called on closed connection.
	 *
	 * @locking Lock m_recvLock and m_sendLock inside.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE void Close();

	/**************************
	 * @locking Is not required.
	 *
	 * @return String interpretation of connection type enum.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] static constexpr std::string_view EnumToString(Type type) noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
Connection::Data
---------------------------------------------------------------------------------*/

FORCE_INLINE Connection::Data::Data(std::unique_ptr<Connection>&& connection, SString<16>&& ipStr, const uint32_t ip,
	const uint16_t port, const Connection::Type type, const bool doReconnection) noexcept
	: m_connection{ std::move(connection) }
	, m_ipStr{ std::move(ipStr) }
	, m_ip{ ip }
	, m_port{ port }
	, m_type{ type }
	, m_doReconnection{ doReconnection }
{
}

FORCE_INLINE [[nodiscard]] bool Connection::Data::SetPthreadId(const uint64_t id) noexcept
{
	if (m_pthreadId != 0) [[unlikely]] {
		LOG_WARNING_NEW("Pthread id: {} cannot be changed for connection id: {}", m_pthreadId, m_connection->GetId());
		return false;
	}

	m_pthreadId = id;
	return true;
}

FORCE_INLINE [[nodiscard]] bool Connection::Data::SetPthreadRecvLoop(std::function<void()>&& func) noexcept
{
	if (m_pthreadRecvLoop != nullptr) [[unlikely]] {
		LOG_WARNING_NEW("Pthread recv loop cannot be changed for connection id: {}", m_connection->GetId());
		return false;
	}

	m_pthreadRecvLoop = std::move(func);
	return true;
}

FORCE_INLINE [[nodiscard]] std::function<void()>* Connection::Data::GetPthreadRecvLoop() noexcept
{
	return &m_pthreadRecvLoop;
}

FORCE_INLINE [[nodiscard]] Connection& Connection::Data::GetConnection() const noexcept { return *m_connection; }

FORCE_INLINE [[nodiscard]] uint64_t Connection::Data::GetConnectionId() const noexcept { return m_connection->GetId(); }

FORCE_INLINE [[nodiscard]] std::string_view Connection::Data::GetIpStr() const noexcept { return m_ipStr.Get(); }

FORCE_INLINE [[nodiscard]] uint64_t Connection::Data::GetPthreadId() const noexcept { return m_pthreadId; }

FORCE_INLINE [[nodiscard]] uint32_t Connection::Data::GetIp() const noexcept { return m_ip; }

FORCE_INLINE [[nodiscard]] uint16_t Connection::Data::GetPort() const noexcept { return m_port; }

FORCE_INLINE [[nodiscard]] Connection::Type Connection::Data::GetType() const noexcept { return m_type; }

FORCE_INLINE [[nodiscard]] bool Connection::Data::GetDoReconnection() const noexcept { return m_doReconnection; }

FORCE_INLINE [[nodiscard]] void* Connection::Data::Trampoline(void* data) noexcept
{
	static_cast<std::function<void()>*>(data)->operator()();
	return nullptr;
}

/*---------------------------------------------------------------------------------
Connection
---------------------------------------------------------------------------------*/

FORCE_INLINE Connection::Connection(const int32_t connection) noexcept
	: m_id{ m_counter.fetch_add(1, std::memory_order_relaxed) }
	, m_connection{ connection }
{
}

FORCE_INLINE Connection::Connection(const uint64_t id, const int32_t connection) noexcept
	: m_id{ id }
	, m_connection{ connection }
{
}

FORCE_INLINE [[nodiscard]] uint64_t Connection::Recv(void* const buffer, const uint64_t size, const int32_t flags)
{
	Lock::Atomic::Guard _{ m_recvLock };
	if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
		return 0;
	}

	while (true) {
		const auto result{ m_recvFunc(m_connection, buffer, size, flags) };

		if (result > 0) [[likely]] {
			return static_cast<uint64_t>(result);
		}

		if (result == 0) [[likely]] {
			// Not sure if it is required
			// pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
			m_isUsable.store(false, std::memory_order_release);
			LOG_INFO_NEW("Socket is closed by other side, connection id: {}", m_id);
			return 0;
		}

		if (errno == EINTR) {
			if (!m_isUsable.load(std::memory_order_relaxed)) {
				LOG_DEBUG_NEW("Recv returned EINTR on shutdown socket, connection id: {}", m_id);
				return 0;
			}

			LOG_DEBUG_NEW("Recv returned EINTR on working socket, connection id: {}", m_id);
			continue;
		}

		if (flags & MSG_PEEK) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				LOG_DEBUG_NEW("Non-blocking recv returned EAGAIN or EWOULDBLOCK, connection id: {}", m_id);
				continue;
			}
		}

		m_isUsable.store(false, std::memory_order_release);
		LOG_ERROR_NEW(
			"Recv returned unrecoverable error №{}: {}, connection id: {}", errno, std::strerror(errno), m_id);
		return 0;
	}
}

FORCE_INLINE [[nodiscard]] uint64_t Connection::Send(const void* const buffer, const uint64_t size, const int32_t flags)
{
	int64_t result [[indeterminate]];
	while (true) {
		{
			Lock::Atomic::Guard _{ m_sendLock };
			if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
				LOG_PROTOCOL_NEW("Attempt to send on inactive connection id: {} is interrupted", m_id);
				return 0;
			}
			result = m_sendFunc(m_connection, buffer, size, flags);
		}

		if (result > 0) [[likely]] {
			return static_cast<uint64_t>(result);
		}

		if (result == 0) {
			m_isUsable.store(false, std::memory_order_release);
			LOG_DEBUG_NEW("Send returns zero, connection id: {}", m_id);
			return 0;
		}

		if (errno == EINTR) {
			if (!m_isUsable.load(std::memory_order_relaxed)) {
				LOG_DEBUG_NEW("Send returned EINTR on shutdown socket, connection id: {}", m_id);
				return 0;
			}

			LOG_DEBUG_NEW("Send returned EINTR, connection id: {}", m_id);
			continue;
		}

		m_isUsable.store(false, std::memory_order_release);
		LOG_ERROR_NEW("Send failed with error №{}: {}, connection id: {}", errno, std::strerror(errno), m_id);
		return 0;
	}
}

template <typename T>
	requires std::is_convertible_v<T, Connection::RecvFuncT>
FORCE_INLINE void Connection::SetRecv(T&& f) noexcept
{
	Lock::Atomic::Guard _{ m_recvLock };
	if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
		return;
	}

	m_recvFunc = std::forward<T>(f);
}

template <typename T>
	requires std::is_convertible_v<T, Connection::SendFuncT>
FORCE_INLINE void Connection::SetSend(T&& f) noexcept
{
	Lock::Atomic::Guard _{ m_sendLock };
	if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
		return;
	}

	m_sendFunc = std::forward<T>(f);
}

FORCE_INLINE [[nodiscard]] uint64_t Connection::GetId() const noexcept { return m_id; }

FORCE_INLINE void Connection::Close()
{
	if (m_isClosed) [[unlikely]] {
		return;
	}

	m_isUsable.store(false, std::memory_order_release);
	m_isClosed = true;
	if (shutdown(m_connection, SHUT_RDWR) == -1) [[unlikely]] {
		if (errno == ENOTCONN) {
			LOG_DEBUG_NEW("Is already closed, connection id: {}", m_id);
		}
		else {
			LOG_WARNING_NEW("Shutdown is failed, connection id: {}. Error №{}: {}", m_id, errno, std::strerror(errno));
		}
	}

	Lock::Atomic::Guard recvGuard{ m_recvLock };
	Lock::Atomic::Guard sendGuard{ m_sendLock };

	if (close(m_connection) != -1) [[likely]] {
		LOG_DEBUG_NEW("Is closed, connection id: {}", m_id);
		return;
	}

	LOG_WARNING_NEW("Closing is failed, connection id: {}. Error №{}: {}", m_id, errno, std::strerror(errno));
}

FORCE_INLINE [[nodiscard]] constexpr std::string_view Connection::EnumToString(const Type type) noexcept
{
	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.
	switch (type) {
	case Type::Undefined:
		return "Undefined";
	case Type::Outcome:
		return "Outcome";
	case Type::Manager:
		return "Manager";
	case Type::Income:
		return "Income";
	case Type::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown connection type: {}", U(type));
		return "Unknown";
	}
}

} // namespace MSAPI

#endif // MSAPI_CONNECTION_INL