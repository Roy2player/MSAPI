/**************************
 * @file        webSocket.inl
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
 *
 * @brief Implementation of data and parallel execution safe functional abstractions for 13 version (RFC 6455) WebSocket
 * protocol.
 */

#ifndef MSAPI_PROTOCOL_WEBSOCKET_INL
#define MSAPI_PROTOCOL_WEBSOCKET_INL

#include "../server/server.h"
#include <unordered_set>

namespace MSAPI {

class Application;

namespace Tests {

namespace Protocol {

namespace WebSocket {

class Observer;

} // namespace WebSocket

} // namespace Protocol

} // namespace Tests

namespace Protocol {

namespace WebSocket {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

class IHandler;

/**************************
 * @brief Object for containing data of WebSocket message.
 *
 * @todo Create custom allocator, which will allow to define a number of elements to be allocated on stack and point on
 * that memory by default. In case of vector, the initial capacity should be equal to that static number. Current
 * implementation is based on assumption that 2 bytes are always in buffer.
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
	static constexpr inline double MB{ 1024. * 1024. };
	static constexpr inline double MAXIMUM_HEADER_MB{ 10. / MB };

private:
	/*
		13 version (RFC 6455) WebSocket frame format

		1 bit	FIN	       	1 = final fragment of a message; 0 = more fragments follow
		1 bit	RSV1	   	Reserved (used for extensions like permessage-deflate)
		1 bit	RSV2	   	Reserved
		1 bit	RSV3	   	Reserved
		4 bits	OPCODE	    Frame type (text, binary, close, ping, pong, continuation)
		1 bit	MASK	   	1 = payload is masked (mandatory for client -> server)
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
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE Data() noexcept = default;

	/**
	 * @brief Construct a new Data object to be sent, create header and payload base on provided parameters. For
	 * close/ping and pong types maximum payload size is 125, if more is provided - no payload will be loaded.
	 *
	 * @param payload Payload data to be sent in message.
	 * @param opcode Message opcode.
	 * @param mask Masking key for payload masking. Default is 0 - no masking.
	 * @param isFinal Indicates if message is final fragment of message. Default is true.
	 * @param rsv1 Reserved bit 1. Default is false.
	 * @param rsv2 Reserved bit 2. Default is false.
	 * @param rsv3 Reserved bit 3. Default is false.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE Data(std::span<const uint8_t> payload, Opcode opcode, uint32_t mask = 0, bool isFinal = true,
		bool rsv1 = false, bool rsv2 = false, bool rsv3 = false) noexcept;

	Data(const Data& other) = delete;
	Data& operator=(const Data& other) = delete;

	FORCE_INLINE Data(Data&& other) noexcept = default;
	FORCE_INLINE Data& operator=(Data&& other) noexcept = default;

	/**************************
	 * @brief Construct a new memory owning Data object, parse header and payload from buffer.
	 *
	 * @attention Payload will be unmasked.
	 *
	 * @param recvBufferInfo Pointer to recv buffer info object with allocated memory. Must not be nullptr.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE Data(RecvBufferInfo* recvBufferInfo);

	/**
	 * @return String interpretation of WebSocket data message.
	 *
	 * @example WebSocket data:
	 * {
	 * 	FIN          : true
	 * 	RSV1         : false
	 * 	RSV2         : false
	 * 	RSV3         : false
	 * 	OPCODE       : Continuation
	 * 	MASK         : 341184513
	 * 	Header size  : 10 | 10000000 01111111
	 * 	Payload size : 5242881
	 * 	Is valid     : true
	 * }
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const;

	/**
	 * @return True if data is final, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsFinal() const noexcept;

	/**
	 * @return True if RVS1 is set, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsRsv1() const noexcept;

	/**
	 * @return True if RVS2 is set, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsRsv2() const noexcept;

	/**
	 * @return True if RVS3 is set, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsRsv3() const noexcept;

	/**
	 * @return Opcode of data.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] Opcode GetOpcode() const noexcept;

	/**
	 * @return True if data is masked, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsMasked() const noexcept;

	/**
	 * @return Return masking key or 0 if message is not masked. In terms of web socket 0 masking key produces no masked
	 * payload "x XOR 0 = x".
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint32_t GetMaskingKey() const noexcept;

	/**
	 * @return Size of header in bytes.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] int8_t GetHeaderSize() const noexcept;

	/**
	 * @return Const view on header part of buffer.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetHeader() const noexcept;

	/**
	 * @return Size of payload in bytes.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] size_t GetPayloadSize() const noexcept;

	/**
	 * @return Const view on payload part of buffer.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetPayload() const noexcept;

	/**
	 * @return Size of buffer in bytes.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] size_t GetBufferSize() const noexcept;

	/**
	 * @return Const view on buffer.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetBuffer() const noexcept;

	/**
	 * @brief Append payload from other data and update own header accordingly.
	 *
	 * @attention Payload must be already unmasked.
	 * @attention Additional overhead on memory move, up to copy, in case of header size change.
	 *
	 * @param other Source of new payload.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE void MergePayload(const Data& other) noexcept;

	/**
	 * @return Check opcode and verify header size based on masking and payload size.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept;

	/**
	 * @return True if buffers are equal.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool operator==(const Data& other) const noexcept;

	/**
	 * @return String interpretation of Opcode enum.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] static std::string_view EnumToString(Opcode value);

	/**
	 * @return String interpretation of CloseStatusCode enum.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] static std::string_view EnumToString(CloseStatusCode value);

	/**
	 * @return Generated 4 bytes long masking key.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] static uint32_t GenerateMaskingKey();

	/**
	 * @brief Apply mask. Works for masking and unmasking: "x XOR y XOR y = x".
	 *
	 * @param payload Begin of payload to be masked. Must not be a nullptr.
	 * @param size Size of payload to be masked.
	 * @param mask Mask to be applied.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE static void ApplyMask(uint8_t* payload, size_t size, uint32_t mask) noexcept;

	/**
	 * @brief Create close WebSocket message with specific status code and reason if provided. Reason will be added only
	 * for status code not equal to -1. Masking, if set, will be in message anyway.
	 *
	 * @tparam T int16_t or CloseStatusCode enum.
	 * @tparam S Span of 1 byte data.
	 *
	 * @param statusCode Ending status. Default is -1 - no status code should be written.
	 * @param reason Ending reason. Default is empty. Cannot be greater than 123 bytes.
	 * @param mask Masking key. Default is 0 - no masking.
	 *
	 * @test Has unit test.
	 */
	template <typename T = int16_t, typename S = uint8_t>
		requires((std::is_same_v<T, int16_t> || std::is_same_v<T, CloseStatusCode>) && sizeof(S) == 1)
	FORCE_INLINE [[nodiscard]] static Data CreateClose(
		T statusCode = static_cast<T>(-1), std::span<S> reason = {}, uint32_t mask = 0);

	/**
	 * @param payloadSize Payload size.
	 * @param isMasked Is masked flag.
	 *
	 * @return Expected header size of message with provided parameters.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] static constexpr int8_t GetExpectedHeaderSize(
		size_t payloadSize, bool isMasked) noexcept;

	/**
	 * @brief One-usage non-owning abstraction to manage the process of splitting one payload into several parts one by
	 * one.
	 *
	 * @tparam T Type of payload, must be one byte long.
	 */
	template <typename T>
		requires(sizeof(T) == 1)
	class SplitGenerator {
	private:
		Data& m_data;
		const std::span<const T> m_buffer;
		size_t m_step;
		size_t m_offset{};
		bool m_masking;

	public:
		/**
		 * @brief Construct non-owning split generator data structure for Binary and Text web socket data types. The
		 * minimum payload size in constant 4, the minimum step is 1. If zero step or step greater than payload size is
		 * provided, then step will be equal to min(buffer size / 4, 65535).
		 *
		 * @param data Web socket data destination.
		 * @param buffer Source of payload.
		 * @param step Payload step.
		 * @param masking Define if messages should be masked. Default is false.
		 *
		 * @test Has unit test.
		 */
		FORCE_INLINE SplitGenerator(Data& data, std::span<T> buffer, size_t step, bool masking = false);

		/**
		 * @brief Prepare next websocket data. In case of enabled masking each message has unique masking key.
		 *
		 * @return False if all buffer were already prepared, true otherwise.
		 *
		 * @test Has unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool Get();
	};

private:
	/**
	 * @brief Check if mask is not zero and set masking parameters in buffer.
	 *
	 * @attention Suppose to be applied only on empty data.
	 *
	 * @param mask Masking key.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE void CheckAndAddMaskForEmptyData(uint32_t mask) noexcept;

	/**
	 * @brief Remove masking from masked and add to unmasked data.
	 *
	 * @attention Suppose to be applied only for data with payload less than 126 bytes (small frames).
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE void ReverseMaskingInDataWithSmallPayload() noexcept;

	// Direct access to buffer
	friend class IHandler;
	friend class MSAPI::Tests::Protocol::WebSocket::Observer;
};

/**************************
 * @brief Send data to connection.
 *
 * @param connection Socket to send.
 * @param data Data to send.
 */
FORCE_INLINE void Send(int connection, const Data& data);

/**************************
 * @brief Functional data abstraction for pthread safe reserving WebSocket messages. Has dynamic limit for storing
 * fragmented data with ability to disable fragmented messages handling at all. Provide several handler functions:
 * - HandleWebSocket for text and binary messages;
 * - HandleWebSocketPong for pong messages;
 * Other types handled automatically:
 * - Continuation messages are handled when they are completed, final message contains masking key, if was, of first
 * initial message. Payload is unmasked.
 * - Close messages are answered back with same data, RSV3 flag set and opposite masking policy. If RSV3 flag is set,
 * close message will not be answered back.
 * - Ping messages are answered back with same data and opposite masking policy.
 *
 * Use MSAPI_HANDLER_WEBSOCKET_PRESET macro to reserve and collect WebSocket message.
 */
class IHandler {
public:
	/**
	 * @brief Data structure to accumulate fragments.
	 */
	struct FragmentedData {
		Data data;
		const int connection;
		Timer timestamp{};

		/**
		 * @brief Create fragmented data object with creation timestamp.
		 *
		 * @param data WebSocket data.
		 * @param connection Socket connection from which reserved message.
		 *
		 * @test Has unit test.
		 */
		FORCE_INLINE FragmentedData(Data&& data, int connection) noexcept;

		FragmentedData(FragmentedData&& other) = default;
		FragmentedData(const FragmentedData& other) = delete;

		FragmentedData& operator=(const FragmentedData& other) = delete;
		FragmentedData& operator=(FragmentedData&& other) = delete;
	};

private:
	const MSAPI::Application* const m_application;
	double m_storedFragmentedDataSizeMb{};
	double m_storedFragmentedDataLimitMb{ 10. };
	std::unordered_map<int, FragmentedData> m_fragmentedDataToConnection;
	std::map<Timer, FragmentedData*> m_fragmentedDataTimerToConnection;
	Pthread::AtomicLock m_fragmentedDataLock;

public:
	/**************************
	 * @brief Construct a new IHandler object, empty constructor.
	 *
	 * @param application Readable pointer to application object.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE IHandler(const MSAPI::Application* application) noexcept;

	/**************************
	 * @brief Default destructor.
	 */
	virtual ~IHandler() = default;

	/**************************
	 * @brief Handler function for text and binary messages.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved WebSocket message.
	 *
	 * @test Has unit test.
	 */
	virtual void HandleWebSocket(int connection, Data&& data) = 0;

	/**
	 * @brief Handler function for WebSocket pong message. Default implementation is empty.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved WebSocket message.
	 *
	 * @test Has unit test.
	 */
	virtual void HandleWebSocketPong([[maybe_unused]] int connection, [[maybe_unused]] Data&& data);

	/**************************
	 * @brief Collect WebSocket message from socket connection and call Handler function if message is final or
	 * completed and Application is running.
	 *
	 * @attention Continuation messages are stored till their completion or purging due to reaching the storage limit.
	 * - Storage fragmentation limit is a dynamic limit which guarantee that storage container will not contain more
	 * data then allowed;
	 * - If income fragment can be stored, it temporary cannot be purged as there is possibility of message complete;
	 * - If income continuation fragment come without initial fragment, it will be dropped away;
	 * - If income fragment is initial but some fragmented data is already stored, stored data will be purged;
	 * - If income message if not fragmented, but fragments are stored, they will not be purged.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved WebSocket message.
	 *
	 * @test Has unit test.
	 *
	 * @todo Add per-connection limit to do not allow one connection to occupy all the available shared limit.
	 * @todo When Application will be a child of Sever, call Server::CloseConnection(connection) at close opcode
	 * handling.
	 */
	FORCE_INLINE void Collect(int connection, Data&& data);

	/**
	 * @return Fragmented data limit.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE double GetFragmentedDataLimit() const noexcept;

	/**
	 * @brief Set new fragmented data limit. Limit cannot be less than zero. If limit is zero, then fragmented messages
	 * will not be stored at all. If limit is redused and is less than already occupied memory, then fragmented data
	 * will be purged in FIFO order.
	 *
	 * @attention Each fragment occupy its payload size + 10 bytes per each fragment. 10 bytes is the biggest possible
	 * header size.
	 *
	 * @param limitMb New limit in MB.
	 *
	 * @return True if limit was set, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool SetFragmentedDataLimit(double limitMb);

	/**
	 * @brief Clear stored fragmented data storage and counters.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE void Clear() noexcept;

	/**
	 * @brief Clear stored fragmented data for connection.
	 *
	 * @param connection Connection to be cleared.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE void ClearConnection(int connection) noexcept;

private:
	static inline constexpr bool CHECK_BEFORE{ true };
	static inline constexpr bool CHECK_USUAL{ false };

	/**
	 * @brief Purge stored fragmented data for connections in FIFO order.
	 *
	 * @tparam T If should be limit checked before first iteration or after.
	 *
	 * @return True if there is enough space, false otherwise.
	 *
	 * @todo Unit test.
	 */
	template <bool T> FORCE_INLINE [[nodiscard]] bool PurgeStoredData();

	/**
	 * @brief Check if additional fragmented data can be stored accordingly to the limits.
	 * - If income fragment and its already stored part is greater than limit, it all will be dropped away;
	 * - If income fragment cannot be stored because of limit, already stored fragments will be dropped away in FIFO
	 * order until there is enough space. No guarantee there will be;
	 *
	 * @attention This function assumes to be called under m_fragmentedDataLock lock.
	 *
	 * @param additionalSizeMb Size of new fragmented data in bytes.
	 * @param fragmentedDataPtr Pointer to already stored fragmented data.
	 * @param connection Connection of income fragment.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool CheckLimitsForStored(
		double additionalSizeMb, FragmentedData* fragmentedDataPtr, int connection);

	/**
	 * @brief Check if new fragmented data can be stored accordingly to the limits.
	 * - If income fragment itself greater than limit, it will be skipped;
	 * - If income fragment cannot be stored because of limit, already stored fragments will be dropped away in FIFO
	 * order until there is enough space. No guarantee there will be;
	 *
	 * @attention This function assumes to be called under m_fragmentedDataLock lock.
	 *
	 * @param additionalSizeMb Size of new fragmented data in bytes.
	 * @param connection Connection of income fragment.
	 *
	 * @return True if fragment can be processed, false otherwise.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool CheckLimitForNew(double additionalSizeMb, int connection);

	friend class MSAPI::Tests::Protocol::WebSocket::Observer;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE Data::Data(const std::span<const uint8_t> payload, const Opcode opcode, uint32_t mask, const bool isFinal,
	const bool rsv1, const bool rsv2, const bool rsv3) noexcept
{
	m_buffer[0] = static_cast<uint8_t>(opcode) & 0x0F;
	m_buffer[0] |= static_cast<uint8_t>(isFinal) << 7;
	m_buffer[0] |= static_cast<uint8_t>(rsv1) << 6;
	m_buffer[0] |= static_cast<uint8_t>(rsv2) << 5;
	m_buffer[0] |= static_cast<uint8_t>(rsv3) << 4;

	if (payload.empty()) {
		CheckAndAddMaskForEmptyData(mask);
		return;
	}

	const auto payloadSize{ payload.size() };
	if (payloadSize <= 125) {
		m_buffer[1] = static_cast<uint8_t>(payloadSize);
		if (mask == 0) {
			m_buffer.insert(m_buffer.end(), payload.begin(), payload.end());
		}
		else {
			m_buffer[1] |= 0b10000000;
			m_headerSize += int8_t{ sizeof(int32_t) };
			m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
			*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
			memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
			ApplyMask(m_buffer.data() + m_headerSize, payloadSize, mask);
		}
		return;
	}

	if (opcode > MSAPI::Protocol::WebSocket::Data::Opcode::Binary) [[unlikely]] {
		LOG_WARNING_NEW("WebSocket {} frame payload size cannot be > 125 "
						"bytes, provided: {}",
			EnumToString(opcode), payloadSize);
		CheckAndAddMaskForEmptyData(mask);
		return;
	}

	if (payloadSize <= 65535) {
		m_buffer[1] = 126;
		if (mask == 0) {
			m_headerSize += int8_t{ sizeof(uint16_t) };
			m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
			*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
				= htobe16(static_cast<uint16_t>(payloadSize));
			memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
		}
		else {
			m_buffer[1] |= 0b10000000;
			m_headerSize += int8_t{ sizeof(uint16_t) + sizeof(int32_t) };
			m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
			*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
				= htobe16(static_cast<uint16_t>(payloadSize));
			*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint16_t)) = mask;
			memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
			ApplyMask(m_buffer.data() + m_headerSize, payloadSize, mask);
		}
		return;
	}

	m_buffer[1] = 127;
	if (mask == 0) {
		m_headerSize += int8_t{ sizeof(uint64_t) };
		m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
		*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = htobe64(payloadSize);
		memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
	}
	else {
		m_buffer[1] |= 0b10000000;
		m_headerSize += int8_t{ sizeof(uint64_t) + sizeof(int32_t) };
		m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
		*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = htobe64(payloadSize);
		*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint64_t)) = mask;
		memcpy(m_buffer.data() + m_headerSize, payload.data(), payloadSize);
		ApplyMask(m_buffer.data() + m_headerSize, payloadSize, mask);
	}
}

FORCE_INLINE Data::Data(RecvBufferInfo* const recvBufferInfo)
{
	// Below code assumes that MSAPI minimum buffer size is 2 bytes
	memcpy(m_buffer.data(), *recvBufferInfo->buffer, REQUIRED_HEADER_SIZE);
	auto payloadHeaderSize{ static_cast<uint64_t>(m_buffer[1] & 0x7F) };
	if (static_cast<int64_t>(payloadHeaderSize) <= 0) {
		if (IsMasked()) {
			m_headerSize += int8_t{ sizeof(uint32_t) };
			if (!Server::ReadAdditionalData(recvBufferInfo, static_cast<size_t>(m_headerSize))) {
				return;
			}
			m_buffer.resize(static_cast<size_t>(m_headerSize));
			memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE, sizeof(uint32_t));
		}

		return;
	}

	const bool isMasked{ IsMasked() };
	if (isMasked) {
		m_headerSize += int8_t{ sizeof(uint32_t) };
	}

	if (payloadHeaderSize <= 125) {
		const auto totalSize{ static_cast<size_t>(m_headerSize) + payloadHeaderSize };
		if (!Server::ReadAdditionalData(recvBufferInfo, totalSize)) {
			return;
		}
		m_buffer.resize(totalSize);

		if (isMasked) {
			memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE,
				payloadHeaderSize + sizeof(uint32_t));

			uint32_t mask;
			memcpy(&mask, m_buffer.data() + REQUIRED_HEADER_SIZE, sizeof(uint32_t));
			ApplyMask(m_buffer.data() + m_headerSize, payloadHeaderSize, mask);
			return;
		}

		memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
			static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE, payloadHeaderSize);
		return;
	}

	if (payloadHeaderSize == 126) {
		m_headerSize += int8_t{ sizeof(uint16_t) };
		if (!Server::ReadAdditionalData(recvBufferInfo, static_cast<size_t>(m_headerSize))) {
			return;
		}
		payloadHeaderSize = be16toh(*reinterpret_cast<const uint16_t*>(
			static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE));
		const auto totalSize{ static_cast<size_t>(m_headerSize) + payloadHeaderSize };
		if (!Server::ReadAdditionalData(recvBufferInfo, totalSize, static_cast<size_t>(m_headerSize))) {
			return;
		}
		m_buffer.resize(totalSize);

		if (isMasked) {
			memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
				static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE,
				payloadHeaderSize + sizeof(uint16_t) + sizeof(uint32_t));
			ApplyMask(m_buffer.data() + m_headerSize, payloadHeaderSize,
				*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint16_t)));
			return;
		}

		memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
			static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE,
			payloadHeaderSize + sizeof(uint16_t));
		return;
	}

	m_headerSize += static_cast<int8_t>(sizeof(uint64_t));
	if (!Server::ReadAdditionalData(recvBufferInfo, static_cast<size_t>(m_headerSize))) {
		return;
	}
	payloadHeaderSize = be64toh(
		*reinterpret_cast<const uint64_t*>(static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE));
	const auto totalSize{ static_cast<size_t>(m_headerSize) + payloadHeaderSize };
	if (!Server::ReadAdditionalData(recvBufferInfo, totalSize, static_cast<size_t>(m_headerSize))) {
		return;
	}
	m_buffer.resize(totalSize);

	if (isMasked) {
		memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
			static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE,
			payloadHeaderSize + sizeof(uint64_t) + sizeof(uint32_t));
		ApplyMask(m_buffer.data() + m_headerSize, payloadHeaderSize,
			*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint64_t)));
		return;
	}

	memcpy(m_buffer.data() + REQUIRED_HEADER_SIZE,
		static_cast<const char*>(*recvBufferInfo->buffer) + REQUIRED_HEADER_SIZE, payloadHeaderSize + sizeof(uint64_t));
}

FORCE_INLINE [[nodiscard]] std::string Data::ToString() const
{
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
		IsFinal(), IsRsv1(), IsRsv2(), IsRsv3(), EnumToString(GetOpcode()), GetMaskingKey(), m_headerSize, m_buffer[0],
		m_buffer[1], GetPayloadSize(), IsValid());
}

FORCE_INLINE [[nodiscard]] bool Data::IsFinal() const noexcept { return (m_buffer[0] & 0x80) >> 7; }

FORCE_INLINE [[nodiscard]] bool Data::IsRsv1() const noexcept { return (m_buffer[0] & 0x40) >> 6; }

FORCE_INLINE [[nodiscard]] bool Data::IsRsv2() const noexcept { return (m_buffer[0] & 0x20) >> 5; }

FORCE_INLINE [[nodiscard]] bool Data::IsRsv3() const noexcept { return (m_buffer[0] & 0x10) >> 4; }

FORCE_INLINE [[nodiscard]] Data::Opcode Data::GetOpcode() const noexcept
{
	return static_cast<Opcode>(m_buffer[0] & 0x0F);
}

FORCE_INLINE [[nodiscard]] bool Data::IsMasked() const noexcept { return (m_buffer[1] & 0x80) >> 7; }

FORCE_INLINE [[nodiscard]] uint32_t Data::GetMaskingKey() const noexcept
{
	// Data producers guarantee that if message contains masked flag, the buffer has capacity for its key.
	if (IsMasked()) {
		return *reinterpret_cast<const uint32_t*>(m_buffer.data() + m_headerSize - 4);
	}

	return 0;
}

FORCE_INLINE [[nodiscard]] int8_t Data::GetHeaderSize() const noexcept { return m_headerSize; }

FORCE_INLINE [[nodiscard]] std::span<const uint8_t> Data::GetHeader() const noexcept
{
	return std::span<const uint8_t>(m_buffer.data(), static_cast<size_t>(m_headerSize));
}

FORCE_INLINE [[nodiscard]] size_t Data::GetPayloadSize() const noexcept
{
	return m_buffer.size() - static_cast<size_t>(m_headerSize);
}

FORCE_INLINE [[nodiscard]] std::span<const uint8_t> Data::GetPayload() const noexcept
{
	return std::span<const uint8_t>(
		m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.size() - static_cast<size_t>(m_headerSize));
}

FORCE_INLINE [[nodiscard]] size_t Data::GetBufferSize() const noexcept { return m_buffer.size(); }

FORCE_INLINE [[nodiscard]] std::span<const uint8_t> Data::GetBuffer() const noexcept
{
	return std::span<const uint8_t>(m_buffer.data(), m_buffer.size());
}

FORCE_INLINE void Data::MergePayload(const Data& other) noexcept
{
	// Each WebSocket fragment contains only its own payload size in header
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

	const auto totalPayloadSize{ currentPayloadSize + other.GetPayloadSize() };
	if (currentPayloadSize >= 65536) {
		*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = htobe64(totalPayloadSize);
		m_buffer.insert(m_buffer.end(), other.GetPayload().begin(), other.GetPayload().end());
		return;
	}

	if (currentPayloadSize <= 125) {
		if (totalPayloadSize <= 125) {
			const bool isMasked{ IsMasked() };
			m_buffer[1] = static_cast<uint8_t>(totalPayloadSize);
			if (isMasked) {
				m_buffer[1] |= 0b10000000;
			}
			m_buffer.insert(m_buffer.end(), other.GetPayload().begin(), other.GetPayload().end());
			return;
		}

		if (totalPayloadSize <= 65535) {
			m_headerSize += int8_t{ sizeof(uint16_t) };
			m_buffer.resize(static_cast<size_t>(m_headerSize) + totalPayloadSize);
			memmove(m_buffer.data() + static_cast<size_t>(m_headerSize),
				m_buffer.data() + m_headerSize - sizeof(uint16_t), currentPayloadSize);
			const bool isMasked{ IsMasked() };
			m_buffer[1] = 126;
			if (isMasked) {
				m_buffer[1] |= 0b10000000;
			}
			*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
				= htobe16(static_cast<uint16_t>(totalPayloadSize));
			memcpy(m_buffer.data() + static_cast<size_t>(m_headerSize) + currentPayloadSize, other.GetPayload().data(),
				otherPayloadSize);
			return;
		}

		m_headerSize += int8_t{ sizeof(uint64_t) };
		m_buffer.resize(static_cast<size_t>(m_headerSize) + totalPayloadSize);
		memmove(m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.data() + m_headerSize - sizeof(uint64_t),
			currentPayloadSize);
		const bool isMasked{ IsMasked() };
		m_buffer[1] = 127;
		if (isMasked) {
			m_buffer[1] |= 0b10000000;
		}
		*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = htobe64(totalPayloadSize);
		memcpy(m_buffer.data() + static_cast<size_t>(m_headerSize) + currentPayloadSize, other.GetPayload().data(),
			otherPayloadSize);
		return;
	}

	if (totalPayloadSize <= 65535) {
		*reinterpret_cast<uint16_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE)
			= htobe16(static_cast<uint16_t>(totalPayloadSize));
		m_buffer.insert(m_buffer.end(), other.GetPayload().begin(), other.GetPayload().end());
		return;
	}

	m_headerSize += int8_t{ sizeof(uint64_t) - sizeof(uint16_t) };
	m_buffer.resize(static_cast<size_t>(m_headerSize) + totalPayloadSize);
	memmove(m_buffer.data() + static_cast<size_t>(m_headerSize),
		m_buffer.data() + m_headerSize - sizeof(uint64_t) + sizeof(uint16_t), currentPayloadSize);
	const bool isMasked{ IsMasked() };
	m_buffer[1] = 127;
	if (isMasked) {
		m_buffer[1] |= 0b10000000;
	}
	*reinterpret_cast<uint64_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = htobe64(totalPayloadSize);
	memcpy(m_buffer.data() + static_cast<size_t>(m_headerSize) + currentPayloadSize, other.GetPayload().data(),
		otherPayloadSize);
}

FORCE_INLINE [[nodiscard]] bool Data::IsValid() const noexcept
{
	const auto opcode{ GetOpcode() };

	if (opcode < Opcode::Continuation || (opcode > Opcode::Binary && opcode < Opcode::Close) || opcode > Opcode::Pong) {
		return false;
	}

	const auto payloadSize{ GetPayloadSize() };
	if (static_cast<size_t>(m_headerSize) > m_buffer.size()) {
		return false;
	}

	return m_headerSize == GetExpectedHeaderSize(payloadSize, IsMasked());
}

FORCE_INLINE [[nodiscard]] bool Data::operator==(const Data& other) const noexcept
{
	return m_buffer == other.m_buffer && m_headerSize == other.m_headerSize;
}

FORCE_INLINE [[nodiscard]] std::string_view Data::EnumToString(const Opcode value)
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

FORCE_INLINE [[nodiscard]] std::string_view Data::EnumToString(const CloseStatusCode value)
{
	switch (value) {
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
		LOG_WARNING_NEW("Unknown WebSocket close status code {}", U(value));
		return "Unknown";
	}
}

FORCE_INLINE [[nodiscard]] uint32_t Data::GenerateMaskingKey()
{
	Timer now;
	const auto nanoseconds{ static_cast<uint32_t>(now.GetNanoseconds()) };
	const auto seconds{ static_cast<uint32_t>(now.GetSeconds() & 0x00000000FFFFFFFF) };
	uint64_t doubleKey{ nanoseconds + seconds };
	doubleKey *= nanoseconds;
	doubleKey *= seconds;
	doubleKey *= doubleKey;
	doubleKey += 1;

	return static_cast<uint32_t>(doubleKey >> 32) + static_cast<uint32_t>(doubleKey);
}

FORCE_INLINE void Data::ApplyMask(uint8_t* const payload, const size_t size, const uint32_t mask) noexcept
{
	size_t index{};
	while (index < size) {
		payload[index] ^= static_cast<uint8_t>(mask >> (8 * (index & 3)));
		index++;
	}
}

template <typename T, typename S>
	requires((std::is_same_v<T, int16_t> || std::is_same_v<T, Data::CloseStatusCode>) && sizeof(S) == 1)
FORCE_INLINE [[nodiscard]] Data Data::CreateClose(const T statusCode, const std::span<S> reason, const uint32_t mask)
{
	Data data;
	data.m_buffer[0] |= 0b10001000;
	if (statusCode != static_cast<T>(-1)) {
		if (!reason.empty()) {
			const auto reasonSize{ reason.size() };
			if (reasonSize > 123) [[unlikely]] {
				LOG_WARNING_NEW(
					"WebSocket Close frame reason size must be less than or equal to 123 bytes, provided: {}",
					reasonSize);

#define TMP_MSAPI_PROTOCOL_WEBSOCKET_CREATE_CLOSE_ONLY_STATUS_CODE                                                     \
	if (mask == 0) {                                                                                                   \
		data.m_buffer.resize(static_cast<size_t>(REQUIRED_HEADER_SIZE) + sizeof(int16_t));                             \
		data.m_buffer[1] = uint8_t{ sizeof(int16_t) };                                                                 \
		*reinterpret_cast<uint16_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE)                                      \
			= htobe16(static_cast<uint16_t>(statusCode));                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		data.m_headerSize += int8_t{ sizeof(uint32_t) };                                                               \
		data.m_buffer.resize(static_cast<size_t>(data.m_headerSize) + sizeof(int16_t));                                \
		data.m_buffer[1] = uint8_t{ sizeof(int16_t) };                                                                 \
		data.m_buffer[1] |= 0b10000000;                                                                                \
		*reinterpret_cast<uint32_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;                              \
		*reinterpret_cast<uint16_t*>(data.m_buffer.data() + static_cast<size_t>(data.m_headerSize))                    \
			= htobe16(static_cast<uint16_t>(statusCode));                                                              \
		ApplyMask(data.m_buffer.data() + static_cast<size_t>(data.m_headerSize), sizeof(int16_t), mask);               \
	}

				TMP_MSAPI_PROTOCOL_WEBSOCKET_CREATE_CLOSE_ONLY_STATUS_CODE;

				return data;
			}

			if (mask == 0) {
				const auto totalSize{ sizeof(int16_t) + reasonSize };
				data.m_buffer.resize(static_cast<size_t>(REQUIRED_HEADER_SIZE) + totalSize);
				data.m_buffer[1] = static_cast<uint8_t>(totalSize);
				*reinterpret_cast<uint16_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE)
					= htobe16(static_cast<uint16_t>(statusCode));
				memcpy(data.m_buffer.data() + static_cast<size_t>(REQUIRED_HEADER_SIZE) + sizeof(int16_t),
					reason.data(), reasonSize);
			}
			else {
				data.m_headerSize += int8_t{ sizeof(uint32_t) };
				const auto totalSize{ sizeof(int16_t) + reasonSize };
				data.m_buffer.resize(static_cast<size_t>(data.m_headerSize) + totalSize);
				data.m_buffer[1] = static_cast<uint8_t>(totalSize);
				data.m_buffer[1] |= 0b10000000;
				*reinterpret_cast<uint32_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
				*reinterpret_cast<uint16_t*>(data.m_buffer.data() + static_cast<size_t>(data.m_headerSize))
					= htobe16(static_cast<uint16_t>(statusCode));
				memcpy(data.m_buffer.data() + static_cast<size_t>(data.m_headerSize) + sizeof(int16_t), reason.data(),
					reasonSize);
				ApplyMask(
					data.m_buffer.data() + static_cast<size_t>(data.m_headerSize), reasonSize + sizeof(int16_t), mask);
			}

			return data;
		}

		TMP_MSAPI_PROTOCOL_WEBSOCKET_CREATE_CLOSE_ONLY_STATUS_CODE;
		return data;

#undef TMP_MSAPI_PROTOCOL_WEBSOCKET_CREATE_CLOSE_ONLY_STATUS_CODE
	}

	if (mask != 0) {
		data.m_headerSize += int8_t{ sizeof(uint32_t) };
		data.m_buffer.resize(static_cast<size_t>(data.m_headerSize));
		data.m_buffer[1] |= 0b10000000;
		*reinterpret_cast<uint32_t*>(data.m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
	}

	return data;
}

FORCE_INLINE [[nodiscard]] constexpr int8_t Data::GetExpectedHeaderSize(
	const size_t payloadSize, const bool isMasked) noexcept
{
	auto result{ static_cast<int8_t>(isMasked ? 4 : 0) };
	if (payloadSize <= 125) {
		return result + 2;
	}

	if (payloadSize <= 65535) {
		return result + 4;
	}

	return result + 10;
}

template <typename T>
	requires(sizeof(T) == 1)
FORCE_INLINE Data::SplitGenerator<T>::SplitGenerator(
	Data& data, const std::span<T> buffer, const size_t step, const bool masking)
	: m_data{ data }
	, m_buffer{ buffer }
	, m_masking{ masking }
{
	const auto bufferSize{ m_buffer.size() };
	if (bufferSize < 4) [[unlikely]] {
		LOG_WARNING_NEW(
			"WebSocket SplitGenerator buffer size must be greater than or equal to 4, provided: {}", bufferSize);
		m_offset = bufferSize;
		return;
	}

	if (const auto opcode{ m_data.GetOpcode() }; opcode != Opcode::Text && opcode != Opcode::Binary) [[unlikely]] {
		LOG_WARNING_NEW("WebSocket SplitGenerator can be used only with Text and Binary messages, provided opcode: {}",
			EnumToString(opcode));
		m_offset = bufferSize;
		return;
	}

	if (step == 0) [[unlikely]] {
		const auto autoStep{ std::min(bufferSize / 4, size_t{ 65535 }) };
		LOG_WARNING_NEW(
			"WebSocket SplitGenerator step must be greater than 0, provided: {}, will be used {}", step, autoStep);
		m_step = autoStep;
		return;
	}

	if (step >= bufferSize) [[unlikely]] {
		const auto autoStep{ std::min(bufferSize / 4, size_t{ 65535 }) };
		LOG_WARNING_NEW("WebSocket SplitGenerator step must be less than buffer size {}, provided: {}, will be used {}",
			bufferSize, step, autoStep);
		m_step = autoStep;
		return;
	}

	m_step = step;
}

template <typename T>
	requires(sizeof(T) == 1)
FORCE_INLINE [[nodiscard]] bool Data::SplitGenerator<T>::Get()
{
	if (m_offset >= m_buffer.size()) {
		return false;
	}

	// Constructor guarantees that step is less than buffer size
	if (m_offset == 0) {
		uint32_t mask{};
		if (m_masking) {
			mask = Data::GenerateMaskingKey();
		}
		if constexpr (std::is_same_v<T, uint8_t>) {
			m_data = Data{ m_buffer.subspan(0, m_step), m_data.GetOpcode(), mask, false, m_data.IsRsv1(),
				m_data.IsRsv2(), m_data.IsRsv3() };
		}
		else {
			const std::span<const uint8_t> specificSpan{ reinterpret_cast<const uint8_t*>(m_buffer.data()), m_step };
			m_data = Data{ specificSpan, m_data.GetOpcode(), mask, false, m_data.IsRsv1(), m_data.IsRsv2(),
				m_data.IsRsv3() };
		}
		m_offset += m_step;
		return true;
	}

	m_data.m_buffer[0] ^= static_cast<uint8_t>(m_data.GetOpcode());
	const auto remainingSize{ m_buffer.size() - m_offset };
	if (remainingSize > m_step) {
		memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset, m_step);
		if (m_masking) {
			const auto mask{ Data::GenerateMaskingKey() };
			*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + m_data.m_headerSize - 4) = mask;
			Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_step, mask);
		}
		m_offset += m_step;
		return true;
	}

	m_data.m_buffer[0] |= 0b10000000;
	if (remainingSize == m_step) {
		memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset, m_step);
		if (m_masking) {
			const auto mask{ Data::GenerateMaskingKey() };
			*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + m_data.m_headerSize - 4) = mask;
			Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_step, mask);
		}
		m_offset += m_step;
		return true;
	}

	if (m_step <= 125) {
		memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
			remainingSize);
		m_data.m_buffer[1] = static_cast<uint8_t>(remainingSize);
		if (m_masking) {
			m_data.m_buffer[1] |= 0b10000000;
			const auto mask{ Data::GenerateMaskingKey() };
			*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
			Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), remainingSize, mask);
		}
		m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + remainingSize);
		m_offset += remainingSize;
		return true;
	}

	if (m_step <= 65535) {
		if (remainingSize > 125) {
			memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
				remainingSize);
			if (m_masking) {
				const auto mask{ Data::GenerateMaskingKey() };
				*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint16_t)) = mask;
				Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), remainingSize, mask);
			}
			*reinterpret_cast<uint16_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE)
				= htobe16(static_cast<uint16_t>(remainingSize));
			m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + remainingSize);
			m_offset += remainingSize;
			return true;
		}

		m_data.m_headerSize -= int8_t{ sizeof(uint16_t) };
		memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
			remainingSize);
		m_data.m_buffer[1] = static_cast<uint8_t>(remainingSize);
		if (m_masking) {
			m_data.m_buffer[1] |= 0b10000000;
			const auto mask{ Data::GenerateMaskingKey() };
			*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
			Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), remainingSize, mask);
		}
		m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + remainingSize);
		m_offset += remainingSize;
		return true;
	}

	if (remainingSize > 65535) {
		memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
			remainingSize);
		if (m_masking) {
			const auto mask{ Data::GenerateMaskingKey() };
			*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint64_t)) = mask;
			Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), remainingSize, mask);
		}
		*reinterpret_cast<uint64_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE) = htobe64(remainingSize);
		m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + remainingSize);
		m_offset += remainingSize;
		return true;
	}

	if (remainingSize > 125) {
		m_data.m_headerSize -= int8_t{ sizeof(uint64_t) - sizeof(uint16_t) };
		memcpy(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset,
			remainingSize);
		m_data.m_buffer[1] = uint8_t{ 126 };
		if (m_masking) {
			m_data.m_buffer[1] |= 0b10000000;
			const auto mask{ Data::GenerateMaskingKey() };
			*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint16_t)) = mask;
			Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), remainingSize, mask);
		}
		*reinterpret_cast<uint16_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE)
			= htobe16(static_cast<uint16_t>(remainingSize));
		m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + remainingSize);
		m_offset += remainingSize;
		return true;
	}

	m_data.m_headerSize -= int8_t{ sizeof(uint64_t) };
	memcpy(
		m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), m_buffer.data() + m_offset, remainingSize);
	m_data.m_buffer[1] = static_cast<uint8_t>(remainingSize);
	if (m_masking) {
		m_data.m_buffer[1] |= 0b10000000;
		const auto mask{ Data::GenerateMaskingKey() };
		*reinterpret_cast<uint32_t*>(m_data.m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
		Data::ApplyMask(m_data.m_buffer.data() + static_cast<size_t>(m_data.m_headerSize), remainingSize, mask);
	}
	m_data.m_buffer.resize(static_cast<size_t>(m_data.m_headerSize) + remainingSize);
	m_offset += remainingSize;
	return true;
}

FORCE_INLINE void Data::CheckAndAddMaskForEmptyData(const uint32_t mask) noexcept
{
	if (mask != 0) {
		m_buffer[1] |= 0b10000000;
		m_headerSize += int8_t{ sizeof(int32_t) };
		m_buffer.resize(static_cast<size_t>(m_headerSize));
		*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
	}
}

FORCE_INLINE void Data::ReverseMaskingInDataWithSmallPayload() noexcept
{
	const auto payloadSize{ GetPayloadSize() };
	if (IsMasked()) {
		m_headerSize = REQUIRED_HEADER_SIZE;
		m_buffer[1] &= 0b01111111;
		if (payloadSize != 0) {
			memmove(m_buffer.data() + REQUIRED_HEADER_SIZE, m_buffer.data() + REQUIRED_HEADER_SIZE + sizeof(uint32_t),
				payloadSize);
			m_buffer.resize(REQUIRED_HEADER_SIZE + payloadSize);
			return;
		}

		m_buffer.resize(REQUIRED_HEADER_SIZE);
		return;
	}

	m_headerSize += int8_t{ sizeof(uint32_t) };
	m_buffer[1] |= 0b10000000;
	const auto mask{ Data::GenerateMaskingKey() };
	m_buffer.resize(static_cast<size_t>(m_headerSize) + payloadSize);
	if (payloadSize != 0) {
		memmove(
			m_buffer.data() + static_cast<size_t>(m_headerSize), m_buffer.data() + REQUIRED_HEADER_SIZE, payloadSize);
		Data::ApplyMask(m_buffer.data() + static_cast<size_t>(m_headerSize), payloadSize, mask);
	}
	*reinterpret_cast<uint32_t*>(m_buffer.data() + REQUIRED_HEADER_SIZE) = mask;
}

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

FORCE_INLINE IHandler::FragmentedData::FragmentedData(Data&& data, const int connection) noexcept
	: data{ std::move(data) }
	, connection{ connection }
{
}

FORCE_INLINE IHandler::IHandler(const MSAPI::Application* const application) noexcept
	: m_application{ application }
{
}

FORCE_INLINE void IHandler::HandleWebSocketPong([[maybe_unused]] const int connection, [[maybe_unused]] Data&& data) { }

FORCE_INLINE void IHandler::Collect(const int connection, Data&& data)
{
	if (m_application->IsRunning()) {
		LOG_PROTOCOL_NEW("{}, connection: {}", data.ToString(), connection);

		if (!data.IsValid()) [[unlikely]] {
			return;
		}

		switch (data.GetOpcode()) {
		case Data::Opcode::Continuation: {
			FragmentedData* fragmentedDataPtr{ nullptr };
			{
				Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
				const auto it{ m_fragmentedDataToConnection.find(connection) };
				if (it == m_fragmentedDataToConnection.end()) {
					LOG_WARNING_NEW("Received continuation frame without initial fragment, message will be "
									"ignored, connection: {}",
						connection);
					return;
				}

				fragmentedDataPtr = &it->second;

				const auto incomeFragmentSize{ static_cast<double>(data.GetPayloadSize()) / Data::MB };
				// Potentially the message is final, so it is removed from map and its potential purging is not
				// possible, but the occupied memory size is not reduced yet
				m_fragmentedDataTimerToConnection.erase(fragmentedDataPtr->timestamp);
				if (!CheckLimitsForStored(incomeFragmentSize, fragmentedDataPtr, connection)) {
					return;
				}
				fragmentedDataPtr->timestamp = Timer{};
			}

			auto& dataRef{ fragmentedDataPtr->data };
			dataRef.MergePayload(data);
			if (data.IsFinal()) {
				dataRef.m_buffer[0] |= 0b10000000;

				LOG_PROTOCOL_NEW("Final {}, connection: {}", dataRef.ToString(), connection);
				const auto dataSizeMb{ static_cast<double>(dataRef.GetPayloadSize()) / Data::MB
					+ Data::MAXIMUM_HEADER_MB };
				HandleWebSocket(connection, std::move(fragmentedDataPtr->data));
				Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
				m_fragmentedDataToConnection.erase(connection);
				m_storedFragmentedDataSizeMb -= dataSizeMb;
				return;
			}

			// Runtime overhead in favor of FIFO memory cleaning
			// The design of protocol itself does not allow to have robust memory storing model without trade offs
			Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
			m_fragmentedDataTimerToConnection.emplace(fragmentedDataPtr->timestamp, fragmentedDataPtr);
			return;
		}
		case Data::Opcode::Text:
		case Data::Opcode::Binary:
			if (!data.IsFinal()) {
				Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
				if (!CheckLimitForNew(static_cast<double>(data.GetPayloadSize()) / Data::MB + Data::MAXIMUM_HEADER_MB,
						connection)) [[unlikely]] {
					return;
				}

				if (auto it{ m_fragmentedDataToConnection.find(connection) }; it != m_fragmentedDataToConnection.end())
					[[unlikely]] {
					LOG_WARNING_NEW("Received new fragmented message while previous fragmented message is not "
									"completed, message will be overwritten, connection: {}",
						connection);
					m_storedFragmentedDataSizeMb
						-= static_cast<double>(it->second.data.GetPayloadSize()) / Data::MB + Data::MAXIMUM_HEADER_MB;
					it->second.data = std::move(data);
					m_fragmentedDataTimerToConnection.erase(it->second.timestamp);
					it->second.timestamp = Timer{};
					m_fragmentedDataTimerToConnection.emplace(it->second.timestamp, &it->second);
					return;
				}

				const auto it{ m_fragmentedDataToConnection
								   .emplace(connection, FragmentedData{ std::move(data), connection })
								   .first };
				m_fragmentedDataTimerToConnection.emplace(it->second.timestamp, &it->second);
				return;
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
				const auto statusCode{ static_cast<Data::CloseStatusCode>(
					be16toh(*reinterpret_cast<const uint16_t*>(payloadPtr))) };
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

			data.m_buffer[0] |= 0b00010000; // Set RSV3 to signal that it is a response to close message
			data.ReverseMaskingInDataWithSmallPayload();
			Send(connection, data);
			// TODO: When Application will be a child of Sever, call Server::CloseConnection(connection) here
			return;
		}
		case Data::Opcode::Ping: {
			data.m_buffer[0] ^= static_cast<uint8_t>(Data::Opcode::Ping);
			data.m_buffer[0] |= static_cast<uint8_t>(Data::Opcode::Pong);
			data.ReverseMaskingInDataWithSmallPayload();
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

FORCE_INLINE double IHandler::GetFragmentedDataLimit() const noexcept { return m_storedFragmentedDataLimitMb; }

FORCE_INLINE [[nodiscard]] bool IHandler::SetFragmentedDataLimit(const double limitMb)
{
	if (Helper::FloatLess(limitMb, 0.)) [[unlikely]] {
		LOG_WARNING_NEW("WebSocket fragmented data limit must be greater than or equal to 0, provided: {}", limitMb);
		return false;
	}

	const auto ratio{ Helper::CompareFloats(limitMb, m_storedFragmentedDataLimitMb) };
	if (ratio == 0) {
		return false;
	}

	Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
	LOG_PROTOCOL_NEW("WebSocket fragmented data limit changed from {:.17f} MB to {:.17f} MB",
		m_storedFragmentedDataLimitMb, limitMb);
	m_storedFragmentedDataLimitMb = limitMb;
	if (ratio < 0) {
		(void)PurgeStoredData<CHECK_BEFORE>();
	}

	return true;
}

FORCE_INLINE void IHandler::Clear() noexcept
{
	Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
	m_fragmentedDataToConnection.clear();
	m_fragmentedDataTimerToConnection.clear();
	m_storedFragmentedDataSizeMb = 0.;
}

FORCE_INLINE void IHandler::ClearConnection(const int connection) noexcept
{
	Pthread::AtomicLock::ExitGuard guard{ m_fragmentedDataLock };
	const auto it{ m_fragmentedDataToConnection.find(connection) };
	if (it != m_fragmentedDataToConnection.end()) {
		m_storedFragmentedDataSizeMb
			-= static_cast<double>(it->second.data.GetPayloadSize()) / Data::MB + Data::MAXIMUM_HEADER_MB;
		m_fragmentedDataTimerToConnection.erase(it->second.timestamp);
		m_fragmentedDataToConnection.erase(it);
	}
}

template <bool T> FORCE_INLINE [[nodiscard]] bool IHandler::PurgeStoredData()
{
	if constexpr (T == CHECK_BEFORE) {
		if (!Helper::FloatGreater(m_storedFragmentedDataSizeMb, m_storedFragmentedDataLimitMb)) {
			return true;
		}
	}

	auto it{ m_fragmentedDataTimerToConnection.begin() };
	while (it != m_fragmentedDataTimerToConnection.end()) {
		m_storedFragmentedDataSizeMb
			-= static_cast<double>(it->second->data.GetPayloadSize()) / Data::MB + Data::MAXIMUM_HEADER_MB;
		LOG_WARNING_NEW(
			"WebSocket fragmented data purged for connection {} due to limit exceed", it->second->connection);
		m_fragmentedDataToConnection.erase(it->second->connection);
		it = m_fragmentedDataTimerToConnection.erase(it);

		if (!Helper::FloatGreater(m_storedFragmentedDataSizeMb, m_storedFragmentedDataLimitMb)) {
			return true;
		}
	}

	return false;
}

FORCE_INLINE [[nodiscard]] bool IHandler::CheckLimitsForStored(
	const double additionalSizeMb, FragmentedData* const fragmentedDataPtr, const int connection)
{
	const auto storedFragmentSize{ static_cast<double>(fragmentedDataPtr->data.GetPayloadSize()) / Data::MB
		+ Data::MAXIMUM_HEADER_MB };
	// If fragment cannot be stored for sure, do not purge anything
	if (Helper::FloatGreater(additionalSizeMb + storedFragmentSize, m_storedFragmentedDataLimitMb)) {
		LOG_WARNING_NEW(
			"WebSocket income data is dropped and stored data is purged for connection {} due to limit exceed",
			connection);
		m_fragmentedDataToConnection.erase(connection);
		m_storedFragmentedDataSizeMb -= storedFragmentSize;
		return false;
	}

	m_storedFragmentedDataSizeMb += additionalSizeMb;
	if (Helper::FloatGreater(m_storedFragmentedDataSizeMb, m_storedFragmentedDataLimitMb)) {
		if (!PurgeStoredData<CHECK_USUAL>()) {
			m_fragmentedDataToConnection.erase(connection);
			m_storedFragmentedDataSizeMb -= storedFragmentSize + additionalSizeMb;
			return false;
		}
	}

	return true;
}

FORCE_INLINE [[nodiscard]] bool IHandler::CheckLimitForNew(const double additionalSizeMb, const int connection)
{
	if (Helper::FloatGreater(additionalSizeMb, m_storedFragmentedDataLimitMb)) [[unlikely]] {
		LOG_WARNING_NEW("WebSocket fragment from connection {} is skipped due to limit exceed", connection);
		return false;
	}

	m_storedFragmentedDataSizeMb += additionalSizeMb;
	if (Helper::FloatGreater(m_storedFragmentedDataSizeMb, m_storedFragmentedDataLimitMb)) {
		if (!PurgeStoredData<CHECK_USUAL>()) {
			m_fragmentedDataToConnection.erase(connection);
			m_storedFragmentedDataSizeMb -= additionalSizeMb;
			return false;
		}
	}

	return true;
}

} // namespace WebSocket

} // namespace Protocol

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_WEBSOCKET_INL