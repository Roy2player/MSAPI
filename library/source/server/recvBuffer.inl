/**************************
 * @file        recvBuffer.inl
 * @version     6.0
 * @date        2025-05-01
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

#ifndef MSAPI_RECV_BUFFER_INL
#define MSAPI_RECV_BUFFER_INL

#include "../help/autoClearPtr.inl"
#include "../help/io.inl"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Abstraction to access the data from socket for particular connection.
 *
 * @todo Probably this structure can be merged into ConnectionInfo structure. But that structure works only with
 * outcome connections, but not with income. Maybe when secure protocol will be implemented this situation will be
 * changed.
 */
class RecvBuffer {
public:
	/**************************
	 * @brief Information about performed recv operation. If buffer size is greater that 0, then data was read
	 * successfully, otherwise error happened. If data was dropped from socket successfully on error, the specific flag
	 * is set.
	 *
	 * The idea of providing extended info is to allow caller decide whatever interrupt problematic connection or try to
	 * read next to dropped data.
	 */
	struct Result {
		const uint64_t bufferSize;
		const bool isDataDropped;

		/**************************
		 * @brief Create recv result object.
		 *
		 * @param bufferSize Final size of the buffer.
		 * @param isDataDropped Flag is data was dropped.
		 *
		 * @test Add unit test.
		 */
		FORCE_INLINE Result(const uint64_t bufferSize, const bool isDataDropped) noexcept
			: bufferSize{ bufferSize }
			, isDataDropped{ isDataDropped }
		{
		}

		Result(const Result& other) = delete;
		Result(Result&& other) = default;
		Result& operator=(const Result& other) = delete;
		Result& operator=(Result&& other) = delete;
	};

private:
	const uint64_t* m_capacityLimit;
	AutoClearPtr<uint8_t> m_buffer;
	uint64_t m_size{};
	uint64_t m_peekedSize{};
	uint64_t m_toProcessSize;
	uint64_t m_capacity;
	uint64_t m_dataType{};
	const int32_t m_connection;
	const int32_t m_connectionId;

public:
	/**************************
	 * @brief Construct a new Recv Buffer object, guarantee that to process size is not greater than limit.
	 *
	 * @param capacityLimit Pointer to capacity limit.
	 * @param toProcessSize Minimum required size to be read on socket to allow execution unit move forward.
	 * @param connection Connection descriptor.
	 * @param id Connection id.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE RecvBuffer(const uint64_t* capacityLimit, uint64_t toProcessSize, int32_t connection, int32_t id);

	RecvBuffer(const RecvBuffer& other) = delete;
	RecvBuffer(RecvBuffer&& other) = delete;
	RecvBuffer& operator=(const RecvBuffer& other) = delete;
	RecvBuffer& operator=(RecvBuffer&& other) = delete;

	/**************************
	 * @brief Check and set minimum required size to be read on socket to allow execution unit move forward. Cannot be
	 * less than 1.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @param toProcessSize New value.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE void SetToProcessSize(uint64_t toProcessSize);

	/**************************
	 * @brief Set the data type. Is used to identify if buffer contains specific protocol.
	 * typeid(message_type).hash_code() can be used.
	 *
	 * @param dataType Data type.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE void SetDataType(uint64_t dataType) noexcept;

	/**************************
	 * @return Minimum required size to be read on socket to allow execution unit move forward.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetToProcessSize() const noexcept;

	/**************************
	 * @return Const buffer.
	 *
	 * @attention Can be invalidated on capacity change.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetBuffer() const noexcept;

	/**************************
	 * @return Const pointer to buffer.
	 *
	 * @attention Can be invalidated on capacity change.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] const uint8_t* GetData() const noexcept;

	/**************************
	 * @return Size of data stored in buffer, include peeked.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetBufferSize() const noexcept;

	/**************************
	 * @return Connection.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] int32_t GetConnection() const noexcept;

	/**************************
	 * @return Connection id.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] int32_t GetConnectionId() const noexcept;

	/**************************
	 * @return Data type.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetDataType() const noexcept;

	/**************************
	 * @brief Renew existed buffer by blocking read minimum required size from socket to allow execution unit move
	 * forward.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @return Buffer size (include peeked) on success, zero with flag if data was dropped otherwise.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] Result Recv();

	/**************************
	 * @brief Extend existed buffer by blocking read additional data from socket. Blocks till buffer size is not equal
	 * to required size or any error. Overwrites peeked bytes if any and reduces peeked size.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @param requiredSize Required buffer size.
	 *
	 * @return True if data was read successfully, false otherwise.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool RecvAdditional(uint64_t requiredSize);

	/**************************
	 * @brief Extend existed buffer by blocking peek additional data from socket. Blocks till any size peek or error.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @param requiredSize Required buffer size.
	 *
	 * @return Buffer size (include peeked) on success, zero otherwise.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t RecvAdditionalPeek(uint64_t requiredSize);

private:
	/**************************
	 * @brief Check if buffer capacity is enough and reallocate buffer in case if required size is greater. If
	 * reallocation fails or size not within limit, then internal state is not changed.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @param requiredSize Required size of buffer.
	 *
	 * @return True on success, false otherwise.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool CheckCapacity(uint64_t requiredSize);

	static constexpr inline bool irregular{ true };
	static constexpr inline bool regular{ false };

	/**************************
	 * @brief Extend existed buffer by recv particular number of bytes from socket in buffer and place after existed
	 * data. Overwrites peeked bytes if any and reduces peeked size.
	 *
	 * @attention For non-blocking recv function return only after successful read or error.
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @tparam Flags Recv flags.
	 * @tparam IsRegular Flag if recv is regular. If yes - reset buffer and peeked sizes, else - required size is
	 * checked against buffer capacity.
	 *
	 * @param requiredSize Required size of buffer.
	 *
	 * @return Buffer size (include peeked) on success, zero with flag if data was dropped otherwise.
	 *
	 * @test Add unit test.
	 */
	template <int32_t Flags, bool IsIrregular> FORCE_INLINE [[nodiscard]] Result RecvImpl(uint64_t requiredSize);

	/**************************
	 * @brief Attempt to splice data from socket to /dev/null.
	 *
	 * @param toDrop Number of bytes to drop.
	 *
	 * @return True of success, false otherwise.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool Drop(uint64_t toDrop) const;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE RecvBuffer::RecvBuffer(
	const uint64_t* const capacityLimit, const uint64_t toProcessSize, const int32_t connection, const int32_t id)
	: m_capacityLimit{ capacityLimit }
	, m_connection{ connection }
	, m_connectionId{ id }
{
	if (toProcessSize > *capacityLimit) [[unlikely]] {
		LOG_WARNING_NEW("Initial to process size is greater that capacity limit {} > {}, limit is used instead",
			toProcessSize, *capacityLimit);
		m_toProcessSize = *m_capacityLimit;
	}
	else {
		m_toProcessSize = toProcessSize;
	}

	m_buffer = AutoClearPtr<uint8_t>{ m_toProcessSize };
	if (m_buffer.Get() == nullptr) [[unlikely]] {
		m_capacity = 0;
		return;
	}

	m_capacity = m_toProcessSize;
}

FORCE_INLINE void RecvBuffer::SetToProcessSize(const uint64_t toProcessSize)
{
	if (toProcessSize < 1) [[unlikely]] {
		LOG_WARNING("To process size cannot be less than 1 byte");
		return;
	}

	if (!CheckCapacity(toProcessSize)) [[unlikely]] {
		return;
	}

	LOG_PROTOCOL_NEW("Change to process size from {} to {} bytes, connection id: {}", m_toProcessSize, toProcessSize,
		m_connectionId);
	m_toProcessSize = toProcessSize;
}

FORCE_INLINE void RecvBuffer::SetDataType(const uint64_t dataType) noexcept { m_dataType = dataType; }

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::GetToProcessSize() const noexcept { return m_toProcessSize; }

FORCE_INLINE [[nodiscard]] std::span<const uint8_t> RecvBuffer::GetBuffer() const noexcept
{
	return { m_buffer.Get(), m_size + m_peekedSize };
}

FORCE_INLINE [[nodiscard]] const uint8_t* RecvBuffer::GetData() const noexcept { return m_buffer.Get(); }

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::GetBufferSize() const noexcept { return m_size + m_peekedSize; }

FORCE_INLINE [[nodiscard]] int32_t RecvBuffer::GetConnection() const noexcept { return m_connection; }

FORCE_INLINE [[nodiscard]] int32_t RecvBuffer::GetConnectionId() const noexcept { return m_connectionId; }

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::GetDataType() const noexcept { return m_dataType; }

FORCE_INLINE [[nodiscard]] RecvBuffer::Result RecvBuffer::Recv() { return RecvImpl<0, regular>(m_toProcessSize); }

FORCE_INLINE [[nodiscard]] bool RecvBuffer::RecvAdditional(const uint64_t requiredSize)
{
	return RecvImpl<0, irregular>(requiredSize).bufferSize != 0;
}

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::RecvAdditionalPeek(const uint64_t requiredSize)
{
	return RecvImpl<MSG_PEEK, irregular>(requiredSize).bufferSize;
}

FORCE_INLINE [[nodiscard]] bool RecvBuffer::CheckCapacity(const uint64_t requiredSize)
{
	if (requiredSize <= m_capacity) [[likely]] {
		return true;
	}

	if (requiredSize > *m_capacityLimit) [[unlikely]] {
		LOG_ERROR_NEW("Required size of recv buffer ({}) is greater than limit ({}), connection id: {}", requiredSize,
			*m_capacityLimit, m_connectionId);
		return false;
	}

	if (m_buffer.Realloc(requiredSize) == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Failed to reallocate recv buffer to {} bytes, connection id: {}", requiredSize, m_connectionId);
		return false;
	}

	m_capacity = requiredSize;
	LOG_PROTOCOL_NEW("Reallocate recv buffer to {} bytes successfully, connection id: {}", m_capacity, m_connectionId);
	return true;
}

template <int32_t Flags, bool IsRegular>
FORCE_INLINE [[nodiscard]] RecvBuffer::Result RecvBuffer::RecvImpl(const uint64_t requiredSize)
{
	uint64_t rest{ requiredSize };

	if constexpr (IsRegular) {
		m_size = 0;
		m_peekedSize = 0;
	}
	else {
		if (requiredSize <= m_size) [[unlikely]] {
			LOG_WARNING_NEW(
				"Attempt to recv invalid amount of data. Required size {} <= buffer size {}, connection id: {}",
				requiredSize, m_size, m_connection);
			return { 0, false };
		}

		rest -= m_size;
		if (!CheckCapacity(requiredSize)) [[unlikely]] {
			return { 0, Drop(rest) };
		}
	}

	while (true) {
		const auto result{ recv(m_connection, m_buffer.Get() + m_size, rest, Flags) };

		if (result == 0) [[unlikely]] {
			// Not sure if it is required
			// pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
			LOG_INFO_NEW("Socket is closed by other side, connection id {}", m_connectionId);
			return { 0, false };
		}

		if (result == -1) [[unlikely]] {
			if constexpr (Flags & MSG_PEEK) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					LOG_PROTOCOL_NEW(
						"Non-blocking recv returned EAGAIN or EWOULDBLOC, connection id {}", m_connectionId);
					continue;
				}
			}

			if (errno == 104) {
				LOG_PROTOCOL_NEW("Recv returned unrecoverable error №104: Connection reset by peer, connection id {}",
					m_connectionId);
				return { 0, false };
			}

			if (errno == 9) {
				LOG_PROTOCOL_NEW(
					"Recv returned unrecoverable error №9: Bad file descriptor, connection id {}", m_connectionId);
				return { 0, false };
			}

			LOG_ERROR_NEW("Recv returned unrecoverable error №{}: {}, connection id {}", errno, std::strerror(errno),
				m_connectionId);
			return { 0, false };
		}

		if constexpr (Flags & MSG_PEEK) {
			LOG_PROTOCOL_NEW("Recv look up {} out of {} in buffer with offset {}, connection id {}", result, rest,
				m_size, m_connectionId);
			m_peekedSize += static_cast<uint64_t>(result);
			break;
		}

		LOG_PROTOCOL_NEW(
			"Recv {} out of {} in buffer with offset {}, connection id {}", result, rest, m_size, m_connectionId);
		m_size += static_cast<uint64_t>(result);
		rest -= static_cast<uint64_t>(result);

		if (rest != 0) [[unlikely]] {
			continue;
		}

		break;
	}

	// Not sure if it is required
	// pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);

	if constexpr ((Flags & MSG_PEEK) == 0) {
		if (requiredSize >= m_peekedSize) {
			m_peekedSize = 0;
		}
		else {
			m_peekedSize -= requiredSize;
		}
	}

	return { m_size + m_peekedSize, false };
}

FORCE_INLINE [[nodiscard]] bool RecvBuffer::Drop(const uint64_t toDrop) const
{
	IO::FileDescriptor::ExitGuard guardFd{ "/dev/null", O_WRONLY, 644 };
	if (guardFd.value == -1) [[unlikely]] {
		LOG_ERROR("Failed to open /dev/null");
		return false;
	}

	uint64_t rest{ toDrop };
	while (true) {
		const auto result{ splice(m_connection, nullptr, guardFd.value, nullptr, rest, SPLICE_F_MOVE) };
		if (result == -1) [[unlikely]] {
			LOG_ERROR_NEW("Failed to splice data to /dev/null error №{}: {}, connection id {}", errno,
				std::strerror(errno), m_connectionId);
			return false;
		}

		rest -= static_cast<uint64_t>(result);
		if (rest != 0) [[unlikely]] {
			LOG_PROTOCOL_NEW("Partially spliced {} out of {} bytes from socket to /dev/null, connection id {}", result,
				toDrop, m_connectionId);
			continue;
		}

		break;
	}

	LOG_PROTOCOL_NEW("Spliced {} bytes from socket to /dev/null, connection id {}", toDrop, m_connectionId);
	return true;
}

} // namespace MSAPI

#endif // MSAPI_RECV_BUFFER_INL