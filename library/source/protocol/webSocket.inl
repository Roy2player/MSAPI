/**************************
 * @file        helper.inl
 * @version     6.0
 * @date        2026-02-16
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

#ifndef MSAPI_WEBSOCKET_PROTOCOL_INL
#define MSAPI_WEBSOCKET_PROTOCOL_INL

#include "../server/server.h"
#include <unordered_set>

namespace MSAPI {

class Application;

namespace IntegrationTest {

namespace WebSocketProtocol {

class Observer;

} // namespace WebSocketProtocol

} // namespace IntegrationTest

namespace WebSocketProtocol {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Object for containing data of WebSocket message.
 */
class Data {
public:
	enum class Opcode : int8_t { Continuation = 0x0, Text = 0x1, Binary = 0x2, Close = 0x8, Ping = 0x9, Pong = 0xA };

	enum class CloseStatusCode : int16_t {
		NormalClosure = 1000, // Connection successfully completed purpose
		GoingAway = 1001, // Endpoint is shutting down or navigating away
		ProtocolError = 1002, // Protocol violation detected
		UnsupportedData = 1003, // Received unsupported data type
		Reserved = 1004, // Reserved
		NoStatusReceived = 1005, // No status code present. When no payload is sent with close frame. Must not be used.
		AbnormalClosure = 1006, // Connection closed without Close frame. Must not be used.
		InvalidFramePayloadData = 1007, // Non-UTF-8 data in text frame. Must not be used.
		PolicyViolation = 1008, // Application-level policy failure
		MessageTooBig = 1009, // Payload exceeds allowed size
		MandatoryExtension = 1010, // The client expected the server to negotiate one or more WebSocket extensions
								   // during the handshake, but the server did not. Used only by client.
		InternalError = 1011, // Unexpected server-side condition
		ServiceRestart = 1012, // Server restarting
		TryAgainLater = 1013, // Temporary overload
		BadGateway = 1014, // Upstream server failure
		TLSHandshakeFailure = 1015, // TLS failure during handshake. Must not be used.
	};

	static constexpr inline int8_t REQUIRED_HEADER_SIZE{ 2 };

private:
	/*
		RFC 6455 WebSocket frame format

		1 bit	FIN	       	1 = final fragment of a message; 0 = more fragments follow
		1 bit	RSV1	   	Reserved (used for extensions like permessage-deflate)
		1 bit	RSV2	   	Reserved
		1 bit	RSV3	   	Reserved
		4 bits	OPCODE	    Frame type (text, binary, close, ping, pong, continuation)
		1 bit	MASK	   	1 = payload is masked (mandatory for client → server)
		7 bits	DATA SIZE	Payload size indicator (see extended length rules below)
		16 bits optional	Extended Payload Length	Unsigned integer, present if Payload Length = 126
		64 bits optional	Extended Payload Length	Unsigned integer, present if Payload Length = 127
		32 bits optional	Masking Key	4-byte array, present if MASK = 1
	*/
	std::vector<uint8_t> m_buffer = std::vector<uint8_t>(REQUIRED_HEADER_SIZE, 0);
	int8_t m_headerSize{ REQUIRED_HEADER_SIZE };

public:
	/**************************
	 * @brief Construct a new Data object, default constructor.
	 */
	FORCE_INLINE Data() noexcept = default;

	FORCE_INLINE Data(const std::span<const uint8_t> payload, const Opcode opcode, uint32_t mask = 0,
		const bool isFinal = true, const bool rsv1 = false, const bool rsv2 = false, const bool rsv3 = false) noexcept
	{
		m_buffer[0] = static_cast<uint8_t>(opcode) & 0x0F;
		m_buffer[0] |= static_cast<uint8_t>(isFinal) << 7;
		m_buffer[0] |= static_cast<uint8_t>(rsv1) << 6;
		m_buffer[0] |= static_cast<uint8_t>(rsv2) << 5;
		m_buffer[0] |= static_cast<uint8_t>(rsv3) << 4;
		const auto payloadSize{ payload.size() };
		if (payloadSize <= 125) {
			m_buffer[1] = static_cast<uint8_t>(payloadSize);
			m_buffer.insert(m_buffer.end(), payload.begin(), payload.end());
			return;
		}

		if (opcode > MSAPI::WebSocketProtocol::Data::Opcode::Binary) [[unlikely]] {
			LOG_WARNING_NEW("WebSocket {} frame payload size cannot be > 125 "
							"bytes, provided: {}",
				EnumToString(opcode), payloadSize);
			return;
		}

		if (payloadSize <= 65535) {
			m_buffer[1] = 126;
			m_headerSize += static_cast<int8_t>(sizeof(uint16_t));
			m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
			*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = static_cast<uint16_t>(payloadSize);
			memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
			return;
		}

		m_buffer[1] = 127;
		m_headerSize += static_cast<int8_t>(sizeof(uint64_t));
		m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
		*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = static_cast<uint64_t>(payloadSize);
		memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
	}

	Data(const Data& other) = delete;
	Data& operator=(const Data& other) = delete;

	FORCE_INLINE Data(Data&& other) noexcept = default;
	FORCE_INLINE Data& operator=(Data&& other) noexcept = default;

	/**************************
	 * @brief Construct a new Data object, parse header and payload from buffer.
	 *
	 * @param recvBufferInfo Pointer to recv buffer info object with allocated memory.
	 */
	FORCE_INLINE Data(RecvBufferInfo* const recvBufferInfo)
	{
		// Below code assumes that MSAPI minimum buffer size is 2 bytes
		memcpy(m_buffer.data(), *recvBufferInfo->buffer, REQUIRED_HEADER_SIZE);
		auto payloadHeaderSize{ static_cast<uint64_t>(m_buffer[1] & 0x7F) };
		if (payloadHeaderSize == 0) {
			return;
		}

		if (IsMasked()) {
			m_headerSize += 4;
		}

		if (payloadHeaderSize <= 125) {
			const auto totalSize{ static_cast<size_t>(m_headerSize) + payloadHeaderSize };
			if (!Server::ReadAdditionalData(recvBufferInfo, totalSize)) {
				return;
			}
			m_buffer.resize(totalSize);

			memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE, payloadHeaderSize);
			return;
		}

		if (payloadHeaderSize == 126) {
			m_headerSize += static_cast<int8_t>(sizeof(uint16_t));
			if (!Server::LookForAdditionalData(recvBufferInfo, static_cast<size_t>(m_headerSize))) {
				return;
			}
			payloadHeaderSize = *reinterpret_cast<const uint16_t*>(
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE);
			const auto totalSize{ static_cast<size_t>(m_headerSize) + payloadHeaderSize };
			if (!Server::ReadAdditionalData(recvBufferInfo, totalSize)) {
				return;
			}
			m_buffer.resize(totalSize);
			memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE,
				payloadHeaderSize + sizeof(uint16_t));
			return;
		}

		if (payloadHeaderSize == 127) {
			m_headerSize += static_cast<int8_t>(sizeof(uint64_t));
			if (!Server::LookForAdditionalData(recvBufferInfo, static_cast<size_t>(m_headerSize))) {
				return;
			}
			payloadHeaderSize = *reinterpret_cast<const uint64_t*>(
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE);
			const auto totalSize{ static_cast<size_t>(m_headerSize) + payloadHeaderSize };
			if (!Server::ReadAdditionalData(recvBufferInfo, totalSize)) {
				return;
			}
			m_buffer.resize(totalSize);
			memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE,
				payloadHeaderSize + sizeof(uint64_t));
			return;
		}

		LOG_ERROR_NEW(
			"Impossible WebSocket payload size: {}, connection: {}", payloadHeaderSize, recvBufferInfo->connection);
	}

	template <typename T>
		requires(std::is_integral_v<T> && sizeof(T) == 1)
	class SplitGenerator {
	private:
		Data& m_data;
		const std::span<const T> m_buffer;
		size_t m_step;
		size_t m_offset{};
		constexpr static inline size_t MINIM_BUFFER_SIZE{ 4 };

	public:
		FORCE_INLINE SplitGenerator(Data& data, const std::span<const T> buffer, const size_t step)
			: m_data{ data }
			, m_buffer{ buffer }
		{
			const auto bufferSize{ m_buffer.size() };
			if (bufferSize < MINIM_BUFFER_SIZE) [[unlikely]] {
				LOG_WARNING_NEW(
					"WebSocket SplitGenerator buffer size must be greater than or equal to {}, provided: {}",
					MINIM_BUFFER_SIZE, bufferSize);
				return;
			}

			if (const auto opcode{ m_data.GetOpcode() }; opcode != Opcode::Text && opcode != Opcode::Binary)
				[[unlikely]] {
				LOG_WARNING_NEW(
					"WebSocket SplitGenerator can be used only with Text and Binary messages, provided opcode: {}",
					EnumToString(opcode));
				return;
			}

			if (step == 0) [[unlikely]] {
				const auto autoStep{ std::min(bufferSize / MINIM_BUFFER_SIZE, static_cast<size_t>(65535)) };
				LOG_WARNING_NEW("WebSocket SplitGenerator step must be greater than 0, provided: {}, will be used {}",
					step, autoStep);
				m_step = autoStep;
				return;
			}

			if (step >= bufferSize) [[unlikely]] {
				const auto autoStep{ std::min(bufferSize / MINIM_BUFFER_SIZE, static_cast<size_t>(65535)) };
				LOG_WARNING_NEW(
					"WebSocket SplitGenerator step must be less than buffer size {}, provided: {}, will be used {}",
					bufferSize, step, autoStep);
				m_step = autoStep;
				return;
			}

			m_step = step;
		}

		FORCE_INLINE [[nodiscard]] bool Get()
		{
			if (m_offset >= m_buffer.size()) {
				return false;
			}

			// Constructor guarantees that step is less than buffer size
			if (m_offset == 0) {
				m_data = Data{ m_buffer.subspan(0, m_step), m_data.GetOpcode(), 0, false, m_data.IsRsv1(),
					m_data.IsRsv2(), m_data.IsRsv3() };
				m_offset += m_step;
				return true;
			}

			m_data.m_buffer[0] ^= static_cast<uint8_t>(m_data.GetOpcode());
			const auto remainingSize{ m_buffer.size() - m_offset };
			if (remainingSize > m_step) {
				memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
					m_step);
				m_offset += m_step;
				return true;
			}

			m_data.m_buffer[0] |= 0b10000000;
			if (remainingSize == m_step) {
				memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
					m_step);
				m_offset += m_step;
				return true;
			}

			const auto currentStep{ std::min(m_step, remainingSize) };
			if (m_step <= 125) {
				memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
					currentStep);
				m_data.m_buffer[1] = static_cast<uint8_t>(currentStep);
				m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + currentStep);
				m_offset += currentStep;
				return true;
			}

			if (m_step <= 65535) {
				if (remainingSize > 125) {
					memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize),
						m_buffer.data() + m_offset, currentStep);
					*reinterpret_cast<uint16_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE)
						= static_cast<uint16_t>(currentStep);
					m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + currentStep);
					m_offset += currentStep;
					return true;
				}

				m_data.m_headerSize -= static_cast<int8_t>(sizeof(uint16_t));
				memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
					currentStep);
				m_data.m_buffer[1] = static_cast<uint8_t>(currentStep);
				m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + currentStep);
				m_offset += currentStep;
				return true;
			}

			if (remainingSize > 65535) {
				memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
					currentStep);
				*reinterpret_cast<uint64_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE)
					= static_cast<uint64_t>(currentStep);
				m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + currentStep);
				m_offset += currentStep;
				return true;
			}

			if (remainingSize > 125) {
				m_data.m_headerSize -= static_cast<int8_t>(sizeof(uint64_t) - sizeof(uint16_t));
				memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
					currentStep);
				*reinterpret_cast<uint16_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE)
					= static_cast<uint16_t>(currentStep);
				m_data.m_buffer[1] = static_cast<uint8_t>(126);
				m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + currentStep);
				m_offset += currentStep;
				return true;
			}

			m_data.m_headerSize -= static_cast<int8_t>(sizeof(uint64_t));
			memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
				currentStep);
			m_data.m_buffer[1] = static_cast<uint8_t>(currentStep);
			m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + currentStep);
			m_offset += currentStep;
			return true;
		}
	};

	template <typename S = std::nullptr_t, typename payloadOrNullptr_t = std::nullptr_t, typename T = void*>
		requires((std::is_same_v<S, int16_t> || std::is_same_v<S, CloseStatusCode> || std::is_same_v<S, std::nullptr_t>)
			&& (std::same_as<payloadOrNullptr_t, std::nullptr_t>
				|| (std::is_convertible_v<std::span<const T>, payloadOrNullptr_t> && std::is_integral_v<T>
					&& sizeof(T) == 1)))
	FORCE_INLINE [[nodiscard]] static Data CreateClose(
		const S statusCode = nullptr, const payloadOrNullptr_t reason = nullptr)
	{
		Data data;
		data.m_buffer[0] |= 0b10001000;
		if constexpr (!std::is_same_v<S, std::nullptr_t>) {
			if constexpr (!std::same_as<payloadOrNullptr_t, std::nullptr_t>) {
				const auto span{ std::span<const T>(reason) };
				const auto reasonSize{ span.size() };
				if (reasonSize > 123) [[unlikely]] {
					LOG_WARNING_NEW(
						"WebSocket Close frame reason size must be less than or equal to 123 bytes, provided: {}",
						reasonSize);
					data.m_buffer.resize(static_cast<size_t>(data.m_headerSize) + sizeof(int16_t));
					data.m_buffer[1] = static_cast<uint8_t>(sizeof(int16_t));
					*reinterpret_cast<uint16_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE)
						= static_cast<uint16_t>(statusCode);
					return data;
				}

				const auto totalSize{ sizeof(int16_t) + reasonSize };
				data.m_buffer.resize(static_cast<size_t>(data.m_headerSize) + totalSize);
				data.m_buffer[1] = static_cast<uint8_t>(totalSize);
				*reinterpret_cast<uint16_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE)
					= static_cast<uint16_t>(statusCode);
				memcpy(data.m_buffer.data() + static_cast<size_t>(data.m_headerSize) + sizeof(int16_t), span.data(),
					reasonSize);
				return data;
			}

			data.m_buffer.resize(static_cast<size_t>(data.m_headerSize) + sizeof(int16_t));
			data.m_buffer[1] = static_cast<uint8_t>(sizeof(int16_t));
			*reinterpret_cast<uint16_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE)
				= static_cast<uint16_t>(statusCode);
		}

		return data;
	}

	FORCE_INLINE [[nodiscard]] std::string ToString() const
	{
		const auto firstByte{ m_buffer[0] };
		return std::format("WebSocket data:\n{{"
						   "\n\tFIN          : {}"
						   "\n\tRSV1         : {}"
						   "\n\tRSV2         : {}"
						   "\n\tRSV3         : {}"
						   "\n\tOPCODE       : {}"
						   "\n\tMASK         : {}"
						   "\n\tHeader size  : {} | {:08b} {:08b}"
						   "\n\tPayload size : {}"
						   "\n\tIs valid     : {}\n}}",
			firstByte >> 7, (firstByte & 0x40) >> 6, (firstByte & 0x20) >> 5, (firstByte & 0x10) >> 4,
			EnumToString(static_cast<Opcode>(firstByte & 0x0F)), (m_buffer[1] & 0x80) >> 7, m_headerSize, firstByte,
			m_buffer[1], GetPayloadSize(), IsValid());
	}

	FORCE_INLINE [[nodiscard]] bool IsFinal() const noexcept { return (m_buffer[0] & 0x80) >> 7; }

	FORCE_INLINE [[nodiscard]] bool IsRsv1() const noexcept { return (m_buffer[0] & 0x40) >> 6; }

	FORCE_INLINE [[nodiscard]] bool IsRsv2() const noexcept { return (m_buffer[0] & 0x20) >> 5; }

	FORCE_INLINE [[nodiscard]] bool IsRsv3() const noexcept { return (m_buffer[0] & 0x10) >> 4; }

	FORCE_INLINE [[nodiscard]] Opcode GetOpcode() const noexcept { return static_cast<Opcode>(m_buffer[0] & 0x0F); }

	FORCE_INLINE [[nodiscard]] bool IsMasked() const noexcept { return (m_buffer[1] & 0x80) >> 7; }

	FORCE_INLINE [[nodiscard]] int8_t GetHeaderSize() const noexcept { return m_headerSize; }

	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetHeader() const noexcept
	{
		return std::span<const uint8_t>(m_buffer.data(), static_cast<size_t>(m_headerSize));
	}

	FORCE_INLINE [[nodiscard]] std::span<uint8_t> GetHeader() noexcept
	{
		return std::span<uint8_t>(m_buffer.data(), static_cast<size_t>(m_headerSize));
	}

	FORCE_INLINE [[nodiscard]] size_t GetPayloadSize() const noexcept
	{
		return m_buffer.size() - static_cast<size_t>(m_headerSize);
	}

	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetPayload() const noexcept
	{
		return std::span<const uint8_t>(
			m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.size() - static_cast<size_t>(m_headerSize));
	}

	FORCE_INLINE [[nodiscard]] std::span<uint8_t> GetPayload() noexcept
	{
		return std::span<uint8_t>(
			m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.size() - static_cast<size_t>(m_headerSize));
	}

	FORCE_INLINE [[nodiscard]] size_t GetBufferSize() const noexcept { return m_buffer.size(); }

	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetBuffer() const noexcept
	{
		return std::span<const uint8_t>(m_buffer.data(), m_buffer.size());
	}

	FORCE_INLINE [[nodiscard]] std::span<uint8_t> GetBuffer() noexcept
	{
		return std::span<uint8_t>(m_buffer.data(), m_buffer.size());
	}

	FORCE_INLINE void MergePayload(const Data& other) noexcept
	{
		// RFC 6455 each WebSocket fragment contains only its own payload size in header
		const auto otherPayloadSize{ other.GetPayloadSize() };
		if (otherPayloadSize == 0) {
			return;
		}

		const auto currentPayloadSize{ GetPayloadSize() };
		if (currentPayloadSize == 0) {
			m_buffer.resize(other.GetBufferSize());
			memcpy(m_buffer.data() + 1, other.GetBuffer().data() + 1, other.GetBufferSize() - 1);
			m_headerSize = other.GetHeaderSize();
			return;
		}

		if (std::numeric_limits<size_t>::max() - currentPayloadSize < otherPayloadSize) [[unlikely]] {
			LOG_WARNING_NEW(
				"Cannot merge WebSocket data, total payload size exceeds size_t max value, current: {}, incoming: {}",
				currentPayloadSize, otherPayloadSize);
			return;
		}

		if (currentPayloadSize >= 65536) {
			m_buffer.insert(m_buffer.end(), other.GetPayload().begin(), other.GetPayload().end());
			return;
		}

		const auto totalPayloadSize{ currentPayloadSize + other.GetPayloadSize() };
		if (currentPayloadSize <= 125) {
			if (totalPayloadSize <= 125) {
				m_buffer[1] = static_cast<uint8_t>(totalPayloadSize);
				m_buffer.insert(m_buffer.end(), other.GetPayload().begin(), other.GetPayload().end());
				return;
			}

			if (totalPayloadSize <= 65535) {
				m_headerSize += static_cast<int8_t>(sizeof(uint16_t));
				m_buffer.resize(static_cast<size_t>(m_headerSize) + totalPayloadSize);
				memmove(m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.data() + REQUIRED_HEADER_SIZE,
					currentPayloadSize);
				m_buffer[1] = 126;
				*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
					= static_cast<uint16_t>(totalPayloadSize);
				memcpy(m_buffer.data() + static_cast<size_t>(m_headerSize) + currentPayloadSize,
					other.GetPayload().data(), otherPayloadSize);
				return;
			}

			m_headerSize += static_cast<int8_t>(sizeof(uint64_t));
			m_buffer.resize(static_cast<size_t>(m_headerSize) + totalPayloadSize);
			memmove(m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.data() + REQUIRED_HEADER_SIZE,
				currentPayloadSize);
			m_buffer[1] = 127;
			*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
				= static_cast<uint64_t>(totalPayloadSize);
			memcpy(m_buffer.data() + static_cast<size_t>(m_headerSize) + currentPayloadSize, other.GetPayload().data(),
				otherPayloadSize);
			return;
		}

		if (totalPayloadSize <= 65535) {
			*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
				= static_cast<uint16_t>(totalPayloadSize);
			m_buffer.insert(m_buffer.end(), other.GetPayload().begin(), other.GetPayload().end());
			return;
		}

		m_headerSize += static_cast<int8_t>(sizeof(uint64_t) - sizeof(uint16_t));
		m_buffer.resize(static_cast<size_t>(m_headerSize) + totalPayloadSize);
		memmove(m_buffer.data() + static_cast<size_t>(m_headerSize),
			m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint16_t), currentPayloadSize);
		m_buffer[1] = 127;
		*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = static_cast<uint64_t>(totalPayloadSize);
		memcpy(m_buffer.data() + static_cast<size_t>(m_headerSize) + currentPayloadSize, other.GetPayload().data(),
			otherPayloadSize);
		return;
	}

	FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept
	{
		const auto opcode{ GetOpcode() };
		return opcode <= Opcode::Pong || (opcode >= Opcode::Continuation && opcode <= Opcode::Binary);
	}

	FORCE_INLINE [[nodiscard]] bool operator==(const Data& other) const noexcept
	{
		return m_buffer == other.m_buffer && m_headerSize == other.m_headerSize;
	}

	FORCE_INLINE [[nodiscard]] static std::string_view EnumToString(const Opcode value)
	{
		switch (value) {
		case Opcode::Continuation:
			return "Continuation";
		case Opcode::Text:
			return "Text";
		case Opcode::Binary:
			return "Binary";
		case Opcode::Close:
			return "Close";
		case Opcode::Ping:
			return "Ping";
		case Opcode::Pong:
			return "Pong";
		default:
			LOG_WARNING_NEW("Unknown WebSocket opcode {}", U(value));
			return "Unknown";
		}
	}

	FORCE_INLINE [[nodiscard]] static std::string_view EnumToString(const CloseStatusCode code)
	{
		switch (code) {
		case CloseStatusCode::NormalClosure:
			return "Normal closure";
		case CloseStatusCode::GoingAway:
			return "Going away";
		case CloseStatusCode::ProtocolError:
			return "Protocol error";
		case CloseStatusCode::UnsupportedData:
			return "Unsupported data";
		case CloseStatusCode::Reserved:
			return "Reserved";
		case CloseStatusCode::NoStatusReceived:
			return "No status received";
		case CloseStatusCode::AbnormalClosure:
			return "Abnormal closure";
		case CloseStatusCode::InvalidFramePayloadData:
			return "Invalid frame payload data";
		case CloseStatusCode::PolicyViolation:
			return "Policy violation";
		case CloseStatusCode::MessageTooBig:
			return "Message too big";
		case CloseStatusCode::MandatoryExtension:
			return "Mandatory extension";
		case CloseStatusCode::InternalError:
			return "Internal error";
		case CloseStatusCode::ServiceRestart:
			return "Service restart";
		case CloseStatusCode::TryAgainLater:
			return "Try again later";
		case CloseStatusCode::BadGateway:
			return "Bad gateway";
		case CloseStatusCode::TLSHandshakeFailure:
			return "TLS handshake failure";
		default:
			LOG_WARNING_NEW("Unknown WebSocket close status code {}", U(code));
			return "Unknown";
		}
	}

	FORCE_INLINE static void MaskPayload(uint8_t* const payload, const size_t size, const uint8_t* const mask) noexcept
	{
		uint32_t mask32;
		memcpy(&mask32, mask, 4);
		size_t index{};

		while (index < size && (reinterpret_cast<uintptr_t>(payload + index) & 3)) {
			payload[index] ^= mask[index & 3];
			index++;
		}

		uint32_t* const p32 = static_cast<uint32_t*>(static_cast<void*>(payload + index));
		const size_t blocks{ (size - index) / 4 };
		for (size_t b{}; b < blocks; b++) {
			p32[b] ^= mask32;
		}
		index += blocks * 4;

		while (index < size) {
			payload[index] ^= mask[index & 3];
			index++;
		}
	}
};

/**************************
 * @brief Send data to connection.
 *
 * @param connection Socket to send.
 * @param data Data to send.
 */
FORCE_INLINE void Send(const int connection, const Data& data)
{
	LOG_PROTOCOL_NEW("Send {} to connection: {}", data.ToString(), connection);

	const std::span<const uint8_t> buffer{ data.GetBuffer() };
	if (send(connection, buffer.data(), buffer.size(), MSG_NOSIGNAL) == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			return;
		}
		LOG_ERROR("Send event failed, connection: " + _S(connection) + ", data: " + data.ToString() + ". Error №"
			+ _S(errno) + ": " + std::strerror(errno));
	}
}

/**************************
 * @brief Object for reserving WebSocket messages. Use MSAPI_HANDLER_WEBSOCKET_PRESET macro to reserve and collect
 * WebSocket message.
 */
class IHandler {
private:
	const MSAPI::Application* const m_application;
	std::unordered_map<int, Data> m_fragmentedDataToConnection;
	Pthread::AtomicLock m_fragmentedDataLock;

public:
	/**************************
	 * @brief Construct a new IHandler object, empty constructor.
	 *
	 * @param application Readable pointer to application object.
	 */
	FORCE_INLINE IHandler(const MSAPI::Application* const application) noexcept
		: m_application{ application }
	{
	}

	/**************************
	 * @brief Default destructor.
	 */
	virtual ~IHandler() = default;

	/**************************
	 * @brief Collect WebSocket message from socket connection and call Handler function if Application is running.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved WebSocket message.
	 */
	FORCE_INLINE void Collect(const int connection, Data&& data)
	{
		if (m_application->IsRunning()) {
			LOG_PROTOCOL_NEW("{}, connection: {}", data.ToString(), connection);

			if (!data.IsValid()) [[unlikely]] {
				return;
			}

			switch (data.GetOpcode()) {
			case Data::Opcode::Continuation: {
				Data* fragmentedDataPtr{ nullptr };
				{
					Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
					const auto it{ m_fragmentedDataToConnection.find(connection) };
					if (it == m_fragmentedDataToConnection.end()) {
						m_fragmentedDataToConnection[connection] = std::move(data);
						return;
					}

					fragmentedDataPtr = &it->second;
				}

				fragmentedDataPtr->MergePayload(data);
				if (data.IsFinal()) {
					fragmentedDataPtr->GetHeader()[0] |= 0b10000000;
					if (fragmentedDataPtr->IsMasked()) {
						Data::MaskPayload(const_cast<uint8_t*>(fragmentedDataPtr->GetPayload().data()),
							fragmentedDataPtr->GetPayloadSize(),
							fragmentedDataPtr->GetHeader().data() + fragmentedDataPtr->GetHeaderSize() - 4);
					}

					LOG_PROTOCOL_NEW("Final {}, connection: {}", fragmentedDataPtr->ToString(), connection);
					HandleWebSocket(connection, std::move(*fragmentedDataPtr));
					Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
					m_fragmentedDataToConnection.erase(connection);
				}
				return;
			}
			case Data::Opcode::Text:
			case Data::Opcode::Binary:
				if (!data.IsFinal()) {
					Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
					if (m_fragmentedDataToConnection.contains(connection)) {
						LOG_WARNING_NEW("Received new fragmented message while previous fragmented message is not "
										"completed, message will be overwritten, connection: {}",
							connection);
					}
					m_fragmentedDataToConnection[connection] = std::move(data);
					return;
				}
				if (data.IsMasked()) {
					Data::MaskPayload(const_cast<uint8_t*>(data.GetPayload().data()), data.GetPayloadSize(),
						data.GetHeader().data() + data.GetHeaderSize() - 4);
				}
				HandleWebSocket(connection, std::move(data));
				return;
			case Data::Opcode::Close: {
				// RSV3 is used as a signal that it is a response to close message
				if (data.IsRsv3()) {
					return;
				}
				const auto payloadSize{ data.GetPayloadSize() };
				if (payloadSize >= 2) {
					const auto* payloadPtr{ data.GetPayload().data() };
					const auto statusCode{ *reinterpret_cast<const Data::CloseStatusCode*>(payloadPtr) };
					if (payloadSize > 2) {
						const auto reason{ std::string_view(
							reinterpret_cast<const char*>(payloadPtr + 2), payloadSize - 2) };
						LOG_PROTOCOL_NEW("Close frame received, status code: {} {}, reason: {}, connection: {}",
							static_cast<uint16_t>(statusCode), Data::EnumToString(statusCode), reason, connection);
					}
					else {
						LOG_PROTOCOL_NEW("Close frame received, status code: {} {}, connection: {}",
							static_cast<uint16_t>(statusCode), Data::EnumToString(statusCode), connection);
					}
				}
				else {
					LOG_PROTOCOL_NEW("Close frame received without status code, connection: {}", connection);
				}

				data.GetHeader()[0] |= 0b00010000; // Set RSV3 to signal that it is a response to close message
				Send(connection, data);
				// TODO: When Application will be a child of Sever, call Server::CloseConnection(connection) here
				return;
			}
			case Data::Opcode::Ping: {
				data.GetHeader()[0] ^= static_cast<uint8_t>(Data::Opcode::Ping);
				data.GetHeader()[0] |= static_cast<uint8_t>(Data::Opcode::Pong);
				Send(connection, data);
				return;
			}
			case Data::Opcode::Pong:
				HandleWebSocketPong(connection, std::move(data));
				return;
			default:
				LOG_WARNING_NEW("Unknown WebSocket opcode {}, connection: {}", U(data.GetOpcode()), connection);
				return;
			}

			HandleWebSocket(connection, std::move(data));
			return;
		}

		LOG_PROTOCOL_NEW("Application is not running. {}, connection: {}", data.ToString(), connection);
	}

	/**************************
	 * @brief Handler function for WebSocket message, does not handle ping/pong/close and fragmented messages. Ping
	 * message is handled by default implementation in Collect function, which sends pong message with same payload.
	 * HandleWebSocketPong function is provided for handling pong messages. Close message is responded by the same
	 * message with RSV3 set, which indicates it is a response to a close message. Fragmented messages are handled by
	 * default implementation in Collect function, which merges fragments and calls Handler function when final fragment
	 * is received.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved WebSocket message.
	 */
	virtual void HandleWebSocket(int connection, Data&& data) = 0;

	virtual void HandleWebSocketPong([[maybe_unused]] int connection, [[maybe_unused]] Data&& data) {};

	friend class MSAPI::IntegrationTest::WebSocketProtocol::Observer;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

} // namespace WebSocketProtocol

} // namespace MSAPI

#endif // MSAPI_WEBSOCKET_PROTOCOL_INL