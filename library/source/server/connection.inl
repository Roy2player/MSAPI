/**************************
 * @file        connection.inl
 * @version     6.0
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

#include "../help/lock.inl"

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Thread-safe wrapper around a socket connection that allows overriding recv/send behaviour.
 *
 * Default recv/send implementations do not close the underlying socket on failure; the owner/caller is expected to
 * handle error recovery and connection lifecycle.
 */
class Connection {
public:
	using recv_func_t = std::function<int64_t(int32_t, void*, uint64_t, int32_t)>;
	using send_func_t = std::function<int64_t(int32_t, const void*, uint64_t, int32_t)>;

private:
	const uint64_t m_id{ m_counter.fetch_add(1, std::memory_order_relaxed) };
	const int32_t m_connection;
	recv_func_t m_recvFunc{ [](const int32_t fd, void* const buffer, const uint64_t size,
								const int32_t flags) noexcept {
		return static_cast<int64_t>(recv(fd, buffer, size, flags));
	} };
	send_func_t m_sendFunc{ [](const int32_t fd, const void* const buffer, const uint64_t size,
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
	 * @brief Construct new connection object.
	 *
	 * @param connection Socket connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Connection(const int32_t connection) noexcept
		: m_connection{ connection }
	{
	}

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
	 * @return Number of read bytes; returns 0 on peer shutdown or on any unrecoverable recv error.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Recv(void* const buffer, const uint64_t size, const int32_t flags)
	{
		contract_assert(buffer != nullptr);
		contract_assert(size != 0);

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
				LOG_INFO_NEW("Socket is closed by other side, connection id {}", m_id);
				return 0;
			}

			if (errno == EINTR) {
				if (!m_isUsable.load(std::memory_order_relaxed)) {
					LOG_DEBUG_NEW("Recv returned EINTR on shutdown socket, connection id {}", m_id);
					return 0;
				}

				LOG_DEBUG_NEW("Recv returned EINTR on working socket, connection id {}", m_id);
				continue;
			}

			if (flags & MSG_PEEK) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					LOG_DEBUG_NEW("Non-blocking recv returned EAGAIN or EWOULDBLOCK, connection id {}", m_id);
					continue;
				}
			}

			m_isUsable.store(false, std::memory_order_release);
			LOG_ERROR_NEW(
				"Recv returned unrecoverable error №{}: {}, connection id {}", errno, std::strerror(errno), m_id);
			return 0;
		}
	}

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
	 * @return Number of send bytes; returns 0 on peer shutdown or on any send error.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Send(const void* const buffer, const uint64_t size, const int32_t flags)
	{
		contract_assert(buffer != nullptr);
		contract_assert(size != 0);

		int64_t result [[gnu::uninitialized]];
		while (true) {
			{
				Lock::Atomic::Guard _{ m_sendLock };
				if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
					return 0;
				}
				result = m_sendFunc(m_connection, buffer, size, flags);
			}

			if (result > 0) [[likely]] {
				return static_cast<uint64_t>(result);
			}

			if (result == 0) {
				m_isUsable.store(false, std::memory_order_release);
				LOG_DEBUG_NEW("Send returns zero, connection id {}", m_id);
				return 0;
			}

			if (errno == EINTR) {
				if (!m_isUsable.load(std::memory_order_relaxed)) {
					LOG_DEBUG_NEW("Send returned EINTR on shutdown socket, connection id {}", m_id);
					return 0;
				}

				LOG_DEBUG_NEW("Send returned EINTR, connection id {}", m_id);
				continue;
			}

			m_isUsable.store(false, std::memory_order_release);
			LOG_ERROR_NEW("Send failed with error №{}: {}, connection id {}", errno, std::strerror(errno), m_id);
			return 0;
		}
	}

	/**************************
	 * @brief Perform one effective splice from connection. Is concurrency safe and won't be called on closed
	 * connection.
	 *
	 * @param fd Destination file descriptor.
	 * @param size Number of bytes to be spliced. It must be greater than 0.
	 *
	 * @return Number of spliced bytes; returns 0 on any splice error.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Splice(const int32_t fd, const uint64_t size)
	{
		contract_assert(size != 0);

		Lock::Atomic::Guard _{ m_recvLock };
		if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
			return 0;
		}

		const auto result{ splice(m_connection, nullptr, fd, nullptr, size, SPLICE_F_MOVE) };

		if (result > 0) [[likely]] {
			return static_cast<uint64_t>(result);
		}

		if (result == 0) {
			LOG_WARNING_NEW("Splice returned 0 while dropping {} byte(s), connection id {}", size, m_id);
			return 0;
		}

		LOG_ERROR_NEW("Failed to splice data. Error №{}: {}, connection id {}", errno, std::strerror(errno), m_id);
		return 0;
	}

	template <typename T>
	concept RecvFunction = std::is_convertible_v<T, recv_func_t>;

	/**************************
	 * @brief Override the default recv function. Is concurrency safe and won't be called on closed connection.
	 *
	 * @tparam T Recv function.
	 *
	 * @param f New recv function.
	 *
	 * @todo Add unit test.
	 */
	template <RecvFunction T> FORCE_INLINE void SetRecv(T&& f) noexcept
	{
		Lock::Atomic::Guard _{ m_recvLock };
		if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
			return;
		}

		m_recvFunc = std::forward<T>(f);
	}

	template <typename T>
	concept SendFunction = std::is_convertible_v<T, send_func_t>;

	/**************************
	 * @brief Override the default send function. Is concurrency safe and won't be called on closed connection.
	 *
	 * @tparam T Send function.
	 *
	 * @param f New send function.
	 *
	 * @todo Add unit test.
	 */
	template <SendFunction T> FORCE_INLINE void SetSend(T&& f) noexcept
	{
		Lock::Atomic::Guard _{ m_sendLock };
		if (!m_isUsable.load(std::memory_order_relaxed)) [[unlikely]] {
			return;
		}

		m_sendFunc = std::forward<T>(f);
	}

	/**************************
	 * @return Id of connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetId() const noexcept { return m_id; }

	/**************************
	 * @brief Shutdown and close connection. Is not concurrency safe and won't be called on closed connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Close()
	{
		if (m_isClosed) [[unlikely]] {
			return;
		}

		m_isUsable.store(false, std::memory_order_release);
		m_isClosed = true;
		if (shutdown(m_connection, SHUT_RDWR) == -1) [[unlikely]] {
			if (errno == ENOTCONN) {
				LOG_DEBUG_NEW("Connection {} is already closed", m_id);
			}
			else {
				LOG_WARNING_NEW("Connection {} shutdown is failed. Error №{}: {}", m_id, errno, std::strerror(errno));
			}
		}

		Lock::Atomic::Guard recvGuard{ m_recvLock };
		Lock::Atomic::Guard sendGuard{ m_sendLock };

		if (close(m_connection) != -1) [[likely]] {
			LOG_DEBUG_NEW("Connection {} is closed", m_id);
			return;
		}

		LOG_WARNING_NEW("Connection {} closing is failed. Error №{}: {}", m_id, errno, std::strerror(errno));
	}
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

} // namespace MSAPI

#endif // MSAPI_CONNECTION_INL