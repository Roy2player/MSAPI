/**************************
 * @file        recvBuffer.inl
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
#include "connection.inl"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Recv buffer manager for particular connection.
 *
 * @note Internal buffer has dynamic size which starts from minimal required to at least start message protocol
 * identification and limited by const capacity.
 *
 * @concurrency No.
 */
class RecvBuffer {
public:
	/**************************
	 * @brief Information about performed recv operation. If buffer size is greater that 0, then data was read
	 * successfully, otherwise connection is closed or error happened. If data was dropped on error, the exact amount is
	 * set.
	 *
	 * The idea of providing extended info is to allow caller decide whatever interrupt problematic connection or try to
	 * read next to dropped data.
	 *
	 * @concurrency No.
	 */
	class Result {
	private:
		const uint64_t m_bufferSize;
		const uint64_t m_droppedSize;

	public:
		/**************************
		 * @brief Create recv result object.
		 *
		 * @param bufferSize Final size of the buffer.
		 * @param droppedSize Flag is data was dropped.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE Result(uint64_t bufferSize, uint64_t droppedSize) noexcept;

		Result(const Result&) = delete;
		FORCE_INLINE Result(Result&&) = default;
		Result& operator=(const Result&) = delete;
		Result& operator=(Result&&) = delete;

		/**************************
		 * @return New buffer size (include peeked) on success, 0 otherwise.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetBufferSize() const noexcept;

		/**************************
		 * @return Dropped size.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetDroppedSize() const noexcept;
	};

private:
	const std::shared_ptr<Connection::Data> m_connectionData;
	const uint64_t m_capacityLimit;
	AutoClearPtr<uint8_t> m_buffer;
	uint64_t m_size{};
	uint64_t m_peekedSize{};
	uint64_t m_toProcessSize;
	uint64_t m_capacity;
	uint64_t m_dataType{}; // No project-wide enum is possible

public:
	/**************************
	 * @brief Construct a new Recv Buffer object, guarantee that to process size is not greater than limit.
	 *
	 * @attention On construction, the internal buffer can be nullptr due to malloc error.
	 *
	 * @param connectionData Connection data structure.
	 * @param capacityLimit Buffer capacity limit.
	 * @param toProcessSize Minimum required size to be read on socket to allow execution unit move forward.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE RecvBuffer(std::shared_ptr<Connection::Data> connectionData /* by value as moved */,
		uint64_t capacityLimit, uint64_t toProcessSize);

	RecvBuffer(const RecvBuffer&) = delete;
	RecvBuffer(RecvBuffer&&) = delete;
	RecvBuffer& operator=(const RecvBuffer&) = delete;
	RecvBuffer& operator=(RecvBuffer&&) = delete;

	/**************************
	 * @brief Check and set minimum required size to be read on socket to allow execution unit move forward. Cannot be
	 * less than 1.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @param toProcessSize New value.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE void SetToProcessSize(uint64_t toProcessSize);

	/**************************
	 * @brief Set the data type. Is used to identify if buffer contains specific protocol.
	 * typeid(message_type).hash_code() can be used.
	 *
	 * @param dataType Data type.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE void SetDataType(uint64_t dataType) noexcept;

	/**************************
	 * @return Minimum required size to be read on socket to allow execution unit move forward.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetToProcessSize() const noexcept;

	/**************************
	 * @return Const buffer.
	 *
	 * @attention Can be invalidated on capacity change.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetBuffer() const noexcept;

	/**************************
	 * @return Const pointer to buffer.
	 *
	 * @attention Can be invalidated on capacity change.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] const uint8_t* GetData() const noexcept;

	/**************************
	 * @return Size of data stored in buffer, include peeked.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetBufferSize() const noexcept;

	/**************************
	 * @return Connection data.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] const std::shared_ptr<Connection::Data>& GetConnectionData() noexcept;

	/**************************
	 * @return Connection id.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetConnectionId() const noexcept;

	/**************************
	 * @return Data type.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetDataType() const noexcept;

	/**************************
	 * @brief Renew existed buffer by blocking read minimum required size from socket to allow execution unit move
	 * forward.
	 *
	 * @attention On construction, the internal buffer can be nullptr due to malloc error.
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @return Result of the operation.
	 *
	 * @todo Add tests coverage.
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
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] bool RecvAdditional(uint64_t requiredSize);

	/**************************
	 * @brief Extend existed buffer by blocking peek additional data from socket. Blocks till any size peek or error.
	 *
	 * @attention Each peeking overwrites previous peek.
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @param requiredSize Required buffer size.
	 *
	 * @return Buffer size (include peeked) on success, zero otherwise.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t RecvAdditionalPeek(uint64_t requiredSize);

	/**************************
	 * @brief Potentially blocking recv trunc data from connection without overwriting previously taken data and remain
	 * size/peeked size.
	 *
	 * @attention Can invalidate pointer to buffer.
	 *
	 * @note Has thread local buffer with 1024 bytes capacity.
	 * - Local buffer is used if trunc size can fit into thread local buffer.
	 * - If trunc size does not fit, try to extend existed buffer and use it on success.
	 * - If attempt to extend buffer is failed, the partial trunc is performed with local buffer capacity chunks.
	 *
	 * @param truncSize Number of bytes to trunc.
	 *
	 * @return Number of truncated bytes.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t RecvTrunc(uint64_t truncSize);

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
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] bool CheckCapacity(uint64_t requiredSize);

	static constexpr inline bool regular{ true };
	static constexpr inline bool irregular{ false };

	/**************************
	 * @brief Extend existed buffer by recv particular number of bytes from socket in buffer and place after existed
	 * data. Overwrites peeked bytes if any and reduces peeked size.
	 *
	 * @attention For non-blocking recv function return only after successful read or error.
	 * @attention Each peeking overwrites previous peek.
	 * @attention Can invalidate pointer to buffer.
	 * @attention If required size is greater than capacity trunc is performed.
	 *
	 * @tparam Flags Recv flags.
	 * @tparam IsRegular Flag if recv is regular. If yes - reset buffer and peeked sizes, else - required size is
	 * checked against buffer capacity.
	 *
	 * @param requiredSize Required size of buffer.
	 *
	 * @pre requiredSize > 0.
	 *
	 * @return Result of the operation.
	 *
	 * @todo Add tests coverage.
	 */
	template <int32_t Flags, bool IsRegular> FORCE_INLINE [[nodiscard]] Result RecvImpl(uint64_t requiredSize);
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
RecvBuffer::Result
---------------------------------------------------------------------------------*/

FORCE_INLINE RecvBuffer::Result::Result(const uint64_t bufferSize, const uint64_t droppedSize) noexcept
	: m_bufferSize{ bufferSize }
	, m_droppedSize{ droppedSize }
{
}

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::Result::GetBufferSize() const noexcept { return m_bufferSize; }

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::Result::GetDroppedSize() const noexcept { return m_droppedSize; }

/*---------------------------------------------------------------------------------
RecvBuffer
---------------------------------------------------------------------------------*/

FORCE_INLINE RecvBuffer::RecvBuffer(
	std::shared_ptr<Connection::Data> connectionData, const uint64_t capacityLimit, const uint64_t toProcessSize)
	: m_connectionData{ std::move(connectionData) }
	, m_capacityLimit{ capacityLimit != 0 ? capacityLimit : 0 }
{
	if (toProcessSize > capacityLimit || toProcessSize == 0) [[unlikely]] {
		LOG_WARNING_NEW("Initial to process size {} > capacity limit {} or equal to zero, limit is used instead",
			toProcessSize, capacityLimit);
		m_toProcessSize = capacityLimit;
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
		m_connectionData->GetConnectionId());
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

FORCE_INLINE [[nodiscard]] const std::shared_ptr<Connection::Data>& RecvBuffer::GetConnectionData() noexcept
{
	return m_connectionData;
}

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::GetConnectionId() const noexcept
{
	return m_connectionData->GetConnectionId();
}

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::GetDataType() const noexcept { return m_dataType; }

FORCE_INLINE [[nodiscard]] RecvBuffer::Result RecvBuffer::Recv() { return RecvImpl<0, regular>(m_toProcessSize); }

FORCE_INLINE [[nodiscard]] bool RecvBuffer::RecvAdditional(const uint64_t requiredSize)
{
	return RecvImpl<0, irregular>(requiredSize).GetBufferSize() != 0;
}

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::RecvAdditionalPeek(const uint64_t requiredSize)
{
	return RecvImpl<MSG_PEEK, irregular>(requiredSize).GetBufferSize();
}

FORCE_INLINE [[nodiscard]] bool RecvBuffer::CheckCapacity(const uint64_t requiredSize)
{
	if (requiredSize <= m_capacity) [[likely]] {
		return true;
	}

	if (requiredSize > m_capacityLimit) [[unlikely]] {
		LOG_ERROR_NEW("Required size of recv buffer {} > limit {}, connection id: {}", requiredSize, m_capacityLimit,
			m_connectionData->GetConnectionId());
		return false;
	}

	if (m_buffer.Realloc(requiredSize) == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Failed to reallocate recv buffer to {} bytes, connection id: {}", requiredSize,
			m_connectionData->GetConnectionId());
		return false;
	}

	m_capacity = requiredSize;
	LOG_PROTOCOL_NEW("Reallocate recv buffer to {} bytes successfully, connection id: {}", m_capacity,
		m_connectionData->GetConnectionId());
	return true;
}

template <int32_t Flags, bool IsRegular>
FORCE_INLINE [[nodiscard]] RecvBuffer::Result RecvBuffer::RecvImpl(const uint64_t requiredSize)
{
	auto rest{ requiredSize };

	if constexpr (IsRegular) {
		m_size = 0;
		m_peekedSize = 0;
	}
	else {
		if (requiredSize <= m_size) [[unlikely]] {
			LOG_WARNING_NEW(
				"Attempt to recv invalid amount of data. Required size {} <= buffer size {}, connection id: {}",
				requiredSize, m_size, m_connectionData->GetConnectionId());
			return { 0, 0 };
		}

		rest -= m_size;
		if (!CheckCapacity(requiredSize)) [[unlikely]] {
			return { 0, RecvTrunc(rest) };
		}
	}

	if constexpr (Flags & MSG_PEEK) {
		m_peekedSize = 0;
	}

	do {
		const auto result{ m_connectionData->GetConnection().Recv(m_buffer.Get() + m_size, rest, Flags) };

		if (result == 0) [[unlikely]] {
			// Not sure if it is required
			// pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
			return { 0, 0 };
		}

		if constexpr (Flags & MSG_PEEK) {
			LOG_PROTOCOL_NEW("Recv look up {} out of {} in buffer with offset: {}, connection id: {}", result, rest,
				m_size, m_connectionData->GetConnectionId());
			m_peekedSize += result;
			break;
		}

		LOG_PROTOCOL_NEW("Recv {} out of {} in buffer with offset: {}, connection id: {}", result, rest, m_size,
			m_connectionData->GetConnectionId());
		m_size += result;
		rest -= result;
	} while (rest != 0);

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

	return { m_size + m_peekedSize, 0 };
}

FORCE_INLINE [[nodiscard]] uint64_t RecvBuffer::RecvTrunc(const uint64_t truncSize)
{
	if (truncSize == 0) [[unlikely]] {
		LOG_WARNING_NEW("Attempt to trunc zeo bytes, connection id: {}", m_connectionData->GetConnectionId());
		return 0;
	}

	static constexpr uint64_t JUNK_BUFFER_SIZE{ 1024 };
	static thread_local std::array<uint8_t, JUNK_BUFFER_SIZE> t_junkStorage;

	bool partialDrop [[indeterminate]];
	uint64_t dropPortion [[indeterminate]];
	uint8_t* truncBuffer [[indeterminate]];

	if (truncSize <= JUNK_BUFFER_SIZE) {
		partialDrop = false;
		dropPortion = truncSize;
		truncBuffer = t_junkStorage.data();
	}
	else if (!CheckCapacity(truncSize + m_size)) [[unlikely]] {
		partialDrop = true;
		dropPortion = JUNK_BUFFER_SIZE;
		truncBuffer = t_junkStorage.data();

		LOG_PROTOCOL_NEW(
			"Trunc data by portion: {}, connection id: {}", JUNK_BUFFER_SIZE, m_connectionData->GetConnectionId());
	}
	else {
		partialDrop = false;
		dropPortion = truncSize;
		truncBuffer = m_buffer.Get() + m_size;
	}

	auto rest{ truncSize };
	while (true) {
		const auto result{ m_connectionData->GetConnection().Recv(truncBuffer, dropPortion, MSG_TRUNC) };

		if (result == 0) [[unlikely]] {
			// Not sure if it is required
			// pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
			return truncSize - rest;
		}

		LOG_PROTOCOL_NEW("Trunc {} out of {}, connection id: {}", result, rest, m_connectionData->GetConnectionId());
		rest -= result;

		if (rest == 0) {
			break;
		}

		if (partialDrop && dropPortion < rest) {
			dropPortion = rest;
		}
	}

	return truncSize;
}

} // namespace MSAPI

#endif // MSAPI_RECV_BUFFER_INL