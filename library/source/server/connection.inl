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
 * @brief Wrapper under pure connection with allowance to guarantee concurency safe and ability to dynamically override its recv and send behaviour in zero cost. Default behaviour methods do not perform any action under connection on failure, because the owner and the main caller suppose to handle that.
 */
class Connection {
public:
	using func_t = std::function<uint64_t(int32_t, void*, uint64_t, int32_t)>;

private:
	const uint64_t m_id{ m_counter.fetch_add(1, std::memory_order_relaxed) };
	const int32_t m_connection;
	func_t m_recvFunc{
		[](const int32_t fd, void* const buffer, const uint64_t size, const int32_t flags) noexcept {
			return recv(fd, buffer, size, flags);
		}
	};
	func_t m_sendFunc{
		[](const int32_t fd, void* const buffer, const uint64_t size, const int32_t flags) noexcept {
			return send(fd, buffer, size, flags);
		}
	};
	Lock::Atomic m_recvLock;
	Lock::Atomic m_sendLock;
	std::atomic_flag m_isOpened{ true };

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
	{}

	Connection(const Connection& other) = delete;
	Connection(Connection&& other) = delete;
	Connection& operator=(const Connection& other) = delete;
	Connection& operator=(Connection&& other) = delete;

	/**************************
	 * @brief Perform one effective recv from connection. Is concurency safe and won't be called on closed connection.
	 *
	 * @param buffer Pointer to buffer. It must have enough space.
	 * @param size Number of bytes to be read.
	 * @param flags Flags for recv.
	 *
	 * @return Number of read bytes and 0 on connection closure.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Recv(void* const buffer, const uint64_t size, const int32_t flags)
	{
		Lock::Atomic::Guard _{ m_recvLock };
		if (!m_isOpened.test()) [[unlikely]] {
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
				m_isOpened.clear(std::memory_order_release);
				LOG_INFO_NEW("Socket is closed by other side, connection id {}", m_id);
				return 0;
			}

			if (flags & MSG_PEEK) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					LOG_PROTOCOL_NEW("Non-blocking recv returned EAGAIN or EWOULDBLOC, connection id {}", m_id);
					continue;
				}
			}

			m_isOpened.clear(std::memory_order_release);
			LOG_ERROR_NEW("Recv returned unrecoverable error №{}: {}, connection id {}", errno, std::strerror(errno), m_id);
			return 0;
		}
	}

	/**************************
	 * @brief Perform one effective send to connection. Is concurency safe and won't be called on closed connection.
	 *
	 * @attention In case of connection closing initiated by other side the send can be called on just closed, but not marked as is closed connection. That is the incredible rare, but still possible case. There is no connection access pattern to privent that behaviour on application side.
	 * 
	 * @param buffer Pointer to buffer. It must have enough data.
	 * @param size Number of bytes to be send.
	 * @param flags Flags for send.
	 *
	 * @return Number of send bytes and 0 on any error. 
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t Send(void* const buffer, const uint64_t size, const int32_t flags)
	{
		if (!m_isOpened.test()) [[unlikely]] {
			return 0;
		}

		{
			Lock::Atomic::Guard _{ m_sendLock };
			const auto result{ m_sendFunc(m_connection, buffer, size, flags) };
		}

		if (result > 0) [[likely]] {
			return static_cast<uint64_t>(result);
		}

		LOG_ERROR_NEW("Send failed with error №{}: {}, connection id {}", errno, std::strerror(errno), m_id);
		return 0;
	}

	/**************************
	 * @brief Perform one effective splice from connection. Is concurency safe and won't be called on closed connection.
	 *
	 * @param fd Destination file descriptor, should not be a pipe.
	 * @param size Number of bytes to be spliced.
	 *
	 * @return Number of spliced bytes and 0 on any error.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] unit64_t Splice(const int32_t fd, const uint64_t size)
	{
		Lock::Atomic::Guard _{ m_recvLock };
		if (!m_isOpened.test()) [[unlikely]] {
			return 0;
		}

		const auto result{ splice(m_connection, nullptr, fd, nullptr, size, SPLICE_F_MOVE) };

		if (result > 0) [[likely]] {
			return static_cast<uint64_t>(result);
		}

		if (result == 0) {
			m_isOpened.clear(std::memory_order_release)
			LOG_WARNING_NEW("Splice returned 0 while dropping {} byte(s), connection id {}", size, m_id);
			return 0;
		}

		LOG_ERROR_NEW("Failed to splice data. Error №{}: {}, connection id {}", errno,
			std::strerror(errno), m_id);
		return 0;
	}

	template <typename T>
	concept Function = std::is_convertible_t<T, func_t>;

	/**************************
	 * @brief Override the default recv function. Is concurency safe and won't be called on closed connection.
	 * 
	 * @tparam T Recv function.
	 *
	 * @param f New recv function.
	 *
	 * @todo Add unit test.
	 */
	template <Function T>
	FORCE_INLINE void SetRecv(T&& f) noexcept {
		Lock::Atomic::Guard _{ m_recvLock };
		if (!m_isOpened.test()) [[unlikely]] {
			return;
		}
	
		m_recvFunc = std::forward<T>(f);
	}

	/**************************
	 * @brief Override the default send function. Is concurency safe and won't be called on closed connection.
	 * 
	 * @tparam T Send function.
	 *
	 * @param f New send function.
	 *
	 * @todo Add unit test.
	 */
	template <Function T>
	FORCE_INLINE void SetSend(T&& f) noexcept {
		Lock::Atomic::Guard _{ m_sendLock };
		if (!m_isOpened.test()) [[unlikely]] {
			return;
		}

		m_sendFunc = std::forward<T>(f);
	}

	/**************************
	 * @return Id fo connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetId() noexcept const
	{
		return m_id;
	}

	/**************************
	 * @brief Shutdown and close connection.
	 */
	FORCE_INLINE void Close()
	{
		if (shutdown(m_connection, SHUT_RDWR) == -1) [[unlikely]] {
			if (errno == ENOTCONN) {
				LOG_DEBUG_NEW("Connection {} is already closed", m_id);
				return;
			}

			LOG_ERROR_NEW("Connection {} shutdown is failed. Error №{}: {}", m_id, errno, std::strerror(errno));
			return;
		}

		if (close(connection) != -1) [[likely]] {
			LOF_DEBUG_NEW("Connection {} is closed", m_id);
			return;
		}

		LOG_ERROR_NEW("Connection {} closing is failed. Error №{}: {}", m_id, errno, std::strerror(errno));
	}
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

} // namespace MSAPI

#endif // MSAPI_CONNECTION_INL