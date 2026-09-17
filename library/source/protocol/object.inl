/**************************
 * @file        object.inl
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
 * @brief Transfers simple copyable objects using stream and filter model.
 *
 * Stream has one custom filter and distributor must know how to react on this filter. Filter can has multiple custom
 * objects to filtration. Stream can be opened with different types: snapshot - get all currently available objects and
 * snapshot and live - get all currently available and all new objects while stream is open. Stream has callbacks about
 * states: opened, snapshot done, failed and object handle. Client must set connection for stream to mark who is the
 * distributor.
 *
 * States:
 * - Pending stream is waiting for answer right after stream opened.
 * - Opened stream is in active state.
 * - Done stream is in active state and got snapshot of data. Stream remains opened state with snapshot done flag is
 * set.
 * - Failed stream is closed with errors on distributor side.
 * - Closed stream is no longer gets any data. If stream closed by server side, it remains snapshot done flag. If client
 * closes stream, snapshot done flag is cleared and id is updated.
 *
 * @note Header data is 32 bytes long, following data alignment constraints will be always meet for systems with max 16
 * bytes alignment.
 * @note Identifier of stream is unique for an application instance.
 * @note Concurency model prioritizes distribution throughput and a stable recipient set over fast stream registration
 * and removal.
 *
 * Distribution takes a read lock on the stream collection and keeps it while checking streams and sending objects.
 * Multiple distributors may distribute concurrently, while stream addition and removal require the collection write
 * lock and wait for distribution to finish. This prioritizes distribution throughput and a stable recipient set over
 * fast stream registration and removal.
 *
 * @todo Filters can be || and &&.
 * @todo Allow number of different filters.
 * @todo typeid.hash_code() should be replaced with custom hash function.
 */

#ifndef MSAPI_PROTOCOL_OBJECT_INL
#define MSAPI_PROTOCOL_OBJECT_INL

#include "../help/autoClearPtr.inl"
#include "../help/log.h"
#include "../server/application.h"
#include "../server/connection.inl"
#include "../server/recvBuffer.inl"
#include "dataHeader.h"
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sys/socket.h>
#include <unordered_set>

namespace MSAPI {

namespace Protocol {

namespace Object {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

enum class Type : int8_t { Undefined, Snapshot, SnapshotAndLive, Max };

/**************************
 * @locking Is not required.
 *
 * @return Description of object protocol stream type enum.
 *
 * @test Yes.
 */
FORCE_INLINE constexpr std::string_view EnumToString(Type value) noexcept;

enum class State : int8_t { Undefined, Pending, Opened, Done, Failed, Closed, Max };

/**************************
 * @locking Is not required.
 *
 * @return Description of object protocol stream state enum.
 *
 * @test Yes.
 */
FORCE_INLINE constexpr std::string_view EnumToString(State value) noexcept;

enum class Issue : int8_t {
	Undefined,
	Empty,
	StreamIsAlreadyOpened,
	StreamDoesNotExist,
	FilterObjectHashMismatch,
	FilterNotFound,
	FilterSizeExceeded,
	UnknownHash,
	DistributorStopped,
	Max
};

/**************************
 * @locking Is not required.
 *
 * @return Description of object protocol stream issue enum.
 *
 * @test Yes.
 */
FORCE_INLINE constexpr std::string_view EnumToString(Issue value) noexcept;

/**************************
 * @brief Structure for provide stream state.
 *
 * @test Yes.
 */
struct StreamStateResponse {
	State state{ State::Undefined };
	Issue issue{ Issue::Empty };
};

/**************************
 * @brief General object for transferring data in stream.
 * Side: server, client.
 *
 * @concurrency No.
 */
class Data : public DataHeader {
private:
	uint64_t m_objectHash;
	uint64_t m_streamId;

public:
	static constexpr inline uint64_t CIPHER{ 2666999999 };

public:
	/**************************
	 * @brief Create object for transfering data in stream, update buffer size.
	 *
	 * @param streamId Stream id for which object is created. Stream id is not required if communication does not
	 * involve streams.
	 * @param objectHash Hash of object in data.
	 * @param size Size of object.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Data(uint64_t streamId, uint64_t objectHash, uint64_t size) noexcept;

	Data(const Data&) = delete;
	Data(Data&&) = delete;
	Data& operator=(const Data&) = delete;
	Data& operator=(Data&&) = delete;

	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool operator==(const Data&) const noexcept = default;

	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool operator!=(const Data&) const noexcept = default;

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
	 * @test Yes.
	 */
	template <typename T>
		requires std::is_same_v<std::decay_t<T>, DataHeader>
	FORCE_INLINE Data(T&& header, const std::span<const uint8_t> buffer) noexcept;

	/**************************
	 * @return Hash of object in data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetObjectHash() const noexcept;

	/**************************
	 * @return True if cipher is correct, buffer size, stream id and hash have not zero values.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept;

	/**************************
	 * @return Stream id for which object is related.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetStreamId() const noexcept;

	/**************************
	 * @brief Pack data before sending in stream.
	 *
	 * @attention Buffer should be freed after using.
	 *
	 * @param data Data for packing.
	 *
	 * @return Auto clear ptr with data for sending, can contain on allocation problems.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] AutoClearPtr<void> PackData(const void* data) const noexcept;

	/**************************
	 * @brief Shift the pointer to the beginning of the object.
	 *
	 * @tparam Object Type of object pointer.
	 *
	 * @param ptr Pointer to object.
	 * @param buffer Buffer with packed data.
	 *
	 * @pre ptr != nullptr;
	 * @pre buffer != nullptr;
	 *
	 * @test Yes.
	 */
	template <typename Object>
	FORCE_INLINE static void GetPointerToObjectInBuffer(const Object** ptr, const void* buffer) noexcept;

	/**************************
	 * @example Object protocol:
	 * {
	 * 			cipher	    : 2666999999
	 * 			buffer size	: 123
	 * 			object hash : 123456789
	 * 			stream id   : 123
	 * }
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const noexcept;
};

/**************************
 * @brief Polymorphic stream abstraction with basic fields.
 * Side: client.
 *
 * @concurrency Yes.
 */
class StreamBase {
public:
	/**************************
	 * @brief Stream state and snapshot flags storage.
	 *
	 * Allows one lock lookup for most frequently checked fields.
	 *
	 * @concurrency No.
	 */
	class StateData {
	private:
		State m_state;
		bool m_isSnapshotDone;

	public:
		/**************************
		 * @brief Construct stream state data.
		 *
		 * @param state Stream state.
		 * @param isSnapshotDone Stream is snapshot done flag.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE StateData(State state, bool isSnapshotDone) noexcept;

		StateData(const StateData&) = delete;
		FORCE_INLINE StateData(StateData&&) noexcept = default;
		StateData& operator=(const StateData&) = delete;
		FORCE_INLINE StateData& operator=(StateData&&) noexcept = default;

		/**************************
		 * @return Stream state.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] State GetState() const noexcept;

		/**************************
		 * @return Stream snapshot done flag.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsSnapshotDone() const noexcept;
	};

protected:
	std::atomic<uint64_t> m_id;
	const uint64_t m_objectHash;
	std::shared_ptr<Connection::Data> m_connectionData;
	Lock::AtomicRW m_lock;
	State m_state{ State::Closed };
	bool m_isSnapshotDone{};

	static inline std::atomic<uint64_t> m_streamCounter{};

public:
	/**************************
	 * @brief Construct a new Stream Base object, generate unique id for stream.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE StreamBase(uint64_t objectHash) noexcept;

	StreamBase(const StreamBase&) = delete;
	StreamBase(StreamBase&&) = delete;
	StreamBase& operator=(const StreamBase&) = delete;
	StreamBase& operator=(StreamBase&&) = delete;

	FORCE_INLINE virtual ~StreamBase() noexcept = default;

	/**************************
	 * @return Stream id.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetId() const noexcept;

	/**************************
	 * @return Stream object hash.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetObjectHash() const noexcept;

private:
	/**************************
	 * @brief Redirect call to specific type handler.
	 *
	 * @param bufferSize Object protocol buffer size.
	 * @param recvBuffer Related recv buffer.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE virtual void HandleObject(uint64_t bufferSize, RecvBuffer& recvBuffer);

	// To manage state on Collect call, assign connection data on SetStream, on handle object and fail streams
	friend class IHandlerBase;
};

/**************************
 * @brief Stream key for both client and server side.
 * Side: client.
 *
 * @concurrency No.
 */
class StreamConnectionId {
private:
	uint64_t m_streamId;
	uint64_t m_connectionId;

public:
	/**************************
	 * @brief Construct stream key.
	 *
	 * @param streamId Id of stream.
	 * @param connectionId Id of connection.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE StreamConnectionId(uint64_t streamId, uint64_t connectionId) noexcept;

	FORCE_INLINE StreamConnectionId(const StreamConnectionId&) noexcept = default;
	FORCE_INLINE StreamConnectionId(StreamConnectionId&&) noexcept = default;
	StreamConnectionId& operator=(const StreamConnectionId&) = delete;
	FORCE_INLINE StreamConnectionId& operator=(StreamConnectionId&&) noexcept = default;

	FORCE_INLINE [[nodiscard]] bool operator==(const StreamConnectionId&) const noexcept = default;

	/**************************
	 * @return Stream id.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetStreamId() const noexcept;

	/**************************
	 * @return Connection id.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetConnectionId() const noexcept;

	/**************************
	 * @return Hash.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] size_t Hash() const noexcept;
};

} // namespace Object

} // namespace Protocol

} // namespace MSAPI

namespace std {

template <> struct hash<MSAPI::Protocol::Object::StreamConnectionId> {
	FORCE_INLINE constexpr size_t operator()(const MSAPI::Protocol::Object::StreamConnectionId value) const noexcept
	{
		return value.Hash();
	}
};

} // namespace std

namespace MSAPI {

namespace Protocol {

namespace Object {

/**************************
 * @brief Polymorphic virtual stream data handler abstraction. Stores all active streams and manage their specific and
 * common callbacks.
 * Side: client.
 *
 * @concurrency Yes.
 */
class IHandlerBase {
private:
	const Application& m_application;
	std::unordered_map<StreamConnectionId, StreamBase*> m_streamConnectionIdToStream;
	Lock::AtomicRW m_streamConnectionIdToStreamLock;
	std::unordered_set<StreamConnectionId> m_closeConfirmation;

public:
	/**************************
	 * @brief Construct a new IHandlerBase object, empty constructor.
	 *
	 * @param application Readable pointer to application.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE IHandlerBase(const Application& application) noexcept;

	FORCE_INLINE virtual ~IHandlerBase() noexcept = default;

	IHandlerBase(const IHandlerBase&) = delete;
	IHandlerBase(IHandlerBase&&) = delete;
	IHandlerBase& operator=(const IHandlerBase&) = delete;
	IHandlerBase& operator=(IHandlerBase&&) = delete;

	/**************************
	 * @brief Callback when stream opened successfully.
	 *
	 * @param streamId Stream id for which callback is called.
	 *
	 * @locking Is not required.
	 * @locking If client has multiply streams with different connections, the handler can be called in parallel and
	 * additional synchronization can be required.
	 *
	 * @test Yes.
	 */
	virtual void HandleStreamOpened(uint64_t streamId) noexcept = 0;

	/**************************
	 * @brief Callback when stream got snapshot of data.
	 *
	 * @param streamId Stream id for which callback is called.
	 *
	 * @locking Is not required.
	 * @locking If client has multiply streams with different connections, the handler can be called in parallel and
	 * additional synchronization can be required.
	 *
	 * @test Yes.
	 */
	virtual void HandleStreamSnapshotDone(uint64_t streamId) noexcept = 0;

	/**************************
	 * @brief Callback when any error occurred on distributor side, reopen action is required.
	 *
	 * @param streamId Stream id for which callback is called.
	 * @param issue Enum issue representation.
	 *
	 * @locking Is not required.
	 * @locking If client has multiply streams with different connections, the handler can be called in parallel and
	 * additional synchronization can be required.
	 *
	 * @test Yes.
	 */
	virtual void HandleStreamFailed(uint64_t streamId, Issue issue) noexcept = 0;

	/**************************
	 * @brief Removes all streams related to connection id and call HandleStreamFailed for each.
	 *
	 * @param connectionId Connection id.
	 *
	 * @locking Read lock m_streamConnectionIdToStreamLock during lookup and write lock each affected stream.
	 * @locking Write lock m_streamConnectionIdToStreamLock during removing.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void FailStreamsForConnectionId(uint64_t connectionId) noexcept;

protected:
	/**************************
	 * @brief Assign a stream to handler. Reassign if connection is different and stream is not opened or pending.
	 *
	 * @param connectionData Stream related connection.
	 * @param stream Stream to be assigned.
	 *
	 * @pre connectionData is not nullptr.
	 * @pre stream is not nullptr.
	 *
	 * @locking Write lock m_streamConnectionIdToStreamLock and write lock stream inside.
	 *
	 * @return True on success, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SetStream(
		const std::shared_ptr<Connection::Data>& connectionData, StreamBase* stream) noexcept;

	/**************************
	 * @brief Try to collect object protocol object or stream state update. If application is not running, object
	 * handling is skipped.
	 *
	 * @param header General data header.
	 * @param recvBuffer Related recv buffer.
	 *
	 * @locking Read lock m_streamConnectionIdToStreamLock on stream lookup and additional write lock on close its
	 * confirmation.
	 * @locking Write lock on stream on stream state update.
	 *
	 * @return True if collected object belongs to object protocol regardless to its handling result, false if protocol
	 * is different.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Collect(DataHeader header, RecvBuffer& recvBuffer) noexcept;

	// To remove assignation on stream destruction
	template <typename Object, typename FObject>
		requires std::is_class_v<Object> && std::is_class_v<FObject>
	friend class Stream;
};

/**************************
 * @brief Specific object type handler.
 * Side: client.
 *
 * @tparam Object Object type to be handled from stream.
 *
 * @concurrency Yes.
 */
template <typename Object>
	requires std::is_class_v<Object>
class IHandler : virtual public IHandlerBase {
public:
	FORCE_INLINE IHandler() noexcept = default;

	IHandler(const IHandler&) = delete;
	IHandler(IHandler&&) = delete;
	IHandler& operator=(const IHandler&) = delete;
	IHandler& operator=(IHandler&&) = delete;

	/**************************
	 * @brief Handler for objects from stream.
	 *
	 * @note Provide the object as r-value reference does not lead to any performance advantages as the object is stored
	 * inside raw buffer, there is no optimization possible.
	 *
	 * @param streamId Stream id for which callback is called.
	 * @param object Handled object.
	 *
	 * @locking Is not required.
	 * @locking If client has multiply streams with different connections to the same object type, the handler can be
	 * called in parallel and additional synchronization can be required.
	 *
	 * @test Yes.
	 */
	virtual void HandleObject(uint64_t streamId, const Object& object) noexcept = 0;
};

/**************************
 * @brief Polymorphic filter data abstraction, stores general data. Is supposed to be touched by one thread.
 * Side: server, client.
 *
 * @attention Filter object cannot be removed from filter, the whole filter must be constructed from scratch.
 *
 * @concurrency No.
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
	 * @test Yes.
	 */
	FORCE_INLINE FilterBase(Type type) noexcept;

	FORCE_INLINE virtual ~FilterBase() noexcept = default;

	FORCE_INLINE FilterBase(const FilterBase&) noexcept = default;
	FORCE_INLINE FilterBase(FilterBase&&) noexcept = default;
	FORCE_INLINE FilterBase& operator=(const FilterBase&) noexcept = default;
	FORCE_INLINE FilterBase& operator=(FilterBase&&) noexcept = default;

	/**************************
	 * @return Total number of objects in filter.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetTotalFilterSize() const noexcept;

	/**************************
	 * @return Type of stream.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] Type GetType() const noexcept;

	/**************************
	 * @return Hash of stream object.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetStreamObjectHash() const noexcept;

	/**************************
	 * @return Hash of filter object.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] virtual uint64_t GetFilterObjectHash() const noexcept;

protected:
	/**************************
	 * @example Filter base:
	 * {
	 * 			type	           : Snapshot
	 * 			stream object hash : 0
	 * 			total filter size  : 0
	 * }
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const noexcept;

	/**************************
	 * @brief Set hash of stream object.
	 *
	 * @param streamObjectHash Hash to be set.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void SetStreamObjectHash(uint64_t streamObjectHash) noexcept;

	/**************************
	 * @brief Set the total filter size.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void SetTotalFilterSize(uint64_t size) noexcept;
};

/**************************
 * @brief Specific object type filter, includes filter object hash and container with filter objects.
 * Side: client.
 *
 * @attention Filter object can't be removed from filter, need to create new filter object.
 *
 * @tparam FObject Type of filter object.
 *
 * @concurency No.
 *
 * @todo Need to support multiple filters types for one stream with different logical operations.
 */
template <typename FObject>
	requires std::is_class_v<FObject>
class Filter : public FilterBase {
private:
	const static inline uint64_t m_filterObjecthash{ typeid(FObject).hash_code() };
	std::vector<FObject> m_objects;

public:
	/**************************
	 * @brief Construct a new Filter object.
	 *
	 * @param type Type of stream.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Filter(Type type) noexcept;

	/**************************
	 * @brief Construct a new Filter object from base filter.
	 *
	 * @param filter Base filter.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Filter(FilterBase&& filter) noexcept;

	FORCE_INLINE Filter(const Filter&) noexcept = default;
	FORCE_INLINE Filter(Filter&&) noexcept = default;
	Filter& operator=(const Filter&) = delete;
	FORCE_INLINE Filter& operator=(Filter&&) noexcept = default;

	/**************************
	 * @brief Set the Filter Object.
	 *
	 * @tparam FO Universal reference filter object type.
	 *
	 * @param object Filter object to be set.
	 *
	 * @return Size of filter objects after adding new one.
	 *
	 * @test Yes.
	 */
	template <typename FO>
		requires std::is_same_v<FObject, std::decay_t<FO>>
	FORCE_INLINE [[nodiscard]] uint64_t SetObject(FO&& object) noexcept;

	/**************************
	 * @return Readable link for container with filter objects.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] const std::vector<FObject>& GetObjects() const noexcept;

	/**************************
	 * @return Hash of filter object.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetFilterObjectHash() const noexcept final;

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
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] std::string ToString() const noexcept;

	// To SetTotalFilterSize and SetStreamObjectHash in SetFilter
	template <typename Object, typename FO /* Required as stream is not declared yet */>
		requires std::is_class_v<Object> && std::is_class_v<FO>
	friend class Stream;
};

/**************************
 * @brief Send object for particular stream.
 *
 * @param connection Connection for sending.
 * @param data Data for sending.
 * @param object Object for sending.
 *
 * @locking Is not required.
 *
 * @return True if data is sent successfully, false if any error occurred and connection is no longer usable.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool Send(Connection& connection, const Data& data, const void* object) noexcept;

/**************************
 * @brief Class for specific object stream, contains handler for callbacks and filter for stream.
 * Side: client.
 *
 * @tparam Object Type of stream object.
 * @tparam FObject Type of stream filter object.
 *
 * @concurrency Yes.
 */
template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
class Stream : public StreamBase {
private:
	IHandler<Object>& m_handler;
	std::optional<Filter<FObject>> m_filter;
	Lock::AtomicRW m_filterLock;

public:
	/**************************
	 * @brief Construct a new Stream object.
	 *
	 * @param handler Handler for callbacks.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] Stream(IHandler<Object>& handler) noexcept;

	/**************************
	 * @brief Close stream.
	 *
	 * @locking Read lock m_lock and write lock m_filterLock inside.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE ~Stream() noexcept;

	Stream(const Stream&) = delete;
	Stream(Stream&&) = delete;
	Stream& operator=(const Stream&) = delete;
	Stream& operator=(Stream&&) = delete;

	/**************************
	 * @locking Read lock inside.
	 *
	 * @return State data of stream.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] StreamBase::StateData GetStateData() noexcept;

	/**************************
	 * @locking Read lock inside.
	 *
	 * @return Related connection data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> GetConnectionData() noexcept;

	/**************************
	 * @brief Set related connection data.
	 *
	 * @param connectionData Related connection data.
	 *
	 * @locking Write lock m_streamConnectionIdToStreamLock for handlers and write lock stream inside.
	 *
	 * @return True on success, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SetConnectionData(const std::shared_ptr<Connection::Data>& connectionData);

	/**************************
	 * @brief Set the new filter for stream in closed or failed state. Attempt to set filter on stream in other state is
	 * failed.
	 *
	 * @tparam FO Type of filter object.
	 *
	 * @param filter Filter to set.
	 *
	 * @locking Read lock m_lock and write lock m_filterLock inside.
	 *
	 * @return True on success, false otherwise.
	 *
	 * @test Yes.
	 */
	template <typename FO>
		requires std::is_same_v<std::decay_t<FO>, Filter<FObject>>
	FORCE_INLINE [[nodiscard]] bool SetFilter(FO&& filter) noexcept;

	/**************************
	 * @brief Open stream if it is closed: failed or undefined state. Required to set distributor connection.
	 *
	 * @locking Read lock stream base and stream filter.
	 *
	 * @return True if stream open is sent to distributor, false if any error occurred.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Open() noexcept;

	/**************************
	 * @brief Close stream if it is active, clear snapshot done flag, set Closed state and set new unique stream id.
	 *
	 * @attention Closed state is applied on client side immediately, when server still can send data before receiving
	 * closing signal. The old stream id saving and new id generation allows to solve race issues:
	 * - Collecting data for unassigned stream.
	 * - Collecting data for reopened stream from previous opening.
	 * @attention On close or failed state initiated by the server the id is remained.
	 *
	 * @locking Write lock m_lock inside.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Close() noexcept;

private:
	/**************************
	 * @brief Redirect call to specific type handler.
	 *
	 * @param bufferSize Object protocol buffer size.
	 * @param recvBuffer Related recv buffer.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void HandleObject(uint64_t bufferSize, RecvBuffer& recvBuffer) final;
};

constexpr static inline bool CLEANUP_INSIDE{ true };
constexpr static inline bool CLEANUP_OUTSIDE{ false };

/**************************
 * @brief Contains data about streams and their filters.
 *
 * Distributor is responsible for sending objects to active streams. Only simple copyable objects are allowed to be sent
 * in stream, because they are sent in binary format.
 *
 * Side: server.
 *
 * @attention To force closing of active streams Stop() should be called.
 *
 * @tparam FObjects Types of filters which distributor can handle.
 *
 * @concurency Yes.
 *
 * @todo Currently only one filter type is supported for one stream, need to support multiple filters with different
 * logical operations.
 *
 * @todo Polish manual routing in HandleBuffer callback:
 * - Distributor knows all the types of filters, so it is possible to:
 * 1) Split Collect<T> for Filter<T> and T itself.
 * 2) Map each version of collect to hash in new map, then it will be possible to automatize filters handling routs.
 * - Register "stream object hash" manually to callback function with StreamData parameter in map with Streams, then
 * each hash will have its own opening callback.
 * - Think how to integrate access verification via Authorization module to allow access manage feature for that type of
 * stream.
 */
template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
class Distributor {
private:
	class Streams;

public:
	/**************************
	 * @brief Stream related data structure.
	 *
	 * @concurrency Yes.
	 */
	class StreamData {
	private:
		std::variant<std::monostate, Filter<FObjects>...> m_filter;
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
		 * @locking Is not required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE StreamData(std::shared_ptr<Connection::Data> connectionData /* by value as moved */,
			std::shared_ptr<Streams> streams /* by value as moved */, uint64_t streamId, uint64_t streamObjectHash,
			uint64_t filterObjectHash, uint64_t totalFiltersSize, Type type) noexcept;

		StreamData(const StreamData&) = delete;
		StreamData(StreamData&&) = delete;
		StreamData& operator=(const StreamData&) = delete;
		StreamData& operator=(StreamData&&) = delete;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Hash of stream object.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetStreamObjectHash() const noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Hash of filter object.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetFilterObjectHash() const noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Connection data structure associated with stream.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Connection& GetConnection() const noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Stream id.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetStreamId() const noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Stream type.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Type GetType() const noexcept;

	private:
		/**************************
		 * @brief Add new filter to stream.
		 *
		 * @tparam FObject Type of filter object.
		 *
		 * @param filter Base filter for filter to be set.
		 *
		 * @locking External write lock is required.
		 *
		 * @test Yes.
		 */
		template <typename FObject>
			requires is_included_in<FObject, FObjects...>
		FORCE_INLINE void SetFilter(FilterBase filter /* copy as moved */) noexcept;

		/**************************
		 * @locking External read lock is required.
		 *
		 * @return Pointer to base of existed filter or nullptr if not set.
		 *
		 * @test Yes.
		 */
		[[nodiscard]] FORCE_INLINE const FilterBase* GetFilter() const noexcept;

		/**************************
		 * @brief Set filter object for stream.
		 *
		 * @tparam FObject Type of filter object.
		 *
		 * @param object Object to be set.
		 *
		 * @locking External write lock is required.
		 *
		 * @return Undefined issue if filter object is set successfully, otherwise return error issue.
		 *
		 * @test Yes.
		 */
		template <typename FObject>
			requires is_included_in<FObject, FObjects...>
		FORCE_INLINE [[nodiscard]] Issue SetFilterObject(const FObject* object) noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Streams data which contains this stream data.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Streams& GetStreams() const noexcept;

		/**************************
		 * @locking External read lock is required.
		 *
		 * @return True if all filters are received, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsActive() const noexcept;

		/**************************
		 * @brief Mark stream as inactive.
		 *
		 * @locking External write lock is required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void UnsetActive() noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Data access lock.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Lock::AtomicRW& GetLock() noexcept;

		/**************************
		 * @locking External read lock is required.
		 *
		 * @example Stream data:
		 * {
		 * 			stream id          : 123
		 * 			connection id      : 74
		 * 			type               : Snapshot
		 *          stream object hash : 235423423
		 * 			filter object hash : 123456789
		 * 			total filter size  : 3
		 *			is active          : true
		 * }
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE std::string ToString() const noexcept;

		// Access to locking required fields on stopping
		friend class Distributor;
	};

private:
	/**************************
	 * @brief Contains all streams data for particular stream object hash.
	 *
	 * @concurrency Yes.
	 */
	class Streams {
	private:
		std::unordered_map<StreamConnectionId, std::shared_ptr<StreamData>> m_streamConnectionIdToStreamData;
		Lock::AtomicRW m_streamConnectionIdToStreamDataLock;

	public:
		FORCE_INLINE Streams() noexcept = default;

		Streams(const Streams&) = delete;
		Streams(Streams&&) = delete;
		Streams& operator=(const Streams&) = delete;
		Streams& operator=(Streams&&) = delete;

		/**************************
		 * @brief Remove stream from streams map.
		 *
		 * @param key Stream key to remove.
		 *
		 * @locking Write lock m_streamConnectionIdToStreamDataLock inside.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void RemoveStream(StreamConnectionId key) noexcept;

		/**************************
		 * @brief Add stream to streams map.
		 *
		 * @param key Stream key to add.
		 * @param streamData Stream data to add.
		 *
		 * @locking Write lock m_streamConnectionIdToStreamDataLock inside.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void AddSteam(
			StreamConnectionId key, std::shared_ptr<StreamData> streamData /* by value as moved */) noexcept;

		/**************************
		 * @locking Is not required.
		 *
		 * @return Internal lock for streams map.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Lock::AtomicRW& GetLock() noexcept;

		/**************************
		 * @locking External read lock is required.
		 *
		 * @return Readable link for streams map.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] std::unordered_map<StreamConnectionId, std::shared_ptr<StreamData>>&
		GetStreams() noexcept;
	};

private:
	const Application& m_application;
	std::unordered_map<StreamConnectionId, std::shared_ptr<StreamData>> m_streamConnectionIdToStreamData;
	Lock::AtomicRW m_streamConnectionIdToStreamDataLock;
	// TODO: Here can be stored handler, inside streams
	// TODO: Stream data can have reference on it. Is should be constant without additional lock
	std::unordered_map<uint64_t, std::shared_ptr<Streams>> m_streamObjectHashToStreams;
	Lock::AtomicRW m_streamObjectHashToStreamsLock;

public:
	/**************************
	 * @brief Construct distributor.
	 *
	 * @param application Readable pointer to application.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Distributor(const Application& application) noexcept;

	/**************************
	 * @brief Default destructor, stops active distributions.
	 *
	 * @locking Perform locking in Stop call.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE virtual ~Distributor() noexcept;

	Distributor(const Distributor&) = delete;
	Distributor(Distributor&&) = delete;
	Distributor& operator=(const Distributor&) = delete;
	Distributor& operator=(Distributor&&) = delete;

	/**************************
	 * @brief Send Failed state for all opened streams regardless to its active state and remove all information about
	 * them.
	 *
	 * @locking Write lock m_streamConnectionIdToStreamDataLock, write lock for each stream data and write lock
	 * m_streamObjectHashToStreamsLock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Stop() noexcept;

	/**************************
	 * @brief Removes all streams related to connection id.
	 *
	 * @param connectionId Connection id.
	 *
	 * @locking Read lock m_streamConnectionIdToStreamDataLock during lookup.
	 * @locking Write lock m_streamConnectionIdToStreamDataLock during removing.
	 * @locking Write lock associated streams structure each removal.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void ClearActiveStreamsForConnectionId(uint64_t connectionId) noexcept;

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
	 * @test Yes.
	 */
	FORCE_INLINE void StreamExternalAction(
		uint64_t streamId, uint64_t connectionId, const StreamStateResponse* response) noexcept;

	/**************************
	 * @brief Collecting stream data, it can be filter or its object. When filter is received, distributor will wait for
	 * all filter objects and then open stream or open stream instantly if filter size is equal to zero. When filter
	 * object is received, distributor will check if all filter objects are received and then open stream.
	 *
	 * @param connectionData Data structure of connection for which data is collected.
	 * @param data Data for collect.
	 * @param object Object for collect.
	 *
	 * @tparam FObject Type of filter object which presented in FObjects.
	 *
	 * @locking
	 * - Filter path: write locks m_streamObjectHashToStreamsLock and m_streamConnectionIdToStreamDataLock on getting
	 * data and write locks of StreamData on filter setting.
	 * - Filter object path: read lock m_streamConnectionIdToStreamDataLock on getting data and write lock of StreamData
	 * on setting stream object.
	 *
	 * @test Yes.
	 *
	 * @todo Need to add supporting multiple filters for one stream. For that each filter must have its own identifier.
	 */
	template <typename FObject>
		requires is_included_in<FObject, FObjects...>
	void Collect(const std::shared_ptr<Connection::Data>& connectionData, const Data& data, const void* object);

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
	 * @tparam Container Type of container.
	 * @tparam Object Type of object.
	 *
	 * @locking Read lock of the stream data on active and filter checking.
	 *
	 * @return True on success, false on error.
	 *
	 * @test Yes.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <template <typename> typename Container, typename Object>
		requires std::is_class_v<Object>
	[[nodiscard]] bool SendObjectsToStream(StreamData& streamData, const Container<Object>& objects,
		const std::function<bool(const FilterBase* filter, const Object& object)>& filterPredicate);

	/**************************
	 * @brief Send object to particular stream and remove it on error.
	 *
	 * Stream is deleted in case:
	 * - Stream is active but not have filter.
	 * - Send is not succeeded.
	 *
	 * @param streamData Stream data structure.
	 * @param object Object to send.
	 * @param filterPredicate Predicate for filter. Cannot be nullptr.
	 *
	 * @tparam Object Type of object.
	 *
	 * @locking Read lock of the stream data on active and filter checking.
	 * @locking Write lock streams on error cleanup.
	 *
	 * @return True if object was sent, false on error and stream is deleted.
	 *
	 * @todo Add tests coverage.
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename Object>
		requires std::is_class_v<Object>
	[[nodiscard]] bool SendObjectToStream(StreamData& streamData, const Object& object,
		const std::function<bool(const FilterBase* filter, const Object& object)>& filterPredicate);

	/**************************
	 * @brief Send new object for all active streams and remove them on error.
	 *
	 * Stream is deleted in case:
	 * - Stream is active but not have filter.
	 * - Send is not succeeded.
	 *
	 * @param object Object to send.
	 * @param filterPredicate Predicate for filter. Cannot be nullptr.
	 *
	 * @tparam Object Type of object.
	 *
	 * @locking Read lock of m_streamObjectHashToStreamsLock on getting data and read lock of streams structure on
	 * checking and sending.
	 *
	 * @test Yes.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename Object>
		requires std::is_class_v<Object>
	void SendNewObject(const Object& object,
		const std::function<bool(const FilterBase* filter, const Object& object)> filterPredicate);

private:
	/**************************
	 * @brief Send object to particular stream and remove it on error if cleanup policy is inside.
	 *
	 * Stream is deleted in case:
	 * - Stream is active but not have filter.
	 * - Send is not succeeded.
	 *
	 * @param streamData Stream data structure.
	 * @param object Object to send.
	 * @param filterPredicate Predicate for filter. Cannot be nullptr.
	 *
	 * @tparam Object Type of object.
	 * @tparam CleanupPolicy Define who is responsible for cleanup.
	 *
	 * @locking Read lock of the stream data on active and filter checking.
	 * @locking Write lock streams on error cleanup.
	 *
	 * @return True if object was sent, false on error.
	 *
	 * @test Yes.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename Object, bool CleanupPolicy>
		requires std::is_class_v<Object>
	[[nodiscard]] bool SendObjectToStreamImpl(StreamData& streamData, const Object& object,
		const std::function<bool(const FilterBase* filter, const Object& object)>& filterPredicate);

	/**************************
	 * @brief Callback to handle new stream is opened action.
	 *
	 * @param streamData Stream data associated with opened stream.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 *
	 * @todo Server should set handler for particular objects type stream instead of figuring out by hash comparison in
	 * common one.
	 */
	virtual void HandleNewStreamOpened(StreamData& streamData) = 0;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE constexpr std::string_view EnumToString(const Type value) noexcept
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

FORCE_INLINE constexpr std::string_view EnumToString(const State value) noexcept
{
	static_assert(U(State::Max) == 6, "Absence of stream state enum transcription");

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
	case State::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown stream state enum {}", U(value));
		return "Unknown";
	}
}

FORCE_INLINE constexpr std::string_view EnumToString(const Issue value) noexcept
{
	static_assert(U(Issue::Max) == 9, "Absence of stream issue enum transcription");

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
	case Issue::DistributorStopped:
		return "Distributor stopped";
	case Issue::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown stream issue enum: {}", U(value));
		return "Unknown";
	}
}

FORCE_INLINE [[nodiscard]] bool Send(Connection& connection, const Data& data, const void* object) noexcept
{
	LOG_PROTOCOL_NEW("Send data: {}, to connection id: {}", data.ToString(), connection.GetId());
	const auto packData{ data.PackData(object) };

	if (packData.Get() == nullptr) [[unlikely]] {
		return false;
	}

	return connection.Send(packData.Get(), data.GetBufferSize(), MSG_NOSIGNAL) != 0;
}

/*---------------------------------------------------------------------------------
Data
---------------------------------------------------------------------------------*/

FORCE_INLINE Data::Data(const uint64_t streamId, const uint64_t objectHash, const uint64_t size) noexcept
	: DataHeader{ CIPHER }
	, m_objectHash{ objectHash }
	, m_streamId{ streamId }
{
	m_bufferSize += sizeof(uint64_t) * 2 + size;
}

template <typename T>
	requires std::is_same_v<std::decay_t<T>, DataHeader>
FORCE_INLINE Data::Data(T&& header, const std::span<const uint8_t> buffer) noexcept
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

FORCE_INLINE [[nodiscard]] uint64_t Data::GetObjectHash() const noexcept { return m_objectHash; }

FORCE_INLINE [[nodiscard]] bool Data::IsValid() const noexcept
{
	return m_cipher == CIPHER && m_bufferSize >= sizeof(uint64_t) * 4 && m_objectHash != 0 && m_streamId != 0;
}

FORCE_INLINE [[nodiscard]] uint64_t Data::GetStreamId() const noexcept { return m_streamId; }

FORCE_INLINE [[nodiscard]] AutoClearPtr<void> Data::PackData(const void* data) const noexcept
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

template <typename Object>
FORCE_INLINE void Data::GetPointerToObjectInBuffer(const Object** ptr, const void* buffer) noexcept
{
	*ptr = reinterpret_cast<const Object*>(&(static_cast<const char*>(buffer)[sizeof(uint64_t) * 4]));
}

FORCE_INLINE [[nodiscard]] std::string Data::ToString() const noexcept
{
	return std::format("Object protocol:\n{{"
					   "\n\tcipher      : {}"
					   "\n\tbuffer size : {}"
					   "\n\tobject hash : {}"
					   "\n\tstream id   : {}"
					   "\n}}",
		m_cipher, m_bufferSize, m_objectHash, m_streamId);
}

/*---------------------------------------------------------------------------------
StreamBase::StateData
---------------------------------------------------------------------------------*/

FORCE_INLINE StreamBase::StateData::StateData(const State state, const bool isSnapshotDone) noexcept
	: m_state{ state }
	, m_isSnapshotDone{ isSnapshotDone }
{
}

FORCE_INLINE [[nodiscard]] State StreamBase::StateData::GetState() const noexcept { return m_state; }

FORCE_INLINE [[nodiscard]] bool StreamBase::StateData::IsSnapshotDone() const noexcept { return m_isSnapshotDone; }

/*---------------------------------------------------------------------------------
StreamBase
---------------------------------------------------------------------------------*/

FORCE_INLINE StreamBase::StreamBase(const uint64_t objectHash) noexcept
	: m_objectHash{ objectHash }
{
	m_id.store(m_streamCounter.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);
}

FORCE_INLINE [[nodiscard]] uint64_t StreamBase::GetId() const noexcept { return m_id.load(std::memory_order_relaxed); }

FORCE_INLINE [[nodiscard]] uint64_t StreamBase::GetObjectHash() const noexcept { return m_objectHash; }

FORCE_INLINE void StreamBase::HandleObject([[maybe_unused]] const uint64_t bufferSize, RecvBuffer& recvBuffer)
{
	LOG_ERROR_NEW("Pure virtual method is called for stream id: {}, connection id: {}",
		m_id.load(std::memory_order_relaxed), recvBuffer.GetConnectionId());
}

/*---------------------------------------------------------------------------------
StreamConnectionId
---------------------------------------------------------------------------------*/

FORCE_INLINE StreamConnectionId::StreamConnectionId(const uint64_t streamId, const uint64_t connectionId) noexcept
	: m_streamId{ streamId }
	, m_connectionId{ connectionId }
{
}

FORCE_INLINE [[nodiscard]] uint64_t StreamConnectionId::GetStreamId() const noexcept { return m_streamId; }

FORCE_INLINE [[nodiscard]] uint64_t StreamConnectionId::GetConnectionId() const noexcept { return m_connectionId; }

FORCE_INLINE [[nodiscard]] size_t StreamConnectionId::Hash() const noexcept
{
	return m_streamId ^ (m_connectionId + 0x9e3779b97f4a7c15ULL + (m_streamId << 6) + (m_streamId >> 2));
}

/*---------------------------------------------------------------------------------
IHandlerBase
---------------------------------------------------------------------------------*/

FORCE_INLINE IHandlerBase::IHandlerBase(const Application& application) noexcept
	: m_application{ application }
{
}

FORCE_INLINE void IHandlerBase::FailStreamsForConnectionId(const uint64_t connectionId) noexcept
{
	std::vector<StreamConnectionId> streamKeys;

	{
		const Lock::AtomicRW::Guard<Lock::read> _{ m_streamConnectionIdToStreamLock };

		for (const auto& [key, stream] : m_streamConnectionIdToStream) {
			if (key.GetConnectionId() == connectionId) {
				streamKeys.emplace_back(key);

				// Friendship access
				const Lock::AtomicRW::Guard<Lock::write> _{ stream->m_lock };
				stream->m_state = State::Failed;
			}
		}
	}

	if (streamKeys.empty()) {
		return;
	}

	auto size{ streamKeys.size() };
	LOG_PROTOCOL_NEW("Client starts failing {} stream(s) for connection id: {}", size, connectionId);

	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamLock };

		for (size_t index{}; index < size;) {
			const auto key{ streamKeys[index] };
			const auto it{ m_streamConnectionIdToStream.find(key) };
			if (it == m_streamConnectionIdToStream.end()) {
				m_closeConfirmation.erase(key);
				streamKeys[index] = std::move(streamKeys.back());
				--size;
				continue;
			}

			m_streamConnectionIdToStream.erase(it);
			++index;
		}
	}

	if (size == 0) [[unlikely]] {
		LOG_PROTOCOL_NEW("Client removed no streams for connection id: {}", connectionId);
		return;
	}

	// TODO: Lazy evaluation if logging level is enabled
	std::string message{ std::format("Client removed {} stream(s) with id:", size) };
	for (size_t index{}; index < size; ++index) {
		std::format_to(std::back_inserter(message), " {},", streamKeys[index].GetStreamId());
	}
	message.pop_back();

	LOG_PROTOCOL_NEW("{}, for connection id: {}", message, connectionId);

	for (size_t index{}; index < size; ++index) {
		HandleStreamFailed(streamKeys[index].GetStreamId(), Issue::DistributorStopped);
	}
}

FORCE_INLINE [[nodiscard]] bool IHandlerBase::SetStream(
	const std::shared_ptr<Connection::Data>& connectionData, StreamBase* const stream) noexcept
{
	const auto streamId{ stream->GetId() };
	bool alreadyAssigned [[indeterminate]];
	bool isOpenedOrPending [[indeterminate]];
	uint64_t previousConnectionId [[indeterminate]];

	do {
		// Friendship access
		const Lock::AtomicRW::Guard<Lock::write> _{ stream->m_lock };
		if (stream->m_connectionData != nullptr) {
			alreadyAssigned = true;
			previousConnectionId = stream->m_connectionData->GetConnectionId();

			if (stream->m_state == State::Pending || stream->m_state == State::Opened) {
				isOpenedOrPending = true;
				break;
			}

			isOpenedOrPending = false;
			break;
		}

		isOpenedOrPending = false;
		alreadyAssigned = false;
		stream->m_connectionData = connectionData;
	} while (false);

	const auto connectionId{ connectionData->GetConnectionId() };
	if (alreadyAssigned && connectionId == previousConnectionId) [[unlikely]] {
		return true;
	}

	if (isOpenedOrPending) [[unlikely]] {
		LOG_WARNING_NEW("Stream id: {} cannot be re assigned to new connection during its current state", streamId);
		return false;
	}

	bool isSuccess [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamLock };
		if (alreadyAssigned) {
			m_streamConnectionIdToStream.erase(StreamConnectionId{ streamId, previousConnectionId });
		}
		isSuccess = m_streamConnectionIdToStream.emplace(StreamConnectionId{ streamId, connectionId }, stream).second;
	}

	if (!isSuccess) [[unlikely]] {
		LOG_ERROR_NEW("Stream id: {} is not assigned to handler connection id: {}", streamId, connectionId);
		return false;
	}

	LOG_PROTOCOL_NEW("Stream id: {} is assigned to handler connection id: {}", streamId, connectionId);
	return true;
}

FORCE_INLINE [[nodiscard]] bool IHandlerBase::Collect(
	DataHeader header /* non const copy as moved */, RecvBuffer& recvBuffer) noexcept
{
	if (header.GetCipher() != Data::CIPHER) {
		return false;
	}

	if (!recvBuffer.RecvAdditional(sizeof(Data))) [[unlikely]] {
		return true;
	}

	const Data data{ std::move(header), recvBuffer.GetBuffer() };
	const auto streamId{ data.GetStreamId() };
	const auto connectionId{ recvBuffer.GetConnectionId() };
	const StreamConnectionId streamConnectionId{ streamId, connectionId };
	LOG_PROTOCOL_NEW("Collect {}, connection id: {}", data.ToString(), connectionId);

	StreamBase* stream{ nullptr };
	do {
		const Lock::AtomicRW::Guard<Lock::read> _{ m_streamConnectionIdToStreamLock };
		const auto it{ m_streamConnectionIdToStream.find(streamConnectionId) };
		if (it == m_streamConnectionIdToStream.end()) [[unlikely]] {
			break;
		}

		stream = it->second;
	} while (false);

	const auto receivedObjectHash{ data.GetObjectHash() };

	if (stream == nullptr) {
		if (receivedObjectHash != typeid(StreamStateResponse).hash_code()) [[unlikely]] {
			// (1) Drop unread payload to keep the connection byte stream in sync
			(void)recvBuffer.RecvTrunc(data.GetBufferSize() - sizeof(Data));
			LOG_PROTOCOL_NEW("Drop object with hash: {} for unactive stream id: {}, connection id: {}",
				receivedObjectHash, streamId, connectionId);
			return true;
		}

		const auto bufferSize{ data.GetBufferSize() };
		if (bufferSize - sizeof(Data) != sizeof(StreamStateResponse)) [[unlikely]] {
			// (1)
			(void)recvBuffer.RecvTrunc(data.GetBufferSize() - sizeof(Data));
			LOG_ERROR_NEW("Data contains unexpected stream state response size. {}", data.ToString());
			return true;
		}

		if (!recvBuffer.RecvAdditional(bufferSize)) [[unlikely]] {
			return true;
		}

		const StreamStateResponse* state [[indeterminate]];
		Data::GetPointerToObjectInBuffer(&state, recvBuffer.GetData());

		do {
			if (state->state != State::Closed) [[unlikely]] {
				break;
			}

			const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamLock };
			if (m_closeConfirmation.erase(streamConnectionId) == 1) [[likely]] {
				return true;
			}
		} while (false);

		LOG_WARNING_NEW("State: {} is reserved for unknown stream id: {}, connection id: {}",
			EnumToString(state->state), streamId, connectionId);
		return true;
	}

	if (receivedObjectHash == stream->GetObjectHash()) {
		if (!m_application.IsRunning()) [[unlikely]] {
			// (1)
			(void)recvBuffer.RecvTrunc(data.GetBufferSize() - sizeof(Data));
			LOG_PROTOCOL_NEW(
				"Application state is not running. Connection id: {}, collect {}", connectionId, data.ToString());
			return true;
		}

		// Friendship access
		stream->HandleObject(data.GetBufferSize(), recvBuffer);
		return true;
	}

	if (receivedObjectHash != typeid(StreamStateResponse).hash_code()) [[unlikely]] {
		// (1)
		(void)recvBuffer.RecvTrunc(data.GetBufferSize() - sizeof(Data));
		LOG_ERROR_NEW("Drop object with unknown hash: {}, stream id: {}, connection id: {}", receivedObjectHash,
			streamId, connectionId);
		return true;
	}

	const auto bufferSize{ data.GetBufferSize() };
	if (bufferSize - sizeof(Data) != sizeof(StreamStateResponse)) [[unlikely]] {
		// (1)
		(void)recvBuffer.RecvTrunc(data.GetBufferSize() - sizeof(Data));
		LOG_ERROR_NEW("Drop data with unexpected size of stream state response size. {}", data.ToString());
		return true;
	}

	if (!recvBuffer.RecvAdditional(bufferSize)) [[unlikely]] {
		return true;
	}

	const StreamStateResponse* state [[indeterminate]];
	Data::GetPointerToObjectInBuffer(&state, recvBuffer.GetData());

	LOG_PROTOCOL_NEW("State: {} is reserved for stream id: {}, connection id: {}", EnumToString(state->state), streamId,
		connectionId);

	switch (state->state) {
	case State::Opened: {
		// Friendship access
		const Lock::AtomicRW::Guard<Lock::write> _{ stream->m_lock };
		stream->m_isSnapshotDone = false;
		stream->m_state = State::Opened;
	}

		HandleStreamOpened(streamId);
		return true;
	case State::Done: {
		// Friendship access
		const Lock::AtomicRW::Guard<Lock::write> _{ stream->m_lock };
		stream->m_isSnapshotDone = true;
	}

		HandleStreamSnapshotDone(streamId);
		return true;
	case State::Failed: {
		// Friendship access
		const Lock::AtomicRW::Guard<Lock::write> _{ stream->m_lock };
		stream->m_state = state->state;
	}

		HandleStreamFailed(streamId, state->issue);
		return true;
	case State::Closed: {
		// Friendship access
		const Lock::AtomicRW::Guard<Lock::write> _{ stream->m_lock };
		stream->m_state = state->state;
	}
		return true;
	default:
		LOG_ERROR_NEW("Unknown state: {} is reserved for for stream id: {}, connection id {}:",
			EnumToString(state->state), streamId, connectionId);
		return true;
	}
}

/*---------------------------------------------------------------------------------
FilterBase
---------------------------------------------------------------------------------*/

FORCE_INLINE FilterBase::FilterBase(const Type type) noexcept
	: m_type(type)
{
}

FORCE_INLINE [[nodiscard]] uint64_t FilterBase::GetTotalFilterSize() const noexcept { return m_totalFilterSize; }

FORCE_INLINE [[nodiscard]] Type FilterBase::GetType() const noexcept { return m_type; }

FORCE_INLINE [[nodiscard]] uint64_t FilterBase::GetStreamObjectHash() const noexcept { return m_streamObjectHash; }

FORCE_INLINE [[nodiscard]] uint64_t FilterBase::GetFilterObjectHash() const noexcept
{
	LOG_ERROR("Pure method is called");
	return 0;
}

FORCE_INLINE [[nodiscard]] std::string FilterBase::ToString() const noexcept
{
	return std::format("Filter base:\n{{"
					   "\n\ttype               : {}"
					   "\n\tstream object hash : {}"
					   "\n\ttotal filter size  : {}"
					   "\n}}",
		EnumToString(m_type), m_streamObjectHash, m_totalFilterSize);
}

FORCE_INLINE void FilterBase::SetStreamObjectHash(const uint64_t streamObjectHash) noexcept
{
	m_streamObjectHash = streamObjectHash;
}

FORCE_INLINE void FilterBase::SetTotalFilterSize(const uint64_t size) noexcept { m_totalFilterSize = size; }

/*---------------------------------------------------------------------------------
Filter
---------------------------------------------------------------------------------*/

template <typename FObject>
	requires std::is_class_v<FObject>
FORCE_INLINE Filter<FObject>::Filter(const Type type) noexcept
	: FilterBase{ type }
{
}

template <typename FObject>
	requires std::is_class_v<FObject>
FORCE_INLINE Filter<FObject>::Filter(FilterBase&& filter) noexcept
	: FilterBase{ std::move(filter) }
{
}

template <typename FObject>
	requires std::is_class_v<FObject>
template <typename FO>
	requires std::is_same_v<FObject, std::decay_t<FO>>
FORCE_INLINE [[nodiscard]] uint64_t Filter<FObject>::SetObject(FO&& object) noexcept
{
	m_objects.emplace_back(std::forward<FO>(object));
	return m_objects.size();
}

template <typename FObject>
	requires std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] const std::vector<FObject>& Filter<FObject>::GetObjects() const noexcept
{
	return m_objects;
}

template <typename FObject>
	requires std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] uint64_t Filter<FObject>::GetFilterObjectHash() const noexcept
{
	return m_filterObjecthash;
}

template <typename FObject>
	requires std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] std::string Filter<FObject>::ToString() const noexcept
{
	return std::format("Filter special:\n{{"
					   "\n\tfilter object hash : {}"
					   "\n\tfilter size        : {}"
					   "\n\t                   : {}"
					   "\n}}",
		m_filterObjecthash, m_objects.size(), FilterBase::ToString());
}

/*---------------------------------------------------------------------------------
Stream
---------------------------------------------------------------------------------*/

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] Stream<Object, FObject>::Stream(IHandler<Object>& handler) noexcept
	: StreamBase{ typeid(Object).hash_code() }
	, m_handler{ handler }
{
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE Stream<Object, FObject>::~Stream() noexcept
{
	Close();
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] StreamBase::StateData Stream<Object, FObject>::GetStateData() noexcept
{
	const Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
	return { m_state, m_isSnapshotDone };
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> Stream<Object, FObject>::GetConnectionData() noexcept
{
	const Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
	return m_connectionData;
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] bool Stream<Object, FObject>::SetConnectionData(
	const std::shared_ptr<Connection::Data>& connectionData)
{
	if (connectionData == nullptr) [[unlikely]] {
		LOG_WARNING_NEW(
			"Reject attempt to set connection data as nullptr to stream id: ", m_id.load(std::memory_order_relaxed));
		return false;
	}

	// Friendship access
	if (!m_handler.SetStream(connectionData, this)) [[unlikely]] {
		return false;
	}

	m_connectionData = connectionData;
	LOG_PROTOCOL_NEW("Stream id: {} is now related with connection id: {}", m_id.load(std::memory_order_relaxed),
		m_connectionData->GetConnectionId());

	return true;
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object>
	&& std::is_class_v<FObject>
template <typename FO>
	requires std::is_same_v<std::decay_t<FO>, Filter<FObject>>
FORCE_INLINE [[nodiscard]] bool Stream<Object, FObject>::SetFilter(FO&& filter) noexcept
{
	const auto streamId{ m_id.load(std::memory_order_relaxed) };
	State state [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::read> _{ m_lock };
		state = m_state;
	}

	do {
		if (state == State::Closed || state == State::Failed) {
			break;
		}

		LOG_WARNING_NEW("Cannot reset filter on stream id: {} in {} state", streamId, EnumToString(state));
		return false;
	} while (false);

	const Lock::AtomicRW::Guard<Lock::write> _{ m_filterLock };

	auto& filterValue{ (m_filter = std::forward<FO>(filter)).value() };
	// Friendship access
	filterValue.SetStreamObjectHash(typeid(Object).hash_code());
	filterValue.SetTotalFilterSize(filterValue.GetObjects().size());

	LOG_PROTOCOL_NEW("Client sets: {} for stream id: {}", filterValue.ToString(), streamId);

	return true;
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE [[nodiscard]] bool Stream<Object, FObject>::Open() noexcept
{
	const auto streamId{ m_id.load(std::memory_order_relaxed) };
	const Lock::AtomicRW::Guard<Lock::read> _{ m_lock };

	if (m_connectionData == nullptr) [[unlikely]] {
		LOG_PROTOCOL_NEW("Client tries to open stream id {} without connection", streamId);
		return false;
	}

	const Lock::AtomicRW::Guard<Lock::read> _{ m_filterLock };

	if (!m_filter.has_value()) [[unlikely]] {
		LOG_PROTOCOL_NEW("Client tries to open stream id {} without filter", streamId);
		return false;
	}

	if (m_state == State::Opened || m_state == State::Pending) [[unlikely]] {
		LOG_PROTOCOL_NEW("Stream id {} is already in active state {}", streamId, EnumToString(m_state));
		return false;
	}

	const auto& filterValue{ m_filter.value() };
	m_state = State::Pending;
	LOG_PROTOCOL_NEW("Client opens stream id {} with {}", streamId, filterValue.ToString());

	auto& connection{ m_connectionData->GetConnection() };

	if (!Send(connection, { streamId, typeid(Filter<FObject>).hash_code(), sizeof(FilterBase) },
			static_cast<const FilterBase*>(&filterValue))) [[unlikely]] {
		m_state = State::Failed;
		return false;
	}

	const Data data{ streamId, typeid(FObject).hash_code(), sizeof(FObject) };
	for (const auto& filterObject : filterValue.GetObjects()) {
		if (!Send(connection, data, &filterObject)) [[unlikely]] {
			m_state = State::Failed;
			return false;
		}
	}

	return true;
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE void Stream<Object, FObject>::Close() noexcept
{
	const auto oldId{ m_id.load(std::memory_order_relaxed) };
	uint64_t newId [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_lock };

		if (m_connectionData == nullptr) [[unlikely]] {
			LOG_PROTOCOL_NEW("Client tries to close stream id {} without connection", oldId);
			return;
		}

		if (m_state != State::Opened && m_state != State::Pending) [[unlikely]] {
			LOG_PROTOCOL_NEW("Reject attempt to close stream id {} with state {}", oldId, EnumToString(m_state));
			return;
		}

		newId = m_streamCounter.fetch_add(1, std::memory_order_relaxed);
		m_id.store(newId, std::memory_order_relaxed);

		m_isSnapshotDone = false;
		m_state = State::Closed;
	}

	LOG_PROTOCOL_NEW("Client closes stream id: {}, new stream id is: {}", oldId, newId);

	StreamStateResponse state{ State::Closed };
	(void)Send(m_connectionData->GetConnection(),
		{ oldId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &state);

	const auto connectionId{ m_connectionData->GetConnectionId() };
	StreamConnectionId oldKey{ oldId, connectionId };
	// Friendship access
	const Lock::AtomicRW::Guard<Lock::write> _{ m_handler.m_streamConnectionIdToStreamLock };
	m_handler.m_streamConnectionIdToStream.erase(oldKey);
	m_handler.m_closeConfirmation.emplace(std::move(oldKey));
	// Connection id is set and association is required
	m_handler.m_streamConnectionIdToStream.emplace(StreamConnectionId{ newId, connectionId }, this);
}

template <typename Object, typename FObject>
	requires std::is_class_v<Object> && std::is_class_v<FObject>
FORCE_INLINE void Stream<Object, FObject>::HandleObject(const uint64_t bufferSize, RecvBuffer& recvBuffer)
{
	if (bufferSize - sizeof(Data) != sizeof(Object)) [[unlikely]] {
		// Drop unread payload to keep the connection byte stream in sync
		(void)recvBuffer.RecvTrunc(bufferSize - sizeof(Data));
		LOG_ERROR_NEW("Drop data with unexpected object size: {}. Stream id: {}, connection id: {}", bufferSize,
			m_id.load(std::memory_order_relaxed), recvBuffer.GetConnectionId());
		return;
	}

	if (!recvBuffer.RecvAdditional(bufferSize)) [[unlikely]] {
		return;
	}

	const Object* object [[indeterminate]];
	Data::GetPointerToObjectInBuffer(&object, recvBuffer.GetData());
	m_handler.HandleObject(m_id.load(std::memory_order_relaxed), *object);
}

/*---------------------------------------------------------------------------------
Distributor::StreamData
---------------------------------------------------------------------------------*/

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE Distributor<FObjects...>::StreamData::StreamData(
	std::shared_ptr<Connection::Data> connectionData /*copy as moved */,
	std::shared_ptr<Streams> streams /* copy as moved */, const uint64_t streamId, const uint64_t streamObjectHash,
	const uint64_t filterObjectHash, const uint64_t totalFiltersSize, const Type type) noexcept
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

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] uint64_t Distributor<FObjects...>::StreamData::GetStreamObjectHash() const noexcept
{
	return m_streamObjectHash;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] uint64_t Distributor<FObjects...>::StreamData::GetFilterObjectHash() const noexcept
{
	return m_filterObjectHash;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] Connection& Distributor<FObjects...>::StreamData::GetConnection() const noexcept
{
	return m_connectionData->GetConnection();
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] uint64_t Distributor<FObjects...>::StreamData::GetStreamId() const noexcept
{
	return m_streamId;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] Type Distributor<FObjects...>::StreamData::GetType() const noexcept
{
	return m_type;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <typename FObject>
	requires is_included_in<FObject, FObjects...>
FORCE_INLINE void Distributor<FObjects...>::StreamData::SetFilter(FilterBase filter /* copy as moved */) noexcept
{
	m_filter = Filter<FObject>(std::move(filter));
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
[[nodiscard]] FORCE_INLINE const FilterBase* Distributor<FObjects...>::StreamData::GetFilter() const noexcept
{
	return std::visit(
		[](const auto& filter) -> const FilterBase* {
			if constexpr (std::is_same_v<std::decay_t<decltype(filter)>, std::monostate>) {
				return nullptr;
			}
			else {
				return &filter;
			}
		},
		m_filter);
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <typename FObject>
	requires is_included_in<FObject, FObjects...>
FORCE_INLINE [[nodiscard]] Issue Distributor<FObjects...>::StreamData::SetFilterObject(
	const FObject* const object) noexcept
{
	auto* const filterPtr{ std::get_if<Filter<FObject>>(&m_filter) };
	if (filterPtr == nullptr) [[unlikely]] {
		if (m_filterObjectHash != typeid(FObject).hash_code()) [[unlikely]] {
			LOG_ERROR_NEW(
				"Filter object hash: {} is not set for stream id: {} as it is different from expected hash: {}",
				typeid(FObject).hash_code(), m_streamId, m_filterObjectHash);
			return Issue::FilterObjectHashMismatch;
		}

		LOG_ERROR_NEW("Filter to be updated with object hash: {} is not found. Stream id: {} ",
			typeid(FObject).hash_code(), m_streamId);
		return Issue::FilterNotFound;
	}

	const auto currentSize{ filterPtr->SetObject(*static_cast<const FObject*>(object)) };

	if (currentSize == m_totalFiltersSize) [[likely]] {
		m_isActive = true;
		return Issue::Undefined;
	}

	if (currentSize < m_totalFiltersSize) [[likely]] {
		return Issue::Undefined;
	}

	return Issue::FilterSizeExceeded;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] Distributor<FObjects...>::Streams&
Distributor<FObjects...>::StreamData::GetStreams() const noexcept
{
	return *m_streams;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] bool Distributor<FObjects...>::StreamData::IsActive() const noexcept
{
	return m_isActive;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE void Distributor<FObjects...>::StreamData::UnsetActive() noexcept
{
	m_isActive = false;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] Lock::AtomicRW& Distributor<FObjects...>::StreamData::GetLock() noexcept
{
	return m_lock;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE std::string Distributor<FObjects...>::StreamData::ToString() const noexcept
{
	return std::format("Stream data:\n{{"
					   "\n\tstream id          : {}"
					   "\n\tconnection id      : {}"
					   "\n\ttype               : {}"
					   "\n\tstream object hash : {}"
					   "\n\tfilter object hash : {}"
					   "\n\ttotal filter size  : {}"
					   "\n\tis active          : {}"
					   "\n}}",
		m_streamId, m_connectionData->GetConnectionId(), EnumToString(m_type), m_streamObjectHash, m_filterObjectHash,
		m_totalFiltersSize, m_isActive);
}

/*---------------------------------------------------------------------------------
Distributor::Streams
---------------------------------------------------------------------------------*/

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE void Distributor<FObjects...>::Streams::RemoveStream(const StreamConnectionId key) noexcept
{
	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
		m_streamConnectionIdToStreamData.erase(key);
	}

	LOG_PROTOCOL_NEW("Removed stream id: {}, connection id: {}", key.GetStreamId(), key.GetConnectionId());
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE void Distributor<FObjects...>::Streams::AddSteam(StreamConnectionId key /* non const copy as moved */,
	std::shared_ptr<Distributor<FObjects...>::StreamData> streamData /* copy as moved */) noexcept
{
	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
		m_streamConnectionIdToStreamData.emplace(std::move(key), std::move(streamData));
	}

	LOG_PROTOCOL_NEW("Added stream id: {}, connection id: {}", key.GetStreamId(), key.GetConnectionId());
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] Lock::AtomicRW& Distributor<FObjects...>::Streams::GetLock() noexcept
{
	return m_streamConnectionIdToStreamDataLock;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE [[nodiscard]] std::unordered_map<StreamConnectionId,
	std::shared_ptr<typename Distributor<FObjects...>::StreamData>>&
Distributor<FObjects...>::Streams::GetStreams() noexcept
{
	return m_streamConnectionIdToStreamData;
}

/*---------------------------------------------------------------------------------
Distributor
---------------------------------------------------------------------------------*/

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE Distributor<FObjects...>::Distributor(const Application& application) noexcept
	: m_application{ application }
{
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE Distributor<FObjects...>::~Distributor() noexcept
{
	Stop();
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE void Distributor<FObjects...>::Stop() noexcept
{
	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
		auto begin{ m_streamConnectionIdToStreamData.begin() };
		const auto end{ m_streamConnectionIdToStreamData.end() };
		if (begin == end) {
			return;
		}

		LOG_PROTOCOL_NEW(
			"Distributor starts removing information about {} stream(s)", m_streamConnectionIdToStreamData.size());
		const StreamStateResponse state{ State::Failed, Issue::DistributorStopped };
		const auto hash{ typeid(StreamStateResponse).hash_code() };
		const auto size{ sizeof(StreamStateResponse) };

		std::shared_ptr<StreamData> streamData;
		while (true) {
			streamData = begin->second;
			(void)Send(streamData->GetConnection(), Data{ begin->first.GetStreamId(), hash, size }, &state);

			begin = m_streamConnectionIdToStreamData.erase(begin);
			if (begin == end) {
				return;
			}
		}
	}

	const Lock::AtomicRW::Guard<Lock::write> _{ m_streamObjectHashToStreamsLock };
	m_streamObjectHashToStreams.clear();
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE void Distributor<FObjects...>::ClearActiveStreamsForConnectionId(const uint64_t connectionId) noexcept
{
	std::vector<StreamConnectionId> streamKeys;

	{
		const Lock::AtomicRW::Guard<Lock::read> _{ m_streamConnectionIdToStreamDataLock };

		for (const auto& [key, streamData] : m_streamConnectionIdToStreamData) {
			if (key.GetConnectionId() == connectionId) {
				streamKeys.emplace_back(key);
			}
		}
	}

	if (streamKeys.empty()) {
		return;
	}

	auto size{ streamKeys.size() };
	LOG_PROTOCOL_NEW("Distributor starts removing {} stream(s) for connection id: {}", size, connectionId);

	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };

		for (size_t index{}; index < size;) {
			const auto key{ streamKeys[index] };
			const auto it{ m_streamConnectionIdToStreamData.find(key) };
			if (it == m_streamConnectionIdToStreamData.end()) [[unlikely]] {
				streamKeys[index] = std::move(streamKeys.back());
				--size;
				continue;
			}

			// Locks inside
			it->second->GetStreams().RemoveStream(key);
			m_streamConnectionIdToStreamData.erase(it);
			++index;
		}
	}

	if (size == 0) [[unlikely]] {
		LOG_PROTOCOL_NEW("Distributor removed no streams for connection id: {}", connectionId);
		return;
	}

	// TODO: Lazy evaluation if logging level is enabled
	std::string message{ std::format("Distributor removed {} stream(s): with id:", size) };
	for (size_t index{}; index < size; ++index) {
		std::format_to(std::back_inserter(message), " {},", streamKeys[index].GetStreamId());
	}
	message.pop_back();

	LOG_PROTOCOL_NEW("{}, for connection id: {}", message, connectionId);
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
FORCE_INLINE void Distributor<FObjects...>::StreamExternalAction(
	const uint64_t streamId, const uint64_t connectionId, const StreamStateResponse* const response) noexcept
{
	const StreamConnectionId streamConnectionId{ streamId, connectionId };
	switch (response->state) {
	case State::Closed: {
		LOG_PROTOCOL_NEW("Client closed stream id: {}, connection id: {}", streamId, connectionId);

		const StreamStateResponse state{ State::Closed };
		std::shared_ptr<StreamData> streamData;
		{
			const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
			const auto it{ m_streamConnectionIdToStreamData.find(streamConnectionId) };
			if (it != m_streamConnectionIdToStreamData.end()) [[likely]] {
				streamData = it->second;
				m_streamConnectionIdToStreamData.erase(it);
			}
		}

		if (streamData == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Have not stream data for stream id: {}, connection id: {}", streamId, connectionId);
			return;
		}

		(void)Send(streamData->GetConnection(),
			Data{ streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &state);

		// Locks inside
		streamData->GetStreams().RemoveStream(streamConnectionId);
	}
		return;
	default:
		LOG_WARNING_NEW("Unexpected stream state for external action: {}, stream id: {}, connection id: {}",
			EnumToString(response->state), streamId, connectionId);
		return;
	}
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <typename FObject>
	requires is_included_in<FObject, FObjects...>
void Distributor<FObjects...>::Collect(
	const std::shared_ptr<Connection::Data>& connectionData, const Data& data, const void* object)
{
	const auto streamId{ data.GetStreamId() };
	const auto connectionId{ connectionData->GetConnectionId() };
	StreamConnectionId streamConnectionId{ streamId, connectionId };

	if (!m_application.IsRunning()) [[unlikely]] {
		LOG_PROTOCOL_NEW("Application state is not Running, collect {}, stream id: {}, connection id: {}",
			data.ToString(), streamId, connectionId);
		return;
	}

	LOG_PROTOCOL_NEW("Collect {}, stream id: {}, connection id: {}", data.ToString(), streamId, connectionId);

	const auto dataHash{ data.GetObjectHash() };
	if (typeid(Filter<FObject>).hash_code() == dataHash) {
		const FilterBase* const filter{ reinterpret_cast<const FilterBase*>(object) };
		const auto filterObjectHash{ typeid(FObject).hash_code() };
		const auto streamObjectHash{ filter->GetStreamObjectHash() };

		std::shared_ptr<Streams> streams;
		{
			{
				const Lock::AtomicRW::Guard<Lock::read> _{ m_streamObjectHashToStreamsLock };
				const auto it{ m_streamObjectHashToStreams.find(streamObjectHash) };
				if (it != m_streamObjectHashToStreams.end()) [[likely]] {
					streams = it->second;
				}
			}

			if (streams == nullptr) [[unlikely]] {
				streams = std::make_shared<Streams>();
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamObjectHashToStreamsLock };
				m_streamObjectHashToStreams.emplace(streamObjectHash, streams);
			}
		}

		const auto streamType{ filter->GetType() };
		const auto totalFilterSize{ filter->GetTotalFilterSize() };
		const auto streamData{ std::make_shared<StreamData>(
			connectionData, streams, streamId, streamObjectHash, filterObjectHash, totalFilterSize, streamType) };
		const auto isFilterNotEmpty{ totalFilterSize != 0 };

		if (streamType == Type::SnapshotAndLive || isFilterNotEmpty) {
			std::pair<typename std::decay_t<decltype(m_streamConnectionIdToStreamData)>::iterator, bool> result;
			{
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
				result = m_streamConnectionIdToStreamData.emplace(streamConnectionId, streamData);
			}

			if (!result.second) [[unlikely]] {
				LOG_ERROR_NEW("Stream id: {} is already opened for connection id: {}", streamId, connectionId);

				const StreamStateResponse response{ State::Failed, Issue::StreamIsAlreadyOpened };
				const Data dataResponse{ streamId, typeid(StreamStateResponse).hash_code(),
					sizeof(StreamStateResponse) };

				(void)Send(result.first->second->GetConnection(), dataResponse, &response);

				return;
			}

			// Locks inside
			streams->AddSteam(streamConnectionId, streamData);
		}

		if (isFilterNotEmpty) {
			const Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };
			streamData->template SetFilter<FObject>(*filter);
			LOG_PROTOCOL_NEW("Waiting filter objects for new stream id: {}, connection id: {}. {}", streamId,
				connectionId, streamData->ToString());
			return;
		}

		{
			const Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };
			streamData->template SetFilter<FObject>(*filter);
			LOG_PROTOCOL_NEW("Instantly open new stream id: {}, connection id: {}. {}", streamId, connectionId,
				streamData->ToString());
		}

		StreamStateResponse response{ State::Opened };
		const Data dataResponse{ streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

		auto& connection{ connectionData->GetConnection() };

		if (!Send(connection, dataResponse, &response)) [[unlikely]] {
			return;
		}

		HandleNewStreamOpened(*streamData);

		response.state = State::Done;
		if (!Send(connection, dataResponse, &response)) [[unlikely]] {
			return;
		}

		if (streamType == Type::Snapshot) {
			response.state = State::Closed;
			if (!Send(connection, dataResponse, &response)) [[unlikely]] {
				return;
			}
		}

		return;
	}

	std::shared_ptr<StreamData> streamData;
	{
		const Lock::AtomicRW::Guard<Lock::read> _{ m_streamConnectionIdToStreamDataLock };
		const auto it{ m_streamConnectionIdToStreamData.find(streamConnectionId) };
		if (it != m_streamConnectionIdToStreamData.end()) [[likely]] {
			streamData = it->second;
		}
	}

	if (streamData == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Have not stream data for stream id: {}, connection id: {}", streamId, connectionId);

		const StreamStateResponse response{ State::Failed, Issue::StreamDoesNotExist };
		const Data dataResponse{ streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

		(void)Send(connectionData->GetConnection(), dataResponse, &response);
		return;
	}

	if (typeid(FObject).hash_code() == dataHash) [[likely]] {
		Streams* streams [[indeterminate]];
		Issue issue [[indeterminate]];
		bool isActive [[indeterminate]];
		{
			const Lock::AtomicRW::Guard<Lock::write> _{ streamData->GetLock() };
			issue = streamData->template SetFilterObject<FObject>(static_cast<const FObject*>(object));
			streams = &(streamData->GetStreams());
			isActive = streamData->IsActive();
		}

		if (issue != Issue::Undefined) [[unlikely]] {
			// Locks inside
			streams->RemoveStream(streamConnectionId);

			{
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
				m_streamConnectionIdToStreamData.erase(streamConnectionId);
			}

			const StreamStateResponse response{ State::Failed, issue };
			const Data dataResponse{ streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

			(void)Send(connectionData->GetConnection(), dataResponse, &response);
			return;
		}

		if (isActive) {
			LOG_PROTOCOL_NEW(
				"Got last filter object and open stream id: {}, connection id: {}", streamId, connectionId);

			StreamStateResponse response{ State::Opened };
			const Data dataResponse{ streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

			if (!Send(connectionData->GetConnection(), dataResponse, &response)) [[unlikely]] {
				// Locks inside
				streams->RemoveStream(streamConnectionId);

				{
					const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
					m_streamConnectionIdToStreamData.erase(streamConnectionId);
				}
				return;
			}

			HandleNewStreamOpened(*streamData);

			response.state = State::Done;
			if (!Send(connectionData->GetConnection(), dataResponse, &response)) [[unlikely]] {
				// Locks inside
				streams->RemoveStream(streamConnectionId);

				{
					const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
					m_streamConnectionIdToStreamData.erase(streamConnectionId);
				}
				return;
			}

			if (streamData->GetType() == Type::Snapshot) {
				response.state = State::Closed;
				if (!Send(connectionData->GetConnection(), dataResponse, &response)) [[unlikely]] {
					return;
				}

				// Locks inside
				streams->RemoveStream(streamConnectionId);

				{
					const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
					m_streamConnectionIdToStreamData.erase(streamConnectionId);
				}
			}

			return;
		}

		LOG_PROTOCOL_NEW("Got filter object for stream id: {}, connection id: {}", streamId, connectionId);
		return;
	}

	LOG_ERROR_NEW(
		"Unknown hash has been reserved: {}, stream id: {}, connection id: {}", dataHash, streamId, connectionId);

	// Locks inside
	streamData->GetStreams().RemoveStream(streamConnectionId);

	{
		const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
		m_streamConnectionIdToStreamData.erase(streamConnectionId);
	}

	const StreamStateResponse response{ State::Failed, Issue::UnknownHash };
	const Data dataResponse{ streamId, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };

	(void)Send(connectionData->GetConnection(), dataResponse, &response);
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <template <typename> typename Container, typename Object>
	requires std::is_class_v<Object>
[[nodiscard]] bool Distributor<FObjects...>::SendObjectsToStream(StreamData& streamData,
	const Container<Object>& objects,
	const std::function<bool(const FilterBase* filter, const Object& object)>& filterPredicate)
{
	const auto objectsSize{ objects.size() };
	if (objectsSize == 0) [[unlikely]] {
		return true;
	}

	const FilterBase* filter [[indeterminate]];
	auto& connection{ streamData.GetConnection() };
	Streams* streams [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::read> _{ streamData.GetLock() };
		if (!streamData.IsActive()) [[unlikely]] {
			LOG_PROTOCOL_NEW(
				"Stream id: {} is not active. Connection id: {}", streamData.GetStreamId(), connection.GetId());
			return true;
		}

		filter = streamData.GetFilter();
		streams = &(streamData.GetStreams());
	}

	if (filter == nullptr) [[unlikely]] {
		LOG_ERROR_NEW(
			"Filter is not set for stream id: {}, connection id: {}", streamData.GetStreamId(), connection.GetId());

		const StreamConnectionId streamConnectionId{ streamData.GetStreamId(), connection.GetId() };
		// Locks inside
		streams->RemoveStream(streamConnectionId);

		{
			const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
			m_streamConnectionIdToStreamData.erase(streamConnectionId);
		}

		return false;
	}

	const Data data{ streamData.GetStreamId(), typeid(Object).hash_code(), sizeof(Object) };
	LOG_PROTOCOL_NEW("Try to send {} objects to stream id: {} connection id: {}", objectsSize, data.GetStreamId(),
		connection.GetId());

	if (filter->GetTotalFilterSize() == 0) {
		for (const auto& object : objects) {
			if (!Send(connection, data, &object)) [[unlikely]] {
				const StreamConnectionId streamConnectionId{ streamData.GetStreamId(), connection.GetId() };
				// Locks inside
				streams->RemoveStream(streamConnectionId);

				{
					const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
					m_streamConnectionIdToStreamData.erase(streamConnectionId);
				}

				return false;
			}
		}

		return true;
	}

	for (const auto& object : objects) {
		if (filterPredicate(filter, object) && !Send(connection, data, &object)) [[unlikely]] {
			const StreamConnectionId streamConnectionId{ streamData.GetStreamId(), connection.GetId() };
			// Locks inside
			streams->RemoveStream(streamConnectionId);

			{
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
				m_streamConnectionIdToStreamData.erase(streamConnectionId);
			}

			return false;
		}
	}

	return true;
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <typename Object>
	requires std::is_class_v<Object>
[[nodiscard]] bool Distributor<FObjects...>::SendObjectToStream(StreamData& streamData, const Object& object,
	const std::function<bool(const FilterBase* filter, const Object& object)>& filterPredicate)
{
	return SendObjectToStreamImpl<Object, CLEANUP_INSIDE>(streamData, object, filterPredicate);
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <typename Object>
	requires std::is_class_v<Object>
void Distributor<FObjects...>::SendNewObject(
	const Object& object, const std::function<bool(const FilterBase* filter, const Object& object)> filterPredicate)
{
	std::shared_ptr<Streams> streams;
	const auto objectHash{ typeid(Object).hash_code() };
	{
		const Lock::AtomicRW::Guard<Lock::read> _{ m_streamObjectHashToStreamsLock };
		const auto it{ m_streamObjectHashToStreams.find(objectHash) };
		if (it == m_streamObjectHashToStreams.end()) {
			return;
		}

		streams = it->second;
	}

	std::vector<StreamConnectionId> toRemove;
	{
		const Lock::AtomicRW::Guard<Lock::read> _{ streams->GetLock() };
		const auto& streamsContainer{ streams->GetStreams() };
		if (streamsContainer.empty()) {
			LOG_PROTOCOL_NEW("No active streams for object hash: {}", objectHash);
			return;
		}

		for (const auto& [key, streamData] : streamsContainer) {
			if (!SendObjectToStreamImpl<Object, CLEANUP_OUTSIDE>(*streamData, object, filterPredicate)) [[unlikely]] {
				toRemove.emplace_back(key);
			}
		}
	}

	if (!toRemove.empty()) [[unlikely]] {
		for (const auto& key : toRemove) {
			// Locks insidebool
			streams->RemoveStream(key);

			{
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
				m_streamConnectionIdToStreamData.erase(key);
			}
		}
	}
}

template <typename... FObjects>
	requires(std::is_class_v<FObjects> && ...)
template <typename Object, bool CleanupPolicy>
	requires std::is_class_v<Object>
[[nodiscard]] bool Distributor<FObjects...>::SendObjectToStreamImpl(StreamData& streamData, const Object& object,
	const std::function<bool(const FilterBase* filter, const Object& object)>& filterPredicate)
{
	const FilterBase* filter [[indeterminate]];
	auto& connection{ streamData.GetConnection() };
	Streams* streams [[indeterminate]];
	{
		const Lock::AtomicRW::Guard<Lock::read> _{ streamData.GetLock() };
		if (!streamData.IsActive()) [[unlikely]] {
			LOG_PROTOCOL_NEW(
				"Stream id: {} is not active. Connection id: {}", streamData.GetStreamId(), connection.GetId());
			return true;
		}

		filter = streamData.GetFilter();
		streams = &(streamData.GetStreams());
	}

	if (filter == nullptr) [[unlikely]] {
		LOG_ERROR_NEW(
			"Filter is not set for stream id: {}, connection id: {}", streamData.GetStreamId(), connection.GetId());

		if constexpr (CleanupPolicy == CLEANUP_INSIDE) {
			const StreamConnectionId streamConnectionId{ streamData.GetStreamId(), connection.GetId() };
			// Locks inside
			streams->RemoveStream(streamConnectionId);

			{
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
				m_streamConnectionIdToStreamData.erase(streamConnectionId);
			}
		}

		return false;
	}

	LOG_PROTOCOL_NEW(
		"Try to send object to stream id: {} connection id: {}", streamData.GetStreamId(), connection.GetId());

	if (filter->GetTotalFilterSize() == 0) {
		if (!Send(connection, { streamData.GetStreamId(), typeid(Object).hash_code(), sizeof(Object) }, &object))
			[[unlikely]] {

			if constexpr (CleanupPolicy == CLEANUP_INSIDE) {
				const StreamConnectionId streamConnectionId{ streamData.GetStreamId(), connection.GetId() };
				// Locks inside
				streams->RemoveStream(streamConnectionId);

				{
					const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
					m_streamConnectionIdToStreamData.erase(streamConnectionId);
				}
			}

			return false;
		}

		return true;
	}

	if (filterPredicate(filter, object)
		&& !Send(connection, { streamData.GetStreamId(), typeid(Object).hash_code(), sizeof(Object) }, &object))
		[[unlikely]] {

		if constexpr (CleanupPolicy == CLEANUP_INSIDE) {
			const StreamConnectionId streamConnectionId{ streamData.GetStreamId(), connection.GetId() };
			// Locks inside
			streams->RemoveStream(streamConnectionId);

			{
				const Lock::AtomicRW::Guard<Lock::write> _{ m_streamConnectionIdToStreamDataLock };
				m_streamConnectionIdToStreamData.erase(streamConnectionId);
			}
		}

		return false;
	}

	return true;
}

} // namespace Object

} // namespace Protocol

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_OBJECT_INL