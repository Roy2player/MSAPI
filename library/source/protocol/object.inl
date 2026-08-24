/**************************
 * @file        object.inl
 * @version     6.0
 * @date        2023-08-29
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
 * @brief Works in paradigm of streams and filters. Stream has one custom filter and distributor must know how to react
 * on this filter. Filter can has multiple custom objects to filtration. Stream can be opened with different types:
 * snapshot - get all currently available objects and snapshot and live - get all currently available and all new
 * objects while stream is open. Stream has callbacks about states: opened, snapshot done, failed and object handle.
 * Client must set connection for stream to mark who is the distributor.
 *
 * @brief Header data is 32 bytes long, following data alignment constraints will be always meet for systems with max 16
 * bytes alignment.
 *
 * @brief Undefined state - default state, can be right after stream created;
 * @brief Pending state - stream is waiting for answer right after stream opened;
 * @brief Opened state - stream is in active state;
 * @brief Done state - stream is in active state and got snapshot of data. Stream remain opened state, but snapshot done
 * flag is set.
 * @brief Failed state - some errors on distributor side, action to reopen is required;
 * @brief Closed state - stream is closed by client or stream type is snapshot and snapshot is done - then stream saved
 * snapshot done flag;
 * @brief Removed state - stream is removed by client, state visible only for distributor.
 *
 * @brief Undefined issue - description for issue is not presented;
 * @brief Empty issue - when no issues occurred;
 * @brief Not unique filter issue - when distributor got filter definition for stream which already reserved it.
 * @brief Reserved filter object without filter issue - when distributor got filter object for stream which does not
 * have filter definition.
 * @brief Unknown filter object hash issue - when distributor got filter object with unknown hash.
 * @brief Unknown hash issue - when distributor got data with unknown hash.
 * @brief Bad variant access issue - when distributor got filter object for available filter, but it is not. Should not
 * be happened.
 * @brief Extra filter object issue - when distributor got more filter objects than expected.
 *
 * @brief Identifier of stream is unique for single application which created that stream. Distributor uses key pair {
 * stream id, connection } to identify stream.
 *
 * @todo Filters can be || and &&
 * @todo Stream can has different filters
 * @todo typeid.hash_code() should be replaced with custom hash function
 */

#ifndef MSAPI_PROTOCOL_OBJECT_INL
#define MSAPI_PROTOCOL_OBJECT_INL

#include "../help/log.h"
#include "dataHeader.h"
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sys/socket.h>
#include "../help/autoClearPtr.inl"
#include "../server/connection.inl"
#include "../server/application.h"

namespace MSAPI {

class Application;

namespace Protocol {

namespace Object {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

enum class Type : int8_t { Undefined, Snapshot, SnapshotAndLive, Max };

/**************************
 * @return Description of object protocol stream type enum.
 *
 * @todo Add unit test.
 */
FORCE_INLINE std::string_view EnumToString(const Type value) noexcept
{
	static_assert(U(Type::Max) == 3, "Absence of stream type enum transcription");

	switch (value) {
	case Type::Undefined:
		return "Undefined";
	case Type::Snapshot:
		return "Snapshot";
	case Type::SnapshotAndLive:
		return "Snapshot and live";
	case Type::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown stream type enum: {}", U(value));
		return "Unknown";
	}
}

enum class State : int8_t { Undefined, Pending, Opened, Done, Failed, Closed, Removed, Max };

/**************************
 * @return Description of object protocol stream state enum.
 *
 * @todo Add unit test.
 */
FORCE_INLINE std::string_view EnumToString(const State value) noexcept
{
	static_assert(U(State::Max) == 7, "Absence of stream state enum transcription");

	switch (value) {
	case State::Undefined:
		return "Undefined";
	case State::Pending:
		return "Pending";
	case State::Opened:
		return "Opened";
	case State::Done:
		return "Done";
	case State::Failed:
		return "Failed";
	case State::Closed:
		return "Closed";
	case State::Removed:
		return "Removed";
	case State::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown stream state enum {}", U(value));
		return "Unknown";
	}
}

enum class Issue : int8_t {
	Undefined,
	Empty,
	StreamIsAlreadyOpened,
	StreamDoesNotExist,
	FilterObjectHashMismatch,
	FilterNotFound,
	FilterSizeExceeded,
	UnknownHash,
	Max
};

/**************************
 * @return Description of object protocol stream issue enum.
 *
 * @todo Add unit test.
 */
FORCE_INLINE std::string_view EnumToString(const Issue value) noexcept
{
	static_assert(U(Issue::Max) == 8, "Absence of stream issue enum transcription");

	switch (value) {
	case Issue::Undefined:
		return "Undefined";
	case Issue::Empty:
		return "Empty";
	case Issue::StreamIsAlreadyOpened:
		return "Stream is already opened";
	case Issue::StreamDoesNotExist:
		return "Stream does not exist";
	case Issue::FilterObjectHashMismatch:
		return "Filter object hash mismatch";
	case Issue::FilterNotFound:
		return "Filter not found";
	case Issue::FilterSizeExceeded:
		return "Filter size exceeded";
	case Issue::UnknownHash:
		return "Unknown hash";
	case Issue::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown stream issue enum: {}", U(value));
		return "Unknown";
	}
}

/**************************
 * @brief Structure for provide stream state.
 *
 * @todo It is much better to provide string description on fail reason.
 */
struct StreamStateResponse {
	State state{ State::Undefined };
	Issue issue{ Issue::Empty };
};

/**************************
 * @brief General object for transferring data in stream.
 * Side: server, client.
 */
class Data : public DataHeader {
private:
	uint64_t m_objectHash;
	uint64_t m_streamId;

public:
	/**************************
	 * @brief Create object for transfering data in stream, update buffer size.
	 *
	 * @param streamId Stream id for which object is created. Stream id is not required if communication does not
	 * involve streams.
	 * @param objectHash Hash of object in data.
	 * @param size Size of object.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE Data(const uint64_t streamId, const uint64_t objectHash, const uint64_t size) noexcept
		: DataHeader{ 2666999999 }
		, m_objectHash{ objectHash }
		, m_streamId{ streamId }
	{
		m_bufferSize += sizeof(uint64_t) * 2 + size;
	}

	/**************************
	 * @brief Construct a new Data object from buffer, copy stream id and stream object hash from it.
	 *
	 * @attention Buffer must be at least 32 bytes long.
	 *
	 * @tparam T DataHeader.
	 *
	 * @param header Data header.
	 * @param buffer Buffer with data.
	 *
	 * @test Has unit test.
	 */
	template <typename T>
	FORCE_INLINE Data(T&& header, const std::span<const uint8_t> buffer) noexcept
		requires std::is_same_v<std::decay_t<T>, DataHeader>
		: DataHeader{ std::forward<T>(header) }
	{
		if (buffer.size() < sizeof(uint64_t) * 4) [[unlikely]] {
			m_streamId = 0;
			m_objectHash = 0;
			return;
		}

		const auto* data{ buffer.data() };
		memcpy(&m_streamId, data + sizeof(uint64_t) * 2, sizeof(uint64_t));
		memcpy(&m_objectHash, data + sizeof(uint64_t) * 3, sizeof(uint64_t));
	}

	/**************************
	 * @return Hash of object in data.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetObjectHash() const noexcept { return m_objectHash; }

	/**************************
	 * @return True if cipher is correct, buffer size, stream id and hash have not zero values.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept
	{
		return m_cipher == 2666999999 && m_bufferSize >= sizeof(uint64_t) * 4 && m_objectHash != 0
			&& m_streamId != 0;
	}

	/**************************
	 * @return Stream id for which object is related.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetStreamId() const noexcept { return m_streamId; }

	/**************************
	 * @brief Pack data before sending in stream.
	 *
	 * @attention Buffer should be freed after using.
	 *
	 * @param data Data for packing.
	 *
	 * @return Auto clear ptr with data for sending, can contain on allocation problems.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] AutoClearPtr<void> PackData(const void* data) const noexcept
	{
		void* const buffer{ malloc(m_bufferSize) };
		if (buffer == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Cannot allocate memory for packing data. Error №{}: {}", errno, std::strerror(errno));
			return { nullptr };
		}

		memcpy(buffer, &m_cipher, sizeof(uint64_t));
		memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t)], &m_bufferSize, sizeof(uint64_t));
		memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t) * 2], &m_streamId, sizeof(uint64_t));
		memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t) * 3], &m_objectHash, sizeof(uint64_t));
		memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t) * 4], data, m_bufferSize - sizeof(uint64_t) * 4);
		// Diagnostic::PrintBinaryDescriptor<Diagnostic::binary>(buffer, m_bufferSize, "Packed object data");
		return { buffer };
	}

	/**************************
	 * @brief Unpack data after receiving from stream.
	 *
	 * @param ptr Pointer to unpacked data.
	 * @param buffer Buffer with packed data.
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE static void UnpackData(const void** ptr, const void* buffer) noexcept
	{
		*ptr = &(static_cast<const char*>(buffer)[sizeof(uint64_t) * 4]);
	}

	//! Is that required?
	/**************************
	 * @test Has unit test.
	 */
	bool operator==(const Data& x) const noexcept = default;

	/**************************
	 * @test Has unit test.
	 */
	bool operator!=(const Data& x) const noexcept = default;

	/**************************
	 * @example Object protocol:
	 * {
	 * 			cipher	    : 2666999999
	 * 			buffer size	: 123
	 * 			object hash : 123456789
	 * 			stream id   : 123
	 * }
	 *
	 * @test Has unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const noexcept
	{
		return std::format("Object protocol:\n{{"
						   "\n\tcipher      : {}"
						   "\n\tbuffer size : {}"
						   "\n\tobject hash : {}"
						   "\n\tstream id   : {}"
						   "\n}}",
			m_cipher, m_bufferSize, m_objectHash, m_streamId);
	}
};

/**************************
 * @brief Polymorphic stream abstraction with basic fields.
 * Side: client.
 * Concurency safe: true.
 */
class StreamBase {
private:
	static inline std::atomic<uint64_t> m_streamCounter{};

protected:
	const uint64_t m_id;
	std::shared_ptr<Connection::Data> m_connectionData;
	Lock::AtomicRW m_lock;
	State m_state{ State::Undefined };
	bool m_isSnapshotDone{};

public:
	/**************************
	 * @brief Construct a new Stream Base object, generate unique id for stream.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE StreamBase() noexcept
		: m_id{ m_streamCounter.fetch_add(1, std::memory_order_relaxed) }
	{
	}

	/**************************
	 * @brief Default virtual destructor.
	 */
	virtual ~StreamBase() noexcept = default;

	/**************************
	 * @brief Set connection data related to stream.
	 *
	 * @param connectionData Related connection data.
	 *
	 * @locking Read lock inside.
	 *
	 * @todo Add unit test.
	 */
	void SetConnectionData(std::shared_ptr<Connection::Data> connectionData /* by value as moved */)
	{
		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
			m_connectionData = std::move(connectionData);
		}

		LOG_PROTOCOL_NEW(
			"Stream id: {} is now related with connection id: {}", m_id, connectionData->GetConnectionId());
	}

	/**************************
	 * @return Related connection data.
	 *
	 * @locking Read lock inside.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> GetConnectionData() noexcept
	{
		Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
		return m_connectionData;
	}

	/**************************
	 * @return True if stream snapshot is done.
	 *
	 * @locking Read lock inside.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool IsSnapshotDone() noexcept
	{
		Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
		return m_isSnapshotDone;
	}

	/**************************
	 * @return State of stream.
	 *
	 * @locking Read lock inside.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] State GetState() noexcept
	{
		Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
		return m_state;
	}

	/**************************
	 * @brief Set state of stream.
	 *
	 * @locking Write lock inside.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void SetState(const State state) noexcept
	{
		Lock::AtomicRW::Guard<Lock::write> _{ m_lock };
		m_state = state;
	}

	/**************************
	 * @return Stream id.
	 *
	 * @concurrency Lock is not required.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetId() const noexcept { return m_id; }

	/**************************
	 * @return True if stream connection is not set.
	 *
	 * @locking Read lock inside.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool Empty() noexcept
	{
		Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
		return m_connectionData.get() == nullptr;
	}
};

/**************************
 * @brief Stream key for both client and server side.
 * Side: client.
 */
struct StreamKey {
	struct Hash
	{
		[[nodiscard]] size_t operator()(const StreamKey& key) const noexcept
		{
			return key.streamId ^ (key.connectionId + 0x9e3779b97f4a7c15ULL + (key.streamId << 6) + (key.streamId >> 2));
		}
	};

	const uint64_t streamId;
	const uint64_t connectionId;

	/**************************
	 * @brief Construct stream key.
	 *
	 * @param streamId Id of stream.
	 * @param connectionId Id of connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE StreamKey(const uint64_t streamId, const uint64_t connectionId) noexcept
		: streamId{ streamId }
		, connectionId{ connectionId }
	{
	}

	[[nodiscard]] bool operator==(const StreamKey&) const noexcept = default;

	// StreamKey(const StreamKey& other) = default;
	// StreamKey(StreamKey&& other) = default;
	// StreamKey& operator=(const StreamKey& other) = default;
	// StreamKey& operator=(StreamKey&& other) = default;
};

/**************************
 * @brief Polymorphic virtual stream data handler abstraction. Stores all active streams and manage their specific and
 * common callbacks. Side: client.
 */
class IHandlerBase {
private:
	std::unordered_map<StreamKey, StreamBase*, StreamKey::Hash> m_streamKeyToStream;
	Lock::AtomicRW m_streamKeyToStreamLock;

public:
	/**************************
	 * @brief Destroy the IHandlerBase object.
	 */
	virtual ~IHandlerBase() noexcept = default;

	/**************************
	 * @brief Callback when stream opened successfully.
	 *
	 * @param streamId Stream id for which callback is called.
	 */
	virtual void HandleStreamOpened(uint64_t streamId) noexcept = 0;

	/**************************
	 * @brief Callback when stream got snapshot of data.
	 *
	 * @param streamId Stream id for which callback is called.
	 */
	virtual void HandleStreamSnapshotDone(uint64_t streamId) noexcept = 0;

	/**************************
	 * @brief Callback when any error occurred on distributor side, reopen action is required.
	 *
	 * @param streamId Stream id for which callback is called.
	 */
	//! How about to provide error string here? :)
	virtual void HandleStreamFailed(uint64_t streamId) noexcept = 0;

protected:
	/**************************
	 * @brief Read lock inside. Check if stream id is assigned to handler.
	 *
	 * @param streamKey Stream key to check.
	 *
	 * @return True if stream key is assigned to handler, false otherwise.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool HasStream(const StreamKey streamKey) noexcept
	{
		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_streamKeyToStreamLock };
			if (m_streamKeyToStream.contains(streamKey)) [[likely]] {
				return true;
			}
		}

		LOG_WARNING_NEW(
			"Stream:connection id {}:{} is not assigned to handler", streamKey.streamId, streamKey.connectionId);
		return false;
	}

	/**************************
	 * @brief Assign a stream to handler.
	 *
	 * @param stream Stream to be assigned.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void SetStream(StreamBase* const stream) noexcept
	{
		const auto streamId{ stream->GetId() };
		const auto connectionData{ stream->GetConnectionData() };

		if (connectionData == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Cannot assign stream id {} to handler, connection is not set", streamId);
			return;
		}

		const auto connectionId{ connectionData->GetConnectionId() };

		bool isSuccess [[gnu::uninitialized]];
		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamLock };
			isSuccess = m_streamKeyToStream.emplace(StreamKey{ streamId, connectionId }, stream).second;
		}

		if (!isSuccess) [[unlikely]] {
			LOG_ERROR_NEW("Stream:connection id {}:{} already assigned to handler", streamId, connectionId);
		}
	}

	/**************************
	 * @brief Unassign stream from handler.
	 *
	 * @param streamKey Key of stream to be unassigned.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void RemoveStream(const StreamKey& streamKey) noexcept
	{
		Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamLock };
		m_streamKeyToStream.erase(streamKey);
	}

	/**************************
	 * @brief Collect stream state.
	 *
	 * @attention If distributor gets StreamStateResponse you should call this function.
	 *
	 * @param streamKey Stream key for which state is collected.
	 * @param state Collected stream state.
	 *
	 * @todo Add unit test.
	 */
	//! Collect recv buffer and unpack it inside
	FORCE_INLINE void CollectStreamState(const StreamKey& streamKey, const StreamStateResponse* const state) noexcept
	{
		//! Here should be check on application running state
		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamLock };
			const auto it{ m_streamKeyToStream.find(streamKey) };
			if (it == m_streamKeyToStream.end()) {
				LOG_WARNING_NEW("State {} for unknown stream:connection id {}:{}", EnumToString(state->state),
					streamKey.streamId, streamKey.connectionId);
				return;
			}

			// Write lock inside
			it->second->SetState(state->state);
		}

		LOG_PROTOCOL_NEW("State {} for stream:connection id {}:{}", EnumToString(state->state), streamKey.streamId,
			streamKey.connectionId);
		switch (state->state) {
		case State::Opened:
			HandleStreamOpened(streamKey.streamId);
			return;
		case State::Done:
			HandleStreamSnapshotDone(streamKey.streamId);
			return;
		case State::Failed:
			HandleStreamFailed(streamKey.streamId);
			return;
		case State::Closed:
			return;
		default:
			LOG_ERROR_NEW("Unknown state {} for stream:connection id {}:{}", EnumToString(state->state),
				streamKey.streamId, streamKey.connectionId);
			return;
		}
	}
};

/**************************
 * @brief Specific object type handler.
 * Side: client.
 *
 * @tparam T Object type to be handled from stream.
 */
template <typename T>
	requires std::is_class_v<T>
class IHandler : virtual public IHandlerBase {
private:
	const Application* m_application;

public:
	/**************************
	 * @brief Construct a new IHandler object, empty constructor.
	 *
	 * @param application Readable pointer to application object.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE IHandler(const Application* const application) noexcept
		: m_application{ application }
	{
	}

	/**************************
	 * @brief Handler for objects from stream.
	 *
	 * @param streamId Stream id for which callback is called.
	 * @param object Handled object.
	 */
	virtual void HandleObject(uint64_t streamId, const T& object) noexcept = 0;

	/**************************
	 * @brief Call the handle object function after verifying that application is in running state and data is reserved
	 * for active stream.
	 *
	 * @note If reserved message from unknown stream id it will be rejected.
	 *
	 * @param data Control object data.
	 * @param connectionId Related connection id.
	 * @param object Object for collecting.
	 *
	 * @todo Add unit test.
	 */
	//! Collect recv buffer and unpack it inside
	FORCE_INLINE void Collect(const Data& data, const uint64_t connectionId, const void* object) noexcept
	{
		if (m_application->IsRunning()) [[unlikely]] {
			LOG_PROTOCOL_NEW(
				"Application state is not running. Connection id: {}, collect data: {}", connectionId, data.ToString());
			return;
		}

		LOG_PROTOCOL_NEW("Collect data: {}", data.ToString());

		const auto streamId{ data.GetStreamId() };

		if (!HasStream({ streamId, connectionId })) [[unlikely]] {
			return;
		}

		const auto hash{ data.GetObjectHash() };
		if (hash == typeid(T).hash_code()) { //! second check
			static_assert(sizeof(Data) % 16 == 0, "In buffer object alignment is incorrect");
			HandleObject(streamId, *reinterpret_cast<const T*>(object));
			return;
		}

		LOG_ERROR_NEW("Unknown hash: {}", hash);
	}
};

/**************************
 * @brief Polymorphic filter data abstraction, stores general data. Is supposed to be touched by one thread.
 * Side: server, client.
 *
 * @attention Filter object can't be removed from filter, need to create new filter object.
 */
class FilterBase {
private:
	uint64_t m_totalFilterSize{};
	uint64_t m_streamObjectHash{};
	Type m_type{ Type::Undefined };

public:
	/**************************
	 * @brief Construct a new empty Filter Base object, empty constructor.
	 *
	 * @param type Stream type to set.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE FilterBase(const Type type) noexcept
		: m_type(type)
	{
	}

	// /**************************
	//  * @brief Construct a new empty Filter Base object, empty constructor.
	//  *
	//  * @attention Type will be UndefinedType and should be set manually.
	//  */
	// FORCE_INLINE FilterBase() noexcept = default;

	/**************************
	 * @brief Destroy the Filter Base object, empty destructor.
	 */
	virtual ~FilterBase() noexcept = default;

	/**************************
	 * @return Total number of objects in filter.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetTotalFilterSize() const noexcept { return m_totalFilterSize; }

	/**************************
	 * @return True if filter is empty, means - filter has not objects.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool Empty() const noexcept { return m_totalFilterSize == 0; }

	/**************************
	 * @return Type of stream.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] Type GetType() const noexcept { return m_type; }

	/**************************
	 * @brief Set hash of stream object.
	 *
	 * @param streamObjectHash Hash to set.
	 *
	 * @todo Should be called automatically after creating specific filter. Because filter didn't know which objects
	 * stream will send.
	 * @todo Add unit test.
	 */
	FORCE_INLINE void SetStreamObjectHash(const uint64_t streamObjectHash) noexcept
	{
		m_streamObjectHash = streamObjectHash;
	}

	/**************************
	 * @return Hash of stream object.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetStreamObjectHash() const noexcept { return m_streamObjectHash; }

	/**************************
	 * @return Hash of filter object.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] virtual uint64_t GetFilterObjectHash() const noexcept //! = 0 (must be pure)
	{
		LOG_ERROR("Pure method is called");
		return 0;
	}

	/**************************
	 * @example Filter base:
	 * {
	 * 			type	           : Snapshot
	 * 			stream object hash : 123456789
	 * 			total filter size  : 3
	 * }
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const noexcept
	{
		return std::format("Filter base:\n{{"
						   "\n\ttype               : {}"
						   "\n\tstream object hash : {}"
						   "\n\ttotal filter size  : {}"
						   "\n}}",
			EnumToString(m_type), m_streamObjectHash, m_totalFilterSize);
	}

protected:
	/**************************
	 * @brief Set the total filter size.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void SetTotalFilterSize(const uint64_t size) noexcept { m_totalFilterSize = size; }
};

/**************************
 * @brief Specific object type filter, includes filter object hash and container with filter objects.
 * Side: client.
 *
 * @attention Filter object can't be removed from filter, need to create new filter object.
 *
 * @tparam T Type of filter object.
 *
 * @todo Need support for multiple filters types for one stream with different logical operations.
 */
template <typename T>
	requires std::is_class_v<T>
class Filter : public FilterBase {
private:
	const uint64_t m_filterObjecthash{ typeid(T).hash_code() };
	std::vector<T> m_objects;

public:
	/**************************
	 * @brief Construct a new Filter object.
	 *
	 * @param type Type of stream.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Filter(const Type type) noexcept
		: FilterBase{ type }
	{
	}

	/**************************
	 * @brief Default construct a new Filter object.
	 *
	 * @attention Type will be UndefinedType and should be set manually.
	 */
	FORCE_INLINE Filter() noexcept = default;

	/**************************
	 * @brief Construct a new Filter object from base filter.
	 *
	 * @param filter Base filter.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Filter(FilterBase filter) noexcept
		: FilterBase{ std::move(filter) }
	{
	}

	/**************************
	 * @brief Set the Filter Object.
	 *
	 * @param object Filter object to be set.
	 *
	 * @return Size of filter objects after adding new one.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t SetObject(T&& object) noexcept
	{
		m_objects.emplace_back(std::forward<T>(object));
		return m_objects.size();
	}

	/**************************
	 * @return Readable link for container with filter objects.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] const std::vector<T>& GetObjects() const noexcept { return m_objects; }

	/**************************
	 * @return Hash of filter object.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetFilterObjectHash() const noexcept final { return m_filterObjecthash; }

	/**************************
	 * @example Filter special:
	 * {
	 * 			filter object hash : 123456789
	 * 			filter size        : 3
	 * 							   : Filter base:
	 * {
	 * ...
	 * }
	 * }
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const noexcept
	{
		return std::format("Filter special:\n{"
						   "\n\tfilter object hash : {}"
						   "\n\ttotal filter size  : {}"
						   "\n\t                   : {}"
						   "\n}",
			m_filterObjecthash, m_objects.size(), FilterBase::ToString());
	}

	//! For what?
	template <typename T1, typename F>
		requires std::is_class_v<T1> && std::is_class_v<F>
	friend class Stream;

	template <typename... Ts>
		requires(std::is_class_v<Ts> && ...)
	friend class Distributor;
};

/**************************
 * @brief Send object for particular stream.
 *
 * @param connection Connection for sending.
 * @param data Data for sending.
 * @param object Object for sending.
 *
 * @return True if data is sent successfully, false if any error occurred and connection is no longer usable.
 *
 * @todo Add unit test.
 */
FORCE_INLINE [[nodiscard]] bool Send(Connection& connection, const Data& data, const void* object) noexcept
{
	LOG_PROTOCOL_NEW("Send data: {}, to connection id: ", data.ToString(), connection.GetId());
	const auto packData{ data.PackData(object) };

	if (packData.Get() == nullptr) [[unlikely]] {
		return false;
	}

	return connection.Send(packData.Get(), data.GetBufferSize(), MSG_NOSIGNAL) != 0;
}

/**************************
 * @brief Class for specific object stream, contains handler for callbacks and filter for stream.
 * Concurency safe: true.
 * Side: client.
 *
 * @tparam T Type of stream object.
 * @tparam F Type of stream filter object.
 */
template <typename T, typename F>
	requires std::is_class_v<T> && std::is_class_v<F>
class Stream : public StreamBase {
private:
	IHandler<T>& m_handler;
	std::optional<Filter<F>> m_filter;
	Lock::AtomicRW m_filterLock;

public:
	/**************************
	 * @brief Construct a new Stream object, set stream id in handler for stream state callbacks.
	 *
	 * @param handler Handler for callbacks.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] Stream(IHandler<T>& handler) noexcept
		: m_handler{ handler }
	{
		m_handler.SetStream(m_id, this);
		LOG_PROTOCOL_NEW("Client creates stream, id {}", m_id);
	}

	/**************************
	 * @brief Destroy the Stream object, remove stream id from handler and call Close() function.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE ~Stream() noexcept
	{
		Lock::AtomicRW::Guard<Lock::write> _{ m_lock };
		if (m_state == State::Failed || m_state == State::Closed || m_state == State::Undefined) {
			return;
		}

		LOG_DEBUG_NEW("Client removes stream id {} with state {}", m_id, EnumToString(m_state));

		m_handler.RemoveStream(m_id);

		if (m_connectionData.get() == nullptr) [[unlikely]] {
			return;
		}

		StreamStateResponse state{ State::Removed };
		(void)Send(m_connectionData->GetConnection(),
			{ m_id, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &state);
	}

	/**************************
	 * @brief Set the new filter and clear snapshot done flag, close stream if it is active.
	 * @Locking Write lock stream base and stream.
	 *
	 * @tparam F1 Type of filter object.
	 *
	 * @param filter Filter to set.
	 *
	 * @todo Add unit test.
	 */
	template <typename F1>
		requires std::is_same_v<decltype(std::decay_t<F1>()), Filter<F>>
	FORCE_INLINE void SetFilter(F1&& filter) noexcept
	{
		Lock::AtomicRW::Guard<Lock::write> _{ m_lock };

		if (m_connectionData.get() == nullptr) [[unlikely]] {
			return;
		}

		if (m_state == State::Opened || m_state == State::Pending) {
			CloseImpl();
		}
		else {
			m_isSnapshotDone = false;
		}

		Lock::AtomicRW::Guard<Lock::write> _{ m_filterLock };

		m_filter = std::forward<F1>(filter);
		m_filter.SetStreamObjectHash(typeid(T).hash_code());
		LOG_PROTOCOL_NEW("Client sets filter {} for stream id {}", m_filter.ToString(), m_id);
	}

	/**************************
	 * @brief Open stream if it is closed: failed or undefined state. Required to set distributor connection.
	 * @locking Read lock stream base and stream filter.
	 *
	 * @return True if stream open is sent to distributor, false if any error occurred.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool Open() noexcept
	{
		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_lock };

			if (m_connectionData.get() == nullptr) [[unlikely]] {
				LOG_DEBUG_NEW("Client tries to open stream id {} without connection", m_id);
				return false;
			}

			Lock::AtomicRW::Guard<Lock::read> _{ m_filterLock };

			if (!m_filter.has_value()) [[unlikely]] {
				LOG_DEBUG_NEW("Client tries to open stream id {} without filter", m_id);
				return false;
			}

			if (m_state == State::Opened || m_state == State::Pending) [[unlikely]] {
				LOG_DEBUG_NEW("Stream id {} is already in active state {}", m_id, EnumToString(m_state));
				return false;
			}

			m_state = State::Pending;
			LOG_PROTOCOL_NEW("Client opens stream id {} with filter {}", m_id, m_filter.ToString());

			auto& connection{ m_connectionData->GetConnection() };

			do {
				if (!Send(connection, { m_id, typeid(Filter<F>).hash_code(), sizeof(FilterBase) },
						static_cast<FilterBase*>(&m_filter))) [[unlikely]] {
					break;
				}

				const Data data{ m_id, typeid(F).hash_code(), sizeof(F) };
				for (const auto& item : m_filter.GetObjects()) {
					if (!Send(connection, data, &item)) [[unlikely]] {
						break;
					}
				}
			} while (false);

			return true;
		}

		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_lock };
			CloseImpl();
		}

		return false;
	}

	/**************************
	 * @brief Close stream if it is active, clear snapshot done flag and set Closed state.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Close() noexcept
	{
		Lock::AtomicRW::Guard<Lock::write> _{ m_lock };

		if (m_connectionData.get() == nullptr) [[unlikely]] {
			LOG_DEBUG_NEW("Client tries to close stream id {} without connection", m_id);
			return;
		}

		if (m_state != State::Opened && m_state != State::Pending) [[unlikely]] {
			LOG_DEBUG_NEW("Reject attempt to close stream id {} with state {}", m_id, EnumToString(m_state));
			return;
		}

		CloseImpl();
	}

private:
	/**************************
	 * @brief Close stream without checking state, clear snapshot done flag and set Closed state.
	 *
	 * @pre Connection is set, stream is in active state.
	 *
	 * @locking Expected to be called with write stream base lock.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void CloseImpl() noexcept
	{
		LOG_PROTOCOL_NEW("Client closes stream id {}", m_id);
		// contract_assert(m_connectionData.get() != nullptr, std::format("Connection is not set for stream id {}",
		// m_id)); contract_assert(m_state == State::Opened || m_state == State::Pending, 	std::format("Stream state {}
		// is not active for stream id {}", EnumToString(m_state), m_id));

		m_isSnapshotDone = false;
		m_state = State::Closed;
		(void)Send(m_connectionData->GetConnection(),
			{ m_id, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &m_state);
	}
};

/**************************
 * @brief Contains data about streams and their filters.
 *
 * Distributor is responsible for sending objects to active streams. Only simple copyable objects are allowed to be sent
 * in stream, because they are sent in binary format.
 *
 * @attention To force closing active streams - call Stop() function.
 *
 * Side: server.
 *
 * @tparam Ts Types of filters which distributor can handle.
 *
 * @todo Currently only one filter type is supported for one stream, need to support multiple filters with different
 * logical operations.
 */
template <typename... Ts>
	requires(std::is_class_v<Ts> && ...)
class Distributor
{
private:
	class Streams;

	/**************************
	 * @brief Stream related data structure.
	 *
	 * @concurrency Yes, external locking is required.
	 */
	class StreamData {
	private:
		std::variant<std::monostate, Filter<Ts>...> m_filter;
		Lock::AtomicRW m_lock;
		const std::shared_ptr<Connection::Data> m_connectionData;
		const std::shared_ptr<Streams> m_streams;
		const uint64_t m_streamId;
		const uint64_t m_streamObjectHash;
		const uint64_t m_filterObjectHash;
		const uint64_t m_totalFiltersSize;
		const Type m_type;
		bool m_isActive;

	public:
		/**************************
		 * @brief Construct new stream data object. Mark as opened if filter size is equal to zero.
		 *
		 * @param connectionData Connection data structure associated with stream.
		 * @param streams Streams data which contains this stream data.
		 * @param streamId Stream id.
		 * @param streamObjectHash Stream object hash.
		 * @param filterObjectHash Filter object hash.
		 * @param totalFiltersSize Total number of object in filter.
		 * @param type Stream type.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE StreamData(std::shared_ptr<Connection::Data> connectionData /* by value as moved */,
			std::shared_ptr<Streams> streams /* by value as moved */, const uint64_t streamId,
			const uint64_t streamObjectHash, const uint64_t filterObjectHash, const uint64_t totalFiltersSize,
			const Type type) noexcept
			: m_connectionData{ std::move(connectionData) }
			, m_streams{ std::move(streams) }
			, m_streamId{ streamId }
			, m_streamObjectHash{ streamObjectHash }
			, m_filterObjectHash{ filterObjectHash }
			, m_totalFiltersSize{ totalFiltersSize }
			, m_type{ type }
			, m_isActive{ totalFiltersSize == 0 }
		{
		}

		/**************************
		 * @return Hash of stream object.
		 *
		 * @locking Not required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetStreamObjectHash() const noexcept { return m_streamObjectHash; }

		/**************************
		 * @return Hash of filter object.
		 *
		 * @locking Not required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetFilterObjectHash() const noexcept { return m_filterObjectHash; }

		/**************************
		 * @return Connection data structure associated with stream.
		 *
		 * @locking Not required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] Connection& GetConnection() const noexcept
		{
			return m_connectionData->GetConnection();
		}

	private:
		/**************************
		 * @brief Add new filter to stream.
		 *
		 * @locking External write lock is required.
		 *
		 * @todo Add unit test.
		 */
		template <typename T>
			requires is_included_in<T, Ts...>
		FORCE_INLINE void SetFilter(const Filter<T>* const filter) noexcept
		{
			m_filter = std::move(*filter);
		}

		[[nodiscard]] FORCE_INLINE const FilterBase* GetFilter() const noexcept
		{
			if (std::holds_alternative<std::monostate>(m_filter)) [[unlikely]] {
				return nullptr;
			}

			return std::visit([](const auto& filter) -> const FilterBase* { return &filter; }, m_filter);
		}

		/**************************
		 * @brief Set filter object for stream.
		 *
		 * @locking External write lock is required.
		 *
		 * @return Undefined issue if filter object is set successfully, otherwise return error issue.
		 *
		 * @todo Add unit test.
		 */
		template <typename T>
			requires is_included_in<T, Ts...>
		FORCE_INLINE [[nodiscard]] Issue SetFilterObject(const T* const object) noexcept
		{
			auto* const filterPtr{ std::get_if<Filter<T>>(&m_filter) };
			if (filterPtr == nullptr) [[unlikely]] {
				if (m_filterObjectHash != typeid(T).hash_code()) [[unlikely]] {
					LOG_ERROR_NEW(
						"Filter object hash: {} is not set for stream id: {} as it is different from expected hash: {}",
						typeid(T).hash_code(), m_streamId, m_filterObjectHash);
					return Issue::FilterObjectHashMismatch;
				}

				LOG_ERROR_NEW("Filter to be updated with object hash: {} is not found. Stream id: {} ",
					typeid(T).hash_code(), m_streamId);
				return Issue::FilterNotFound;
			}

			const auto currentSize{ filterPtr->SetObject(*static_cast<const T*>(object)) };

			if (currentSize == m_totalFiltersSize) {
				m_isActive = true;
				return Issue::Undefined;
			}

			if (currentSize < m_totalFiltersSize) {
				return Issue::Undefined;
			}

			return Issue::FilterSizeExceeded;
		}

		/**************************
		 * @return Streams data which contains this stream data.
		 *
		 * @locking Not required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] Streams& GetStreams() const noexcept { return m_streams->get(); }

		/**************************
		 * @return True if all filters are received, false otherwise.
		 *
		 * @locking External read lock is required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool IsActive() const noexcept { return m_isActive; }

		/**************************
		 * @brief Mark stream as inactive.
		 *
		 * @locking External write lock is required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void UnsetActive() noexcept { m_isActive = false; }

		/**************************
		 * @locking External read lock is required.
		 *
		 * @example Stream data:
		 * {
		 * 			stream id           : 123
		 * 			connection id       : 74
		 * 			type                : Snapshot
		 * 			filter object hash  : 123456789
		 * 			total filter size   : 3
		 *			is active           : true
		 * }
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE std::string ToString() const noexcept
		{
			return std::format("Stream data:\n{{"
							   "\n\tstream id           : {}"
							   "\n\tconnection id       : {}"
							   "\n\ttype                : {}"
							   "\n\tstream object hash  : {}"
							   "\n\tfilter object hash  : {}"
							   "\n\ttotal filter size   : {}"
							   "\n\tis active           : {}"
							   "\n}}",
				m_streamId, m_connectionData->GetConnectionId(), EnumToString(m_type), m_streamObjectHash,
				m_filterObjectHash, m_totalFiltersSize, m_isActive);
		}
	};

	/**************************
	 * @brief Stream key for both client and server side.
	 * Side: server.
	 */
	struct StreamKey {
		const uint64_t streamId;
		const uint64_t connectionId;
	};

	/**************************
	 * @brief Contains all streams data for particular stream object hash.
	 *
	 * @concurrency ??? //!
	 */
	class Streams {
	private:
		std::unordered_map<StreamKey, std::shared_ptr<StreamData>> m_streamKeyToStreamData;
		Lock::AtomicRW m_streamKeyToStreamDataLock;

	public:
		// FORCE_INLINE Streams() noexcept = default;

		/**************************
		 * @brief Remove stream from streams map.
		 *
		 * @param key Stream key to remove.
		 *
		 * @locking Write lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void RemoveStream(const StreamKey key) noexcept
		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
			m_streamKeyToStreamData.erase(key);
		}

		/**************************
		 * @brief Add stream to streams map.
		 *
		 * @param key Stream key to add.
		 * @param streamData Stream data to add.
		 *
		 * @locking Write lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void AddSteam(
			StreamKey key, std::shared_ptr<StreamData> streamData /* by value as moved */) noexcept
		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
			m_streamKeyToStreamData.emplace(std::move(key), std::move(streamData));
		}

		/**************************
		 * @return Internal lock for streams map.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] Lock::AtomicRW& GetLock() noexcept { return m_streamKeyToStreamDataLock; }

		/**************************
		 * @return Readable link for streams map.
		 *
		 * @locking External locking is required.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] std::unordered_map<StreamKey, std::shared_ptr<StreamData>>&
		GetStreams() const noexcept
		{
			return m_streamKeyToStreamData;
		}
	};

private:
	const Application* m_application;
	std::unordered_map<StreamKey, std::shared_ptr<StreamData>> m_streamKeyToStreamData;
	Lock::AtomicRW m_streamKeyToStreamDataLock;
	std::unordered_map<uint64_t, std::shared_ptr<Streams>> m_streamObjectHashToStreams;
	Lock::AtomicRW m_streamObjectHashToStreamsLock;

public:
	/**************************
	 * @brief Construct distributor.
	 * 
	 * @param application Readable pointer to application object.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Distributor(const Application* const application) noexcept
		: m_application{ application }
	{}

	/**************************
	 * @brief Default destructor, call Stop() inside.
	 *
	 * @todo Add unit test.
	 */
	virtual ~Distributor() noexcept { Stop(); }

	/**************************
	 * @brief Send Failed state for all opened streams and remove all information about them.
	 *
	 * @locking Write lock for stream key to stream data, write lock for each stream data and write lock for stream
	 * object hash to streams.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Stop() noexcept
	{
		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
			auto begin{ m_streamKeyToStreamData.begin() };
			const auto end{ m_streamKeyToStreamData.end() };
			if (begin == end) {
				return;
			}

			LOG_PROTOCOL("Distributor starts removing information about streams");
			const StreamStateResponse state{ State::Failed };
			const auto hash{ typeid(StreamStateResponse).hash_code() };
			const auto size{ sizeof(StreamStateResponse) };

			std::shared_ptr<StreamData> streamData;
			while (true) {
				streamData = begin->second;

				Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };

				if (streamData->IsActive()) {
					streamData->UnsetActive();
					(void)Send(*(begin->second->GetConnection()), Data{ begin->first.streamId, hash, size }, &state);
				}

				begin = m_streamKeyToStreamData.erase(begin);
				if (begin == end) {
					return;
				}
			}
		}

		Lock::AtomicRW::Guard<Lock::write> _{ m_streamObjectHashToStreamsLock };
		m_streamObjectHashToStreams.clear();
	}

	/**************************
	 * @brief Apply action for stream from client side. Only Closed state is expected from client, all other states will
	 * be logged as warning.
	 *
	 * @param streamId Stream id for which action is applied.
	 * @param connectionId Connection id for which action is applied.
	 * @param response Income stream state response from client.
	 *
	 * @locking Write lock for stream key to stream data.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void StreamExternalAction(
		const uint64_t streamId, const uint64_t connectionId, const StreamStateResponse* const response) noexcept
	{
		StreamKey streamKey{ streamId, connectionId };
		switch (response->state) {
		case State::Closed: {
			LOG_PROTOCOL_NEW("Client closed stream, id: {}, connection id: {}", streamId, connectionId);

			std::shared_ptr<StreamData> streamData;
			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
				const auto it{ m_streamKeyToStreamData.find(streamKey) };
				if (it != m_streamKeyToStreamData.end()) [[likely]] {
					streamData = it->second;
					m_streamKeyToStreamData.erase(it);
				}
			}

			if (streamData == nullptr) {
				LOG_ERROR_NEW("Have not stream data for stream id: {}, connection id: {}", streamId, connectionId);
				return;
			}

			// Locks inside
			streamData->GetStreams().RemoveStream(streamKey);
		}
			return;
		default:
			LOG_WARNING_NEW("Unexpected stream state for external action: {}, stream id: {}, connection id: {}",
				EnumToString(response->state), streamId, connectionId);
			return;
		}
	}

	/**************************
	 * @brief Collecting stream data, it can be filter or its object. When filter is received, distributor will wait for
	 * all filter objects and then open stream or open stream instantly if filter size is equal to zero. When filter
	 * object is received, distributor will check if all filter objects are received and then open stream.
	 *
	 * @param connectionData Data structure of connection for which data is collected.
	 * @param data Data for collect.
	 * @param object Object for collect.
	 *
	 * @tparam T Type of filter object which presented in Ts.
	 *
	 * @locking Filter path: write locks m_streamObjectHashToStreamsLock and m_streamKeyToStreamDataLock on getting data
	 * and write locks of StreamData on filter setting. Filter object path: read lock m_streamKeyToStreamDataLock on
	 * getting data and write lock of StreamData on setting stream object.
	 *
	 * @todo Need to add supporting multiple filters for one stream. For that each filter must have its own identifier.
	 * @todo Add unit test.
	 * @todo Now it is intended runtime hash verifying before calling that function. Function do that again and
	 * moreover, when filter is accessed from variant, it is checked again. That must be resolved in one call with
	 * internal hash verifying to make API cleaner.
	 */
	template <typename T>
		requires is_included_in<T, Ts...>
	void Collect(const std::shared_ptr<Connection::Data>& connectionData, const Data& data, const void* object)
	{
		StreamKey streamKey{ data.GetStreamId(), connectionData->GetConnectionId() };

		if (!m_application->IsRunning()) [[unlikely]] {
			LOG_PROTOCOL_NEW("Application state is not Running, collect data: {}, connection id: {}", data.ToString(),
				streamKey.connectionId);
			return;
		}

		LOG_PROTOCOL_NEW("Collect data: {}, connection id: {}", data.ToString(), streamKey.connectionId);

		const auto dataHash{ data.GetObjectHash() };
		if (typeid(Filter<T>).hash_code() == dataHash) {
			const FilterBase* const filter{ reinterpret_cast<const FilterBase*>(object) };
			const auto filterObjectHash{ typeid(T).hash_code() };
			const auto streamObjectHash{ filter->GetStreamObjectHash() };

			std::shared_ptr<Streams> streams;
			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_streamObjectHashToStreamsLock };
				const auto it{ m_streamObjectHashToStreams.find(streamObjectHash) };
				if (it != m_streamObjectHashToStreams.end()) [[likely]] {
					streams = it->second;
				}
				else {
					streams = std::make_shared<Streams>();
					m_streamObjectHashToStreams.emplace(streamObjectHash, streams);
				}
			}

			const auto streamType{ filter->GetType() };
			const auto streamData{ std::make_shared<StreamData>(connectionData, streams, streamKey.streamId,
				streamObjectHash, filterObjectHash, filter->GetTotalFilterSize(), streamType) };

			if (streamType == Type::SnapshotAndLive) {
				std::pair<bool, typename std::decay_t<decltype(m_streamKeyToStreamData)>::iterator> result;
				{
					Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
					result = m_streamKeyToStreamData.emplace(streamKey, streamData);
				}

				if (!result.first) [[unlikely]] {
					LOG_ERROR_NEW("Stream id: {} is already opened for connection id: {}, {}", streamKey.streamId,
						streamKey.connectionId, filter->ToString());
					SendFailed(streamData, Issue::StreamIsAlreadyOpened);
					return;
				}

				// Locks inside
				streams->AddSteam(streamKey, streamData);

				{
					Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };
					streamData->template SetFilter<T>(static_cast<const Filter<T>*>(filter));
					LOG_PROTOCOL_NEW("Waiting filter's objects for new stream, {}", streamData->ToString());
				}
				return;
			}

			{
				Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };
				streamData->template SetFilter<T>(static_cast<const Filter<T>*>(filter));
				LOG_PROTOCOL_NEW("Instantly open, {}", streamData->ToString());
			}

			StreamStateResponse response{ State::Opened };
			const Data dataResponse{ streamKey.streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

			auto& connection{ connectionData->GetConnection() };

			if (!Send(connection, dataResponse, &response)) [[unlikely]] {
				return;
			}

			HandleNewStreamOpened(streamData->get());

			response.state = State::Done;
			if (Send(connection, dataResponse, &response)) [[unlikely]] {
				return;
			}

			response.state = State::Closed;
			if (Send(connection, dataResponse, &response)) [[unlikely]] {
				return;
			}
		}

		std::shared_ptr<StreamData> streamData;
		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_streamKeyToStreamDataLock };
			const auto it{ m_streamKeyToStreamData.find(streamKey) };
			if (it != m_streamKeyToStreamData.end()) [[likely]] {
				streamData = it->second;
			}
		}

		if (streamData == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Have not stream data for stream id: {}, connection id: {}", streamKey.streamId,
				streamKey.connectionId);

			const StreamStateResponse response{ State::Failed, Issue::StreamDoesNotExist };
			const Data dataResponse{ streamKey.streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

			(void)Send(connectionData->GetConnection(), dataResponse, &response);
			return;
		}

		if (typeid(T).hash_code() == dataHash) [[likely]] {
			Streams* streams [[gnu::uninitialized]];
			Issue issue [[gnu::uninitialized]];
			bool isActive [[gnu::uninitialized]];
			{
				Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };
				issue = streamData->template SetFilterObject<T>(static_cast<const T*>(object));
				streams = &(streamData->GetStreams());
				isActive = streamData->IsActive();
			}

			if (issue != Issue::Undefined) [[unlikely]] {
				// Locks inside
				streams->RemoveStream(streamKey);

				{
					Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
					m_streamKeyToStreamData.erase(streamKey);
				}

				const StreamStateResponse response{ State::Failed, issue };
				const Data dataResponse{ streamKey.streamId, typeid(StreamStateResponse).hash_code(),
					sizeof(StreamStateResponse) };

				(void)Send(connectionData->GetConnection(), dataResponse, &response);
				return;
			}

			if (isActive) {
				LOG_PROTOCOL_NEW("Got last filter's object and open stream id: {}, connection id: {}",
					streamKey.streamId, streamKey.connectionId);

				StreamStateResponse response{ State::Opened };
				const Data dataResponse{ streamKey.streamId, typeid(StreamStateResponse).hash_code(),
					sizeof(StreamStateResponse) };

				if (!Send(connectionData->GetConnection(), dataResponse, &response)) [[unlikely]] {
					// Locks inside
					streams->RemoveStream(streamKey);

					{
						Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
						m_streamKeyToStreamData.erase(streamKey);
					}
					return;
				}

				HandleNewStreamOpened(streamData->get());

				response.state = State::Done;
				if (!Send(connectionData->GetConnection(), dataResponse, &response)) [[unlikely]] {
					// Locks inside
					streams->RemoveStream(streamKey);

					{
						Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
						m_streamKeyToStreamData.erase(streamKey);
					}
					return;
				}

				return;
			}

			LOG_PROTOCOL_NEW(
				"Got filter's object for stream id: {}, connection id: {}", streamKey.streamId, streamKey.connectionId);
			return;
		}

		LOG_ERROR_NEW("Unknown hash has been reserved: {}, stream id: {}, connection id: {}", dataHash, streamKey.streamId,
			streamKey.connectionId);

		{
			Lock::AtomicRW::Guard<Lock::read> _{ streamData->GetLock() };
			// Locks inside
			streamData->GetStreams().RemoveStream(streamKey);
		}

		{
			Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
			m_streamKeyToStreamData.erase(streamKey);
		}

		const StreamStateResponse response{ State::Failed, Issue::UnknownHash };
		const Data dataResponse{ streamKey.streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

		(void)Send(connectionData->GetConnection(), dataResponse, &response);
	}

	/**************************
	 * @brief Send objects to particular stream.
	 *
	 * Stream is deleted in case:
	 * - Stream is active but not have filter.
	 * - Any send is not succeeded.
	 *
	 * @param streamData Stream data structure.
	 * @param objects Objects to send. Size is checked at the beginning of the call.
	 * @param filterPredicate Predicate for filter. Cannot be nullptr.
	 *
	 * @tparam T Type of container.
	 * @tparam S Type of object.
	 *
	 * @locking Read lock of the stream data on active and filter checking.
	 *
	 * @return True of success or no error case, false on error and stream is deleted.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 * @todo Add unit test.
	 */
	template <template <typename> typename T, typename S>
		requires std::is_class_v<S>
	[[nodiscard]] bool SendObjectsToStream(const StreamData& streamData, const T<S>& objects,
		const std::function<bool(const FilterBase* filter, const S& object)>& filterPredicate) noexcept
	{
		const auto objectsSize{ objects.size() };
		if (objectsSize == 0) [[unlikely]] {
			return true;
		}

		const FilterBase* filter [[gnu::uninitialized]];
		auto& connection{ streamData.GetConnection() };
		Streams* streams [[gnu::uninitialized]];
		{
			Lock::AtomicRW::Guard<Lock::read> _{ streamData.GetLock() };
			if (!streamData.IsActive()) [[unlikely]] {
				LOG_PROTOCOL_NEW(
					"Stream id: {} is not active. Connection id: {}", streamData.GetStreamId(), connection->GetId());
				return true;
			}

			filter = streamData.GetFilter();
			streams = &(streamData.GetStreams());
		}

		if (filter == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Filter is not set for stream id: {}, connection id: {}", streamData.GetStreamId(),
				connection->GetId());

			const StreamKey streamKey{ streamData.GetStreamId(), connection->GetId() };
			// Locks inside
			streams->RemoveStream(streamKey);

			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
				m_streamKeyToStreamData.erase(streamKey);
			}

			return false;
		}

		const Data data{ streamData.GetStreamId(), typeid(S).hash_code(), sizeof(S) };
		LOG_PROTOCOL_NEW("Try to send {} objects to stream id: {} connection id: {}", objectsSize, data.GetStreamId(),
			connection->GetId());

		for (const auto& object : objects) {
			if (filterPredicate(filter, object) && !Send(connection, data, &object)) [[unlikely]] {
				const StreamKey streamKey{ streamData.GetStreamId(), connection->GetId() };
				// Locks inside
				streams->RemoveStream(streamKey);

				{
					Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
					m_streamKeyToStreamData.erase(streamKey);
				}

				return false;
			}
		}

		return true;
	}

	/**************************
	 * @brief Send object to particular stream.
	 *
	 * Stream is deleted in case:
	 * - Stream is active but not have filter.
	 * - Send is not succeeded.
	 *
	 * @param streamData Stream data structure.
	 * @param object Object to send.
	 * @param filterPredicate Predicate for filter. Cannot be nullptr.
	 *
	 * @tparam T Type of object.
	 *
	 * @locking Read lock of the stream data on active and filter checking.
	 *
	 * @return True if object was sent, false on error and stream is deleted.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename T>
		requires std::is_class_v<T>
	[[nodiscard]] bool SendObjectToStream(const StreamData& streamData, const T& object,
		const std::function<bool(const FilterBase* filter, const T& object)>& filterPredicate) const
	{
		const FilterBase* filter [[gnu::uninitialized]];
		auto& connection{ streamData.GetConnection() };
		Streams* streams [[gnu::uninitialized]];
		{
			Lock::AtomicRW::Guard<Lock::read> _{ streamData.GetLock() };
			if (!streamData.IsActive()) [[unlikely]] {
				LOG_PROTOCOL_NEW(
					"Stream id: {} is not active. Connection id: {}", streamData.GetStreamId(), connection->GetId());
				return true;
			}

			filter = streamData.GetFilter();
			streams = &(streamData.GetStreams());
		}

		if (filter == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Filter is not set for stream id: {}, connection id: {}", streamData.GetStreamId(),
				connection->GetId());

			const StreamKey streamKey{ streamData.GetStreamId(), connection->GetId() };
			// Locks inside
			streams->RemoveStream(streamKey);

			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
				m_streamKeyToStreamData.erase(streamKey);
			}

			return false;
		}

		LOG_PROTOCOL_NEW(
			"Try to send object to stream id: {} connection id: {}", streamData.GetStreamId(), connection->GetId());

		if (filterPredicate(filter, object)
			&& !Send(connection, { streamData.GetStreamId(), typeid(T).hash_code(), sizeof(T) }, &object))
			[[unlikely]] {
			const StreamKey streamKey{ streamData.GetStreamId(), connection->GetId() };
			// Locks inside
			streams->RemoveStream(streamKey);

			{
				Lock::AtomicRW::Guard<Lock::write> _{ m_streamKeyToStreamDataLock };
				m_streamKeyToStreamData.erase(streamKey);
			}

			return false;
		}

		return true;
	}

	/**************************
	 * @brief Send new object for all active streams.
	 *
	 * @param object Object to send.
	 * @param filterPredicate Predicate for filter. Cannot be nullptr.
	 *
	 * @tparam T Type of object.
	 *
	 * @locking Read lock of m_streamObjectHashToStreamsLock on getting data and read lock of streams structure on
	 * checking and sending.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 * @todo Add unit test.
	 */
	template <typename T>
		requires std::is_class_v<T>
	void SendNewObject(
		const T& object, const std::function<bool(const FilterBase* filter, const T& object)> filterPredicate) const
	{
		std::shared_ptr<Streams> streams;
		const auto objectHash{ typeid(T).hash_code() };
		{
			Lock::AtomicRW::Guard<Lock::read> _{ m_streamObjectHashToStreamsLock };
			const auto it{ m_streamObjectHashToStreams.find(objectHash) };
			if (it == m_streamObjectHashToStreams.end()) {
				return;
			}

			streams = it->second;
		}

		Lock::AtomicRW::Guard<Lock::read> _{ streams->GetLock() };
		const auto& streamsContainer{ streams->GetStreams() };
		if (streamsContainer.empty()) {
			LOG_PROTOCOL_NEW("No active streams for object hash: {}", objectHash);
			return;
		}

		for (const auto& [key, streamData] : streamsContainer) {
			(void)SendObjectToStream(streamData, object, filterPredicate);
		}
	}

private:
	/**************************
	 * @brief Callback to handle new stream is opened action.
	 *
	 * @param streamData Stream data associated with opened stream.
	 *
	 * @todo Logic must be changed to be analogous to WebSocketEvents - function must be bound for a new stream of a
	 * particular type.
	 */
	virtual void HandleNewStreamOpened(const StreamData& streamData) = 0;
};

} // namespace Object

} // namespace Protocol

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_OBJECT_H