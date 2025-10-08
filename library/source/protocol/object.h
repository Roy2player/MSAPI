/**************************
 * @file        object.h
 * @version     6.0
 * @date        2023-08-29
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 *
 * @brief Works in paradigm of streams and filters. Stream has one custom filter and distributor must know how to react
 * on this filter. Filter can has multiple custom objects to filtration. Stream can be opened with different types:
 * snapshot - get all currently available objects and snapshot and live - get all currently available and all new
 * objects while stream is open. Stream has callbacks about states: opened, snapshot done, failed and object handle.
 * Client must set connection for stream to mark who is the distributor.
 *
 * @brief Undefined state - default state, can be right after stream created;
 * @brief Pending state - stream is waiting for answer right after stream opened;
 * @brief Opened state - stream is in active state;
 * @brief Done state - stream is in active state and got snapshot of data;
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
 * stream id, connection } for identify stream.
 *
 * @todo Filters can be || and &&
 * @todo Stream can has different filters
 * @todo typeid.hash_code() should be replaced with custom hash function
 */

#ifndef MSAPI_OBJECT_PROTOCOL_H
#define MSAPI_OBJECT_PROTOCOL_H

#include "../help/diagnostic.h"
#include "../help/identifier.h"
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

namespace MSAPI {

class Application;

namespace ObjectProtocol {

enum class Type : int16_t { Undefined, Snapshot, SnapshotAndLive, Max };
enum class State : int16_t { Undefined, Pending, Opened, Done, Failed, Closed, Removed, Max };
enum class Issue : int16_t {
	Undefined,
	Empty,
	NotUniqueFilter,
	ReservedFilterObjectWithoutFilter,
	UnknownFilterObjectHash,
	UnknownHash,
	BadVariantAccess,
	ExtraFilterObject,
	Max
};

/**************************
 * @return Description of object protocol stream type enum.
 *
 * @example Undefined, Snapshot, Snapshot and live, Max.
 */
std::string_view EnumToString(ObjectProtocol::Type value);

/**************************
 * @return Description of object protocol stream state enum.
 *
 * @example Undefined, Pending, Opened, Done, Staled, Failed, Closed, Removed, Max.
 */
std::string_view EnumToString(ObjectProtocol::State value);

/**************************
 * @return Description of object protocol stream issue enum.
 *
 * @example Undefined, Empty, Not unique filter, Reserved filter object without filter, Unknown filter object hash,
 * Unknown hash, Max.
 */
std::string_view EnumToString(ObjectProtocol::Issue value);

/**************************
 * @brief Structure for provide stream state.
 *
 * @todo Probably can be removed and State enum can be used instead.
 */
struct StreamStateResponse {
	State state{ State::Undefined };
	Issue issue{ Issue::Empty };
};

/**************************
 * @brief Structure for HandleNewStreamOpened callback.
 */
struct StreamData {
	const int connection{ 0 };
	Type type{ Type::Undefined };
	bool open{ false };
	size_t objectHash{ 0 };
	size_t filterSize{ 0 };

	/**************************
	 * @example Stream data:
	 * {
	 * 			connection 			: 3
	 * 			type	   			: Snapshot
	 * 			open	   			: true
	 * 			filter object hash  : 123456789
	 * 			filter size         : 3
	 * }
	 */
	std::string ToString() const;
};

/**************************
 * @brief General object for transferring data in stream.
 */
class Data : public DataHeader {
private:
	int m_streamId;
	size_t m_hash;

public:
	/**************************
	 * @brief Create object for transfer data in stream, update buffer size.
	 *
	 * @param streamId Stream id for which object is created. Stream id is not required if communication does not
	 * involve streams.
	 * @param hash Hash of object.
	 * @param size Size of object.
	 *
	 * @test Has unit test.
	 *
	 * @todo Move stream_id in separated StreamData object
	 */
	Data(const int streamId, const size_t hash, const size_t size)
		: DataHeader{ 2666999999 }
		, m_streamId{ streamId }
		, m_hash{ hash }
	{
		m_bufferSize += sizeof(size_t) + sizeof(int) + size;
	}

	/**************************
	 * @brief Construct a new Data object from buffer, copy stream id and hash from it.
	 *
	 * @attention Buffer must be at least 28 bytes long, otherwise undefined behaviour.
	 *
	 * @tparam T DataHeader.
	 *
	 * @param header Data header.
	 * @param buffer Buffer with data.
	 *
	 * @test Has unit test.
	 */
	template <typename T>
	Data(T&& header, const void* buffer)
		requires std::is_same_v<std::decay_t<T>, DataHeader>
		: DataHeader{ std::forward<T>(header) }
	{
		memcpy(&m_streamId, static_cast<const int8_t*>(buffer) + sizeof(size_t) * 2, sizeof(int));
		memcpy(&m_hash, static_cast<const int8_t*>(buffer) + sizeof(size_t) * 2 + sizeof(int), sizeof(size_t));
	}

	/**************************
	 * @return Hash of object.
	 *
	 * @test Has unit test.
	 */
	size_t GetHash() const;

	/**************************
	 * @return True if object is valid, means, object data cipher is correct.
	 *
	 * @test Has unit test.
	 */
	bool IsValid() const;

	/**************************
	 * @return Stream id for which object is related.
	 *
	 * @test Has unit test.
	 */
	int GetStreamId() const;

	/**************************
	 * @brief Pack data before sending in stream.
	 *
	 * @attention Buffer should be freed after using.
	 *
	 * @param data Data for packing.
	 *
	 * @return Packed data for sending.
	 *
	 * @test Has unit test.
	 *
	 * @todo Think how to align data to fast reading from buffer.
	 */
	void* PackData(const void* data) const;

	/**************************
	 * @brief Unpack data after receiving from stream.
	 *
	 * @param ptr Pointer to unpacked data.
	 * @param buffer Buffer for packed data.
	 *
	 * @test Has unit test.
	 */
	static void UnpackData(void** ptr, void* buffer);

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
	 * 			hash	    : 123456789
	 * 			stream id   : 123
	 * }
	 *
	 * @test Has unit test.
	 */
	std::string ToString() const;

	/**************************
	 * @return True if all unit tests are passed and false otherwise.
	 */
	static bool UNITTEST();
};

/**************************
 * @brief Send object for particular stream.
 *
 * @param connection Connection for which object is sent.
 * @param data Data for sending
 * @param object Object for sending.
 */
void Send(int connection, const Data& data, const void* object);

class IHandlerBase;

/**************************
 * @brief Common virtual class for all specific streams, contains container with all stream' ids. Stream id is unique
 * for application which owned that stream.
 */
class StreamBase : public Identifier {
private:
	static std::set<int> m_streamIds;

protected:
	int m_connection{ 0 };
	State m_state{ State::Undefined };
	bool m_snapshotDone{ false };

public:
	/**************************
	 * @brief Construct a new Stream Base object, generate unique id for stream.
	 */
	StreamBase();

	/**************************
	 * @brief Destroy the Stream Base object and remove stream id from container.
	 */
	~StreamBase();

	/**************************
	 * @return True if stream snapshot is done.
	 */
	bool IsSnapshotDone() const;

	/**************************
	 * @return State of stream.
	 */
	State GetState() const;

	/**************************
	 * @return Stream's source connection.
	 */
	int GetConnection() const;

	/**************************
	 * @return True if stream connection is not set.
	 */
	bool Empty() const;

	/**************************
	 * @brief Set source connection for opening stream.
	 *
	 * @param connection Connection to set.
	 */
	void SetConnection(int connection);

protected:
	/**************************
	 * @brief Set state of stream.
	 *
	 * @param state State to set.
	 */
	void SetState(State state);

	//* For encapsulate protected function
	friend IHandlerBase;
};

/**************************
 * @brief Class for IHandler and Distributor Collect functions, provides opportunity to check application state.
 */
class ApplicationStateChecker {
private:
	const MSAPI::Application* m_application;

public:
	/**************************
	 * @brief Construct a new Application State Checker object, empty constructor.
	 *
	 * @param application Readable pointer to application object.
	 */
	ApplicationStateChecker(const MSAPI::Application* application);

	/**************************
	 * @return True if application state is Running.
	 */
	bool CheckApplicationState() const;
};

/**************************
 * @brief Common virtual class for all specific handlers, contains general callbacks and container with related
 * streams.
 */
class IHandlerBase {
private:
	std::map<int, StreamBase*> m_streamToId;

public:
	/**************************
	 * @brief Destroy the IHandlerBase object, empty destructor.
	 */
	virtual ~IHandlerBase() = default;

	/**************************
	 * @brief Callback when stream opened successfully.
	 *
	 * @param streamId Stream id for which callback is called.
	 */
	virtual void HandleStreamOpened(int streamId) = 0;

	/**************************
	 * @brief Callback when stream got snapshot of data.
	 *
	 * @param streamId Stream id for which callback is called.
	 */
	virtual void HandleStreamSnapshotDone(int streamId) = 0;

	/**************************
	 * @brief Callback when any error occurred on distributor side, reopen action is required.
	 *
	 * @param streamId Stream id for which callback is called.
	 */
	virtual void HandleStreamFailed(int streamId) = 0;

	/**************************
	 * @return Readable link for container with all streams to their ids.
	 */
	const std::map<int, StreamBase*>& GetStreamsContainer() const;

	/**************************
	 * @brief Set the Stream object for stream id.
	 *
	 * @param streamId Stream id for which stream is set.
	 * @param stream Stream base object to set.
	 */
	void SetStream(int streamId, StreamBase* stream);

	/**************************
	 * @brief Remove stream id from container.
	 *
	 * @param streamId Stream id to remove.
	 */
	void RemoveStream(int streamId);

	/**************************
	 * @brief Collect stream state from stream id.
	 *
	 * @attention If distributor gets StreamStateResponse you should call this function.
	 *
	 * @param streamId Stream id for which state is collected.
	 * @param state Stream state to collect.
	 */
	void CollectStreamState(int streamId, const StreamStateResponse* state);
};

/**************************
 * @brief Class for specific object handler.
 *
 * @attention It is required to provide pointer to application for ApplicationStateChecker class in Application
 * constructor.
 *
 * @tparam T Object type for which handler is created.
 */
template <typename T>
	requires std::is_class_v<T>
class IHandler : virtual public IHandlerBase, virtual protected ApplicationStateChecker {
public:
	/**************************
	 * @brief Handler for stream object.
	 *
	 * @param streamId Stream id for which callback is called.
	 * @param object Object for which callback is called.
	 */
	virtual void HandleObject(int streamId, const T& object) = 0;

	/**************************
	 * @brief Collect data object from stream and manage call callback function.
	 *
	 * @note If reserved message from unknown stream id it will be rejected.
	 *
	 * @param data Data object for collect.
	 * @param object Object for collect.
	 */
	void Collect(const Data& data, const void* object)
	{
		if (CheckApplicationState()) {
			LOG_PROTOCOL("Collect data: " + data.ToString());

			const auto streamId{ data.GetStreamId() };
			const auto& streamToId = GetStreamsContainer();
			if (const auto it = streamToId.find(streamId); it == streamToId.end()) {
				LOG_WARNING("Unknown stream id: " + _S(streamId));
				return;
			}
			const auto hash{ data.GetHash() };
			if (hash == typeid(T).hash_code()) {
				HandleObject(streamId, *reinterpret_cast<const T*>(object));
				return;
			}

			LOG_ERROR("Unknown hash: " + _S(hash));
			return;
		}

		LOG_PROTOCOL("Application state is Paused, collect data: " + data.ToString());
	}
};

/**************************
 * @brief Base class for all filters, contain general filter information.
 *
 * @attention Filter object can't be removed from filter, need to create new filter object.
 *
 * @todo I'm not sure if filter base still required class. Need to pay attention on it during implementation of
 * multiple filters for one stream.
 */
class FilterBase {
private:
	Type m_type{ Type::Undefined };
	size_t m_filterSize{ 0 };
	size_t m_streamObjectHash{ 0 };

public:
	/**************************
	 * @brief Construct a new empty Filter Base object, empty constructor.
	 *
	 * @param type Stream type to set.
	 *
	 * @todo Validation for type.
	 * @todo Probably better set type in stream but not in Filter. And stream should set their type in Filter during
	 * opening action.
	 */
	FilterBase(Type type);

	/**************************
	 * @brief Construct a new empty Filter Base object, empty constructor.
	 *
	 * @attention Type will be UndefinedType and should be set manually.
	 */
	FilterBase() = default;

	/**************************
	 * @brief Destroy the Filter Base object, empty destructor.
	 */
	virtual ~FilterBase() = default;

	/**************************
	 * @return Number of objects in filter.
	 */
	size_t GetFilterSize() const;

	/**************************
	 * @return True if filter is empty, means - filter has not objects.
	 */
	bool Empty() const;

	/**************************
	 * @return Type of stream.
	 */
	Type GetType() const;

	/**************************
	 * @brief Set type of stream.
	 *
	 * @param type Type to set.
	 *
	 * @todo Validation for type.
	 */
	void SetType(Type type);

	/**************************
	 * @brief Set hash of stream object.
	 *
	 * @param streamObjectHash Hash to set.
	 *
	 * @todo Should be called automatically after creating specific filter. Because filter didn't know which objects
	 * stream will send.
	 */
	void SetStreamObjectHash(size_t streamObjectHash);

	/**************************
	 * @return Hash of stream object.
	 */
	size_t GetStreamObjectHash() const;

	/**************************
	 * @return Hash of filter object.
	 */
	virtual size_t GetFilterObjectHash() const = 0;

	/**************************
	 * @example Filter base:
	 * {
	 * 			type	   : Snapshot
	 * 			obj. hash  : 123456789
	 * 			filt. size : 3
	 * }
	 */
	std::string ToString() const;

protected:
	/**************************
	 * @brief Increment filter size.
	 */
	void IncrementFilterSize();
};

/**************************
 * @brief Class for specific object filter, includes filter object hash and container with filter objects.
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
	size_t m_hash{ typeid(T).hash_code() };
	std::vector<T> m_objects;

public:
	/**************************
	 * @brief Construct a new Filter object, empty constructor.
	 *
	 * @param type Stream type to set.
	 */
	Filter(Type type)
		: FilterBase(type)
	{
	}

	/**************************
	 * @brief Default construct a new Filter object, empty constructor.
	 *
	 * @attention Type will be UndefinedType and should be set manually.
	 */
	Filter() = default;

	/**************************
	 * @brief Construct a new Filter object, copy constructor from base filter.
	 *
	 * @param filter Filter to copy.
	 */
	Filter(const FilterBase& filter)
		: FilterBase(filter)
	{
	}

	/**************************
	 * @brief Set the Filter Object.
	 *
	 * @param object Object to set.
	 */
	void SetObject(const T& object)
	{
		m_objects.emplace_back(object);
		IncrementFilterSize();
	}

	/**************************
	 * @return Readable link for container with filter objects.
	 */
	const std::vector<T>& GetObjects() const { return m_objects; }

	/**************************
	 * @return Hash of filter object.
	 */
	size_t GetFilterObjectHash() const final { return m_hash; }

	/**************************
	 * @example Filter special:
	 * {
	 * 			obj. hash   : 123456789
	 * 			filt. size  : 3
	 * 			Base filter : Filter base:
	 * {
	 * ...
	 * }
	 * }
	 *
	 * @return std::string
	 */
	std::string ToString() const
	{
		std::stringstream stream;

#define format std::left << std::setw(11)

		stream << std::fixed << std::setprecision(16) << "Filter special:\n{\n\t" << format << "obj. hash" << " : "
			   << _S(m_hash) << "\n\t" << format << "filt. size" << " : " << _S(m_objects.size()) << "\n\t" << format
			   << "Base filter" << " :\n"
			   << FilterBase::ToString() << "\n}";

#undef format

		return stream.str();
	}

private:
	/**************************
	 * @return Readable pointer to base filter.
	 */
	const FilterBase* GetBase() const { return this; }

	/**************************
	 * @brief Internal function for set filter object inside distributor.
	 *
	 * @param object Object to set.
	 */
	void SetObjectInternal(const T& object) { m_objects.emplace_back(object); }

	template <typename T1, typename F>
		requires std::is_class_v<T1> && std::is_class_v<F>
	friend class Stream;

	template <typename... Ts>
		requires(std::is_class_v<Ts> && ...)
	friend class Distributor;
};

/**************************
 * @brief Main class for all distributors, contains general information about streams and their filters. Distributor
 * uses key pair { stream id, connection } for identify stream.
 *
 * @attention Note which objects the distributor can send in doxygen annotation.
 * @attention Call Stop() function before stop the server is required.
 * @attention It is required to provide pointer to application for ApplicationStateChecker class in Application
 * constructor.
 * @attention If application got Paused state, it should call Stop() function for informing all active streams about it.
 *
 * @tparam Ts Types of filters which distributor can handle.
 */
template <typename... Ts>
	requires(std::is_class_v<Ts> && ...)
class Distributor : virtual protected ApplicationStateChecker {
protected:
	//* { { stream id client, connection }, stream data } }
	std::map<std::pair<int, int>, StreamData> m_streamDataToIdAndConnection;
	//* { { stream id client, connection }, stream data } }
	std::multimap<std::pair<int, int>, std::variant<Filter<Ts>...>> m_filtersToStreamIdAndConnection;
	//* { object hash, { stream id, connection } } only for snapshot and live streams
	std::multimap<size_t, std::pair<int, int>> m_activeStreamsToObjectHash;

public:
	/**************************
	 * @brief Default destructor, call Stop() inside.
	 */
	virtual ~Distributor() { Stop(); }

	/**************************
	 * @brief Send Failed state for all opened streams and remove all information about them.
	 */
	void Stop()
	{
		if (m_streamDataToIdAndConnection.empty()) {
			return;
		}
		LOG_PROTOCOL("Removing distributor information about streams");
		auto copyStreamDataToId = m_streamDataToIdAndConnection;
		StreamStateResponse state{ State::Failed };
		for (const auto& [idAndConnection, streamData] : copyStreamDataToId) {
			if (streamData.open) {
				//* Will removed anyway
				//* streamData.open = false;
				ObjectProtocol::Send(streamData.connection,
					{ idAndConnection.first, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) },
					&state);
				// Diagnostic::PrintBinaryDescriptor(data, sizeof(ObjectProtocol::Data), "Right after send");
				RemoveInformationAboutStream(idAndConnection);
			}
		}
	}

	/**************************
	 * @brief Apply client side action for particular stream.
	 *
	 * @param idAndConnection Stream id and connection for which action is applied.
	 * @param response Income stream state response from client.
	 */
	void StreamExternalAction(const std::pair<int, int>& idAndConnection, const StreamStateResponse* response)
	{
		switch (response->state) {
		case State::Closed:
			LOG_PROTOCOL("Client closed stream, id: " + _S(idAndConnection.first)
				+ ", connection: " + _S(idAndConnection.second));
			if (const auto it{ m_streamDataToIdAndConnection.find(idAndConnection) };
				it == m_streamDataToIdAndConnection.end()) {

				LOG_ERROR("Have not stream data for stream id: " + _S(idAndConnection.first)
					+ ", connection: " + _S(idAndConnection.second));
				return;
			}

			RemoveInformationAboutStream(idAndConnection);
			return;
		default:
			LOG_WARNING_NEW("Unexpected stream state for external action: {}, stream id: {}, connection: {}",
				EnumToString(response->state), idAndConnection.first, idAndConnection.second);
			return;
		}
	}

	/**************************
	 * @brief Specific distributor collect function manage two types of income data: Filter and filter object. 1)
	 * When reserved Filter, distributor extract necessary data and wait for filter objects. 2) When filter object
	 * is received, distributor check filter size and if it is equal to filter objects number, distributor open
	 * stream.
	 *
	 * @param connection Connection for which data is collected.
	 * @param data Data for collect.
	 * @param object Object for collect.
	 *
	 * @tparam T Type of filter object which presented in Ts.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename T>
		requires is_included_in<T, Ts...>
	void Collect(const int connection, const Data& data, const void* object)
	{
		if (CheckApplicationState()) {
			LOG_PROTOCOL("Collect data: " + data.ToString() + ", connection: " + _S(connection));
			const auto hash{ data.GetHash() };
			const int streamId{ data.GetStreamId() };
			std::pair<int, int> idAndConnection{ streamId, connection };
			if (typeid(Filter<T>).hash_code() == hash) {
				if (m_filtersToStreamIdAndConnection.find(idAndConnection) != m_filtersToStreamIdAndConnection.end()) {
					LOG_ERROR(
						"Got not unique filter for stream id: " + _S(streamId) + ", connection: " + _S(connection));
					SendFailed(idAndConnection, Issue::NotUniqueFilter);
					return;
				}

				const FilterBase* filter = reinterpret_cast<const FilterBase*>(object);
				const auto streamObjectHash{ filter->GetStreamObjectHash() };
				const auto filterObjectHash{ typeid(T).hash_code() };

				const auto& streamData = m_streamDataToIdAndConnection
											 .emplace(idAndConnection,
												 StreamData{ connection, filter->GetType(), filter->Empty(),
													 streamObjectHash, filter->GetFilterSize() })
											 .first->second;
				m_filtersToStreamIdAndConnection.emplace(
					idAndConnection, std::variant<Filter<Ts>...>(Filter<T>(*filter)));

				if (streamData.open) {
					LOG_PROTOCOL("Instant open stream with empty filter, id: " + _S(streamId)
						+ ", connection: " + _S(connection) + ", hash of stream's object: " + _S(streamObjectHash)
						+ ", hash of filter's object: " + _S(filterObjectHash));
					Open(idAndConnection);
				}
				else {
					LOG_PROTOCOL("Waiting filter's objects for new filter, id: " + _S(streamId)
						+ ", connection: " + _S(connection) + ", hash of stream's object: " + _S(streamObjectHash)
						+ ", hash of filter's object: " + _S(filterObjectHash));
				}
				return;
			}

			if (typeid(T).hash_code() == hash) {
				auto [currentFilter, filtersEnd] = m_filtersToStreamIdAndConnection.equal_range(idAndConnection);
				if (currentFilter == filtersEnd) {
					LOG_ERROR("Reserved filter object without filter for stream id: " + _S(streamId)
						+ ", connection: " + _S(connection));
					SendFailed(idAndConnection, Issue::ReservedFilterObjectWithoutFilter);
					return;
				}

				if (m_filtersToStreamIdAndConnection.count(idAndConnection) > 1) {
					do {
						if (std::holds_alternative<Filter<T>>(currentFilter->second)) {
							break;
						}
						++currentFilter;
					} while (currentFilter != filtersEnd);

					if (currentFilter == filtersEnd) {
						LOG_ERROR("Got filter object with unknown hash: " + _S(hash) + ", stream id: " + _S(streamId)
							+ ", connection: " + _S(connection));
						SendFailed(idAndConnection, Issue::UnknownFilterObjectHash);
						return;
					}
				}
				else if (!std::holds_alternative<Filter<T>>(currentFilter->second)) {
					LOG_ERROR("Got filter object with unknown hash: " + _S(hash) + ", stream id: " + _S(streamId)
						+ ", connection: " + _S(connection));
					SendFailed(idAndConnection, Issue::UnknownFilterObjectHash);
					return;
				}

				Filter<T>* filterPtr = nullptr;
				try {
					filterPtr = &std::get<Filter<T>>(currentFilter->second);
				}
				//* Theoretically should not be happened.
				catch (const std::bad_variant_access&) {
					LOG_ERROR("Unexpected throw bad variant access. Hash: " + _S(hash) + ", stream id: " + _S(streamId)
						+ ", connection: " + _S(connection));
					SendFailed(idAndConnection, Issue::BadVariantAccess);
					return;
				}

				filterPtr->SetObjectInternal(*static_cast<const T*>(object));
				LOG_PROTOCOL("Got filter's object for stream, id: " + _S(streamId) + ", connection: " + _S(connection)
					+ ", filter objects number: " + _S(filterPtr->GetObjects().size())
					+ ", expected: " + _S(filterPtr->GetFilterSize()));
				// TODO: When stream will has multiply filters we should check all of them
				if (filterPtr->GetFilterSize() == filterPtr->GetObjects().size()) {
					Open(idAndConnection);
				}
				else if (filterPtr->GetFilterSize() < filterPtr->GetObjects().size()) {
					LOG_ERROR("Filter has more objects when expected, hash: " + _S(hash)
						+ ", stream id: " + _S(streamId) + ", connection: " + _S(connection));
					SendFailed(idAndConnection, Issue::ExtraFilterObject);
				}
				return;
			}

			SendFailed(idAndConnection, Issue::UnknownHash);
			LOG_ERROR("Got unknown hash " + _S(hash));
			return;
		}

		LOG_PROTOCOL(
			"Application state is Paused, collect data: " + data.ToString() + ", connection: " + _S(connection));
	}

	/**************************
	 * @brief Send old objects for particular stream.
	 *
	 * @param streamId Stream id for which objects are sent.
	 * @param objects Objects for sending.
	 * @param filterPredicate Predicate for filter.
	 *
	 * @tparam T Type of container with objects.
	 * @tparam S Type of object to send.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <template <typename> typename T, typename S>
		requires std::is_class_v<S>
	void SendOldObjects(const int streamId, const StreamData& streamData, const T<S>& objects,
		const std::function<bool(const FilterBase* filter, const S& object)>& filterPredicate) const
	{
		LOG_PROTOCOL("Try to send old objects for stream id: " + _S(streamId)
			+ ", connection: " + _S(streamData.connection) + ", objects number: " + _S(objects.size()));
		for (const auto& object : objects) {
			Send(streamId, object, streamData, filterPredicate);
		}
	}

	/**************************
	 * @brief Send old object for particular stream.
	 *
	 * @param streamId Stream id for which object is sent.
	 * @param object Object for sending.
	 * @param filterPredicate Predicate for filter.
	 *
	 * @tparam T Type of object to send.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename T>
		requires std::is_class_v<T>
	void SendOldObject(const int streamId, const StreamData& streamData, const T& object,
		const std::function<bool(const FilterBase* filter, const T& object)>& filterPredicate) const
	{
		LOG_PROTOCOL(
			"Try to send old object for stream id: " + _S(streamId) + ", connection: " + _S(streamData.connection));
		Send(streamId, object, streamData, filterPredicate);
	}

	/**************************
	 * @brief Send new object for all active streams.
	 *
	 * @param object Object for sending.
	 * @param filterPredicate Predicate for filter.
	 *
	 * @tparam T Type of object to send.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename T>
		requires std::is_class_v<T>
	void SendNewObject(
		const T& object, const std::function<bool(const FilterBase* filter, const T& object)> filterPredicate) const
	{
		LOG_PROTOCOL("Searching subscribers for hash: " + _S(typeid(T).hash_code()));
		auto [currentActiveStreamIt, endActiveStreamIt]
			= m_activeStreamsToObjectHash.equal_range(typeid(T).hash_code());
		if (currentActiveStreamIt == endActiveStreamIt) {
			LOG_PROTOCOL("Have not any active stream for hash: " + _S(typeid(T).hash_code()));
			return;
		}
		bool send{ false };
		while (currentActiveStreamIt != endActiveStreamIt) {
			if (auto [currentFilterIt, endFilterIt]
				= m_filtersToStreamIdAndConnection.equal_range(currentActiveStreamIt->second);
				currentFilterIt != endFilterIt) {

				while (currentFilterIt != endFilterIt) {
					std::visit(
						[&send, &filterPredicate, &object](auto&& filter) {
							if (filter.Empty() || filterPredicate(filter.GetBase(), object)) {
								send = true;
							}
						},
						currentFilterIt->second);

					if (send) {
						break;
					}
					++currentFilterIt;
				}
			}
			else {
				LOG_WARNING("Not fount any filter for stream id: " + _S(currentActiveStreamIt->second.first)
					+ ", connection: " + _S(currentActiveStreamIt->second.second));
			}

			if (send) {
				if (const auto it = m_streamDataToIdAndConnection.find(currentActiveStreamIt->second);
					it != m_streamDataToIdAndConnection.end()) {

					ObjectProtocol::Send(it->second.connection,
						{ currentActiveStreamIt->second.first, it->second.objectHash, sizeof(T) }, &object);
				}
				else {
					LOG_ERROR("Didn't find data for stream id: " + _S(currentActiveStreamIt->second.first)
						+ ", connection: " + _S(currentActiveStreamIt->second.second));
				}
				send = false;
			}

			++currentActiveStreamIt;
		}
	}

private:
	/**************************
	 * @brief Callback about new stream opened action.
	 *
	 * @param streamId Stream id for which callback is called.
	 * @param streamData Stream data for which callback is called.
	 */
	virtual void HandleNewStreamOpened(const int id, const StreamData& streamData) = 0;

	/**************************
	 * @brief Open stream for particular stream id.
	 *
	 * @param id Stream id for which stream is opened.
	 */
	void Open(const std::pair<int, int>& idAndConnection)
	{
		auto it = m_streamDataToIdAndConnection.find(idAndConnection);
		if (it == m_streamDataToIdAndConnection.end()) {
			LOG_PROTOCOL("Didn't find data for stream id: " + _S(idAndConnection.first)
				+ ", connection: " + _S(idAndConnection.second));
			return;
		}
		it->second.open = true;

		const bool onlySnapshot{ it->second.type == Type::Snapshot };
		LOG_PROTOCOL("New stream is opening: " + it->second.ToString() + ", id: " + _S(idAndConnection.first)
			+ ", connection: " + _S(idAndConnection.second) + ", only snapshot: " + _S(onlySnapshot));

		if (!onlySnapshot) {
			LOG_PROTOCOL("Stream id: " + _S(idAndConnection.second) + ", connection: " + _S(idAndConnection.second)
				+ " set as active");
			m_activeStreamsToObjectHash.emplace(it->second.objectHash, idAndConnection);
		}

		StreamStateResponse state{ State::Opened };
		const Data data{ idAndConnection.first, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) };
		ObjectProtocol::Send(it->second.connection, data, &state);
		HandleNewStreamOpened(idAndConnection.first, it->second);
		state.state = State::Done;
		ObjectProtocol::Send(it->second.connection, data, &state);
		if (onlySnapshot) {
			state.state = State::Closed;
			ObjectProtocol::Send(it->second.connection, data, &state);
			RemoveInformationAboutStream(idAndConnection);
		}
	}

	void SendFailed(const std::pair<int, int>& idAndConnection, const Issue issue)
	{
		const auto it = m_streamDataToIdAndConnection.find(idAndConnection);
		if (it == m_streamDataToIdAndConnection.end()) {
			LOG_PROTOCOL("Try to send failed for unknown stream, id: " + _S(idAndConnection.first)
				+ ", connection: " + _S(idAndConnection.second));
			return;
		}

		StreamStateResponse state{ State::Failed, issue };
		ObjectProtocol::Send(it->second.connection,
			{ idAndConnection.first, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &state);
		RemoveInformationAboutStream(idAndConnection);
	}

	/**************************
	 * @brief Send object for particular stream.
	 *
	 * @param id Stream id for which object is sent.
	 * @param object Object for sending.
	 * @param filterPredicate Predicate for filter.
	 *
	 * @tparam T Type of object to send.
	 *
	 * @todo Need to add support for multiple filters for one stream.
	 */
	template <typename T>
		requires std::is_class_v<T>
	void Send(const int id, const T& object, const StreamData& streamData,
		const std::function<bool(const FilterBase* filter, const T& object)>& filterPredicate) const
	{
		bool send{ false };
		if (auto [currentFilter, filterEnd]
			= m_filtersToStreamIdAndConnection.equal_range({ id, streamData.connection });
			currentFilter != filterEnd) {

			do {
				std::visit(
					[&send, &filterPredicate, &object](auto&& filter) {
						if (filter.Empty() || filterPredicate(filter.GetBase(), object)) {
							send = true;
						}
					},
					currentFilter->second);

				if (send) {
					break;
				}
				++currentFilter;
			} while (currentFilter != filterEnd);
		}
		else {
			LOG_WARNING("Not fount any filter for stream id: " + _S(id) + ", connection: " + _S(streamData.connection));
		}

		if (send) {
			ObjectProtocol::Send(streamData.connection, { id, streamData.objectHash, sizeof(T) }, &object);
		}
	}

	/**************************
	 * @brief Remove all related information about stream.
	 *
	 * @param id Stream id for which information is removed.
	 */
	void RemoveInformationAboutStream(const std::pair<int, int>& idAndConnection)
	{
		const auto it = m_streamDataToIdAndConnection.find(idAndConnection);
		if (it == m_streamDataToIdAndConnection.end()) {
			LOG_PROTOCOL("Try to remove information about unknown stream, id: " + _S(idAndConnection.first)
				+ ", connection: " + _S(idAndConnection.second));
			return;
		}

		LOG_PROTOCOL("Remove information about stream, id: " + _S(idAndConnection.first)
			+ ", connection: " + _S(idAndConnection.second));

		auto [current, end] = m_activeStreamsToObjectHash.equal_range(it->second.objectHash);
		while (current != end) {
			if (current->second == idAndConnection) {
				m_activeStreamsToObjectHash.erase(current);
				break;
			}

			++current;
		}

		m_streamDataToIdAndConnection.erase(idAndConnection);
		m_filtersToStreamIdAndConnection.erase(idAndConnection);
	}
};

/**************************
 * @brief Class for specific object stream, contains handler for callbacks and filter for stream.
 *
 * @tparam T Type of stream object.
 * @tparam F Type of stream filter object.
 */
template <typename T, typename F>
	requires std::is_class_v<T> && std::is_class_v<F>
class Stream : public StreamBase {
private:
	IHandler<T>* m_handler;
	Filter<F> m_filter;
	bool m_haveFilter{ false };

public:
	/**************************
	 * @brief Construct a new Stream object, set stream id in handler for stream state callbacks.
	 *
	 * @param handler Handler for callbacks.
	 */
	Stream(IHandler<T>* handler)
		: m_handler(handler)
	{
		m_handler->SetStream(m_id, this);
		LOG_PROTOCOL("Client creates stream, id: " + _S(m_id));
	}

	/**************************
	 * @brief Destroy the Stream object, remove stream id from handler and call Close() function.
	 */
	~Stream()
	{
		if (m_state == State::Failed || m_state == State::Closed || m_state == State::Undefined) {
			LOG_DEBUG_NEW("Reject attempt to destroy stream with state: {}, id: {}", EnumToString(m_state), m_id);
			return;
		}
		m_handler->RemoveStream(m_id);
		StreamStateResponse state{ State::Removed };
		Send(m_connection, { m_id, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &state);
	}

	/**************************
	 * @brief Set the Filter object, clear snapshot done flag and call Close() function if stream is opened.
	 *
	 * @param filter Filter to set.
	 */
	void SetFilter(const Filter<F>& filter)
	{
		m_snapshotDone = false;
		m_filter = filter;
		m_filter.SetStreamObjectHash(typeid(T).hash_code());
		m_haveFilter = true;
		LOG_PROTOCOL("Client sets filter for stream, id: " + _S(m_id) + ", filter: " + m_filter.ToString());
		//* Close stream if we update filter for working stream
		if (m_state == State::Opened || m_state == State::Pending) {
			Close();
		}
	}

	/**************************
	 * @brief Open stream if it is closed: failed or undefined stare. Required to set distributor connection.
	 *
	 * @return True if stream is opened, false if stream empty, if does not filter or any issue happen.
	 */
	bool Open()
	{
		if (Empty()) {
			LOG_WARNING("Client tries to open stream without connection, id: " + _S(m_id));
			return false;
		}
		if (m_state == State::Opened || m_state == State::Pending) {
			LOG_WARNING_NEW("Reject attempt to open stream with state: {}, id: {}", EnumToString(m_state), m_id);
			return false;
		}
		if (!m_haveFilter) {
			LOG_WARNING("Try to open stream without filter, id: " + _S(m_id));
			return false;
		}
		m_state = State::Pending;
		LOG_PROTOCOL("Client opens stream, id: " + _S(m_id) + ", filter: " + m_filter.ToString());

		//* First we send base filter options
		Send(m_connection, { m_id, typeid(Filter<F>).hash_code(), sizeof(FilterBase) }, m_filter.GetBase());
		//* Next we send all filter objects
		const auto filterObjectHash{ typeid(F).hash_code() };
		for (const auto& item : m_filter.GetObjects()) {
			Send(m_connection, { m_id, filterObjectHash, sizeof(F) }, &item);
		}
		return true;
	}

	/**************************
	 * @brief Close stream if it is active, clear snapshot done flag and set Closed state.
	 */
	void Close()
	{
		if (m_state != State::Opened && m_state != State::Pending) {
			LOG_DEBUG_NEW("Reject attempt to close stream with state: {}, id: {}", EnumToString(m_state), m_id);
			return;
		}
		LOG_PROTOCOL("Client closes stream, id: " + _S(m_id));
		m_snapshotDone = false;
		m_state = State::Closed;
		Send(m_connection, { m_id, typeid(StreamStateResponse).hash_code(), sizeof(StreamStateResponse) }, &m_state);
	}
};

}; //* namespace ObjectProtocol

}; //* namespace MSAPI

#endif //* MSAPI_OBJECT_PROTOCOL_H