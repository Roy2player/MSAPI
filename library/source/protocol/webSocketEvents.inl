/**************************
 * @file        webSocketEvents.inl
 * @date        2026-04-10
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
 * @brief Events protocol - parallel execution distributing model which works on top of web socket protocol with json
 * payload, supports single and stream events with filters. Uses MSAPI Authorization to control data access rights.
 *
 * @note The concurrency model prioritizes fast event registration and removal over distribution throughput.
 */

#ifndef MSAPI_PROTOCOL_WEB_SOCKET_EVENTS_INL
#define MSAPI_PROTOCOL_WEB_SOCKET_EVENTS_INL

#include "../help/json.h"
#include "../server/authorization.inl"
#include "webSocket.inl"

namespace MSAPI {

namespace Protocol {

namespace WebSocket {

namespace Events {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Event communication model types.
 */
enum class Type : int8_t { Undefined, Single, Stream, Max };

/**************************
 * @return String interpretation of event type enum.
 *
 * @locking Is not required.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(Type type) noexcept;

/**************************
 * @brief Result of handling an event.
 */
enum class HandleResult : int8_t {
	Undefined,
	Success,
	Fail,
	// Single-event handlers can postpone their response.
	Delay,
	// Stream handlers send snapshot chunks directly; the output buffer is not sent.
	SuccessChunked,
	IOError,
	Max
};

/**************************
 * @return String interpretation of handle result enum.
 *
 * @locking Is not required.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(HandleResult result) noexcept;

/**************************
 * @brief Reason for removing a registered event.
 */
enum class TerminationReason : int8_t {
	Undefined,
	ClientInterrupted,
	ConnectionClosed,
	Cleanup,
	LimitExceeded,
	RemovedByFilter,
	// Applies to failure of registered events by filter; initialization failure does not notify termination.
	Failed,
	// Response processing completed locally; no peer acknowledgment is implied.
	Completed,
	Max
};

/**************************
 * @brief Returns the string representation of a termination reason and logs unknown values.
 *
 * @param reason Termination reason.
 *
 * @locking Is not required.
 *
 * @return Enumerator name, or "Unknown" if the value is not recognized.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(TerminationReason reason) noexcept;

/**************************
 * @brief Send return result.
 */
enum class SendResult : int8_t { Undefined, Success, Fail, Nothing, IOError, Max };

/**************************
 * @return String interpretation of send result enum.
 *
 * @locking Is not required.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(SendResult result) noexcept;

/**************************
 * @brief Send failed event state with error.
 *
 * @attention Error text is inserted into JSON unchanged. It must not contain double quotes, backslashes, or ASCII
 * control characters (U+0000 through U+001F).
 *
 * @param id Event id.
 * @param connection Connection of request.
 * @param error Description of the failure.
 *
 * @locking Is not required.
 *
 * @return True if the failure frame is sent, false on transport failure.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool SendFailed(uint64_t id, Connection& connection, std::string_view error);

/**************************
 * @brief Event holder, contains common data and handler function.
 *
 * @tparam EventType Type of an event.
 *
 * @concurrency No.
 */
template <typename EventType> class Event {
public:
	using HandlerT = std::function<HandleResult(std::string&, const EventType&)>;
	using TerminationHandlerT = std::function<void(const EventType&, TerminationReason)>;

	/**************************
	 * @brief Handler and access requirements holder.
	 *
	 * @concurrency No.
	 */
	class HandlerData {
	private:
		const HandlerT m_handler;
		const TerminationHandlerT m_terminationHandler;
		const int16_t m_grade;
		const bool m_isPermissionRequired;

	public:
		/**************************
		 * @brief Constructs a handler with permission requirements.
		 *
		 * @param handler Event type specific handler.
		 * @param terminationHandler Event type specific termination handler.
		 * @param grade Minimal required grade.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit HandlerData(
			HandlerT&& handler, TerminationHandlerT&& terminationHandler, int16_t grade) noexcept;

		/**************************
		 * @brief Constructs a handler without permission requirements.
		 *
		 * @param handler Event type specific handler.
		 * @param terminationHandler Event type specific termination handler.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit HandlerData(HandlerT&& handler, TerminationHandlerT&& terminationHandler) noexcept;

		HandlerData(const HandlerData&) = delete;
		HandlerData(HandlerData&&) = delete;
		HandlerData& operator=(const HandlerData&) = delete;
		HandlerData& operator=(HandlerData&&) = delete;

		/**************************
		 * @return Minimal required grade, meaningful only if permission is required.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] int16_t GetGrade() const noexcept;

		/**************************
		 * @return True if permission is required to handle the event, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsPermissionRequired() const noexcept;

		/**************************
		 * @return Event type specific handler.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const HandlerT& GetHandler() const noexcept;

		/**************************
		 * @return Event type specific termination handler.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const TerminationHandlerT& GetTerminationHandler() const noexcept;
	};

	using HandlerDataT = HandlerData;

private:
	const uint64_t m_id;
	std::shared_ptr<HandlerData> m_handlerData;
	const Json m_json;
	std::shared_ptr<Connection::Data> m_connectionData;

public:
	/**************************
	 * @brief Construct event holder.
	 *
	 * @param id Event id.
	 * @param handlerData Specific handler and access requirements holder.
	 * @param json Request json.
	 * @param connectionData Connection data of request.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE Event(uint64_t id, std::shared_ptr<HandlerData>&& handlerData, Json&& json,
		const std::shared_ptr<Connection::Data>& connectionData) noexcept;

	Event(const Event&) = delete;
	FORCE_INLINE Event(Event&&) = default;
	Event& operator=(const Event&) = delete;
	FORCE_INLINE Event& operator=(Event&&) = default;

	/**************************
	 * @return Id of event.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetId() const noexcept;

	/**************************
	 * @return Request json.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] const Json& GetJson() const noexcept;

	/**************************
	 * @return Connection data of request.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] const std::shared_ptr<Connection::Data>& GetConnectionData() const noexcept;

private:
	/**************************
	 * @brief Handle event by the specific handler.
	 *
	 * @param payload Preformed payload, is expected to be a valid json value in case of success, only error message on
	 * fail.
	 *
	 * @return Value of handler return result type.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] HandleResult Handle(std::string& payload);

	/**************************
	 * @brief Handle event termination by the specific handler after it has been removed from the registry.
	 *
	 * @note Callbacks from concurrent removals may overlap; applications synchronize their shared callback state.
	 *
	 * @param reason Reason for removal.
	 *
	 * @pre The caller retains ownership of the removed event until this call returns and invokes this function exactly
	 * once, outside registry locks.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void HandleTermination(TerminationReason reason) const noexcept;

	// Allows distributors to dispatch initial handlers and notify termination after removing registry entries.
	template <typename, typename, typename, typename> friend class Distributor;
};

/**************************
 * @brief Filter by event identity.
 */
class IdentityFilter {
private:
	uint64_t identity;

public:
	/**************************
	 * @brief Create empty filter.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE IdentityFilter() noexcept;

	/**************************
	 * @brief Create filter.
	 *
	 * @param identity Event identity.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE explicit IdentityFilter(uint64_t identity) noexcept;

	/**************************
	 * @brief Create filter from json. Filter is expected to be under "filter" key with unsigned type, empty filter is
	 * created by default.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE explicit IdentityFilter(const Json& json) noexcept;

	FORCE_INLINE IdentityFilter(const IdentityFilter&) noexcept = default;
	FORCE_INLINE IdentityFilter(IdentityFilter&&) noexcept = default;
	FORCE_INLINE IdentityFilter& operator=(const IdentityFilter&) noexcept = default;
	FORCE_INLINE IdentityFilter& operator=(IdentityFilter&&) noexcept = default;

	/**************************
	 * @return True if values are equal.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] bool operator==(IdentityFilter other) const noexcept;

	/**************************
	 * @return True if self value is less.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] bool operator<(IdentityFilter other) const noexcept;

	/**************************
	 * @return True if self value is greater.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] bool operator>(IdentityFilter other) const noexcept;
};

/**************************
 * @brief The representation of the request with only one response, which can be delayed and responded later.
 */
class Single : public Event<Single> {
public:
	using BaseT = Event<Single>;

public:
	/**************************
	 * @brief Construct single event holder.
	 *
	 * @param id Event id.
	 * @param handlerData Specific handler and access requirements holder.
	 * @param json Request json.
	 * @param connectionData Connection data of request.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE Single(uint64_t id, std::shared_ptr<HandlerData>&& handlerData, Json&& json,
		const std::shared_ptr<Connection::Data>& connectionData) noexcept;

	Single(const Single&) = delete;
	FORCE_INLINE Single(Single&&) noexcept = default;
	Single& operator=(const Single&) = delete;
	FORCE_INLINE Single& operator=(Single&&) noexcept = default;
};

/**************************
 * @brief The representation of the request with many responses. First call of handler will be addressed to just opened
 * stream. In places, where additional data can be sent, all related opened streams will be notified with it.
 *
 * Opened means initialization starts, not that it succeeds. The synchronous callback either fills the prefixed output
 * and returns Success, or calls SendData repeatedly and returns SuccessChunked. Both successes register the stream
 * before Done. Done means the initial callback and all initial snapshot sends finish. Failed does not retract chunks
 * already delivered; the client handles or discards partial application data.
 *
 * Chunking avoids constructing and parsing one large JSON message and lets the application choose chunk sizes. It can
 * increase message counts.
 *
 * @concurrency No.
 */
class Stream : public Event<Stream> {
public:
	using BaseT = Event<Stream>;

	/**************************
	 * @brief State of stream.
	 */
	enum class State : int8_t { Undefined, Pending, Opened, Done, Failed, Closed, Removed, Max };

public:
	/**************************
	 * @brief Construct stream event holder.
	 *
	 * @param id Event id.
	 * @param handlerData Specific handler and access requirements holder.
	 * @param json Request json.
	 * @param connectionData Connection data of request.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE Stream(uint64_t id, std::shared_ptr<HandlerData>&& handlerData, Json&& json,
		const std::shared_ptr<Connection::Data>& connectionData) noexcept;

	Stream(const Stream&) = delete;
	FORCE_INLINE Stream(Stream&&) noexcept = default;
	Stream& operator=(const Stream&) = delete;
	FORCE_INLINE Stream& operator=(Stream&&) noexcept = default;

	/**************************
	 * @brief Send new stream state.
	 *
	 * @param state New state.
	 *
	 * @return True if the state frame is sent, false on transport failure.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SendState(State state) const;

	/**************************
	 * @brief Sends one valid JSON value only to this stream.
	 *
	 * @param data Valid JSON value, without the protocol envelope.
	 *
	 * @pre The stream remains alive and the caller does not initiate sends after stream termination.
	 *
	 * @return True if the data frame is sent, false on transport failure.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SendData(std::string_view data) const;

	/**************************
	 * @return String interpretation of stream state enum.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static constexpr std::string_view EnumToString(State state) noexcept;
};

template <typename T, typename S>
concept EventTypePtrT = std::is_same_v<std::decay_t<T>, std::shared_ptr<S>>;

template <typename T, typename S>
concept HandlerT = std::is_convertible_v<T, typename S::BaseT::HandlerT>;

template <typename T>
concept DataT = std::is_same_v<std::string_view, T> || std::is_convertible_v<T, std::function<bool(std::string&)>>;

/**************************
 * @brief Parallel execution distributor, controls access rights to newly collected events before their processing and
 * limits stored events for each connection separately.
 *
 * Contains events in three: connection -> filters -> events.
 * Events storing is limited by the total number of events. If the number of events is exceed the limit, due to the
 * difficulty of data structures and parallelism efficiency access algorithms they are purged in bunch at once. Purging
 * coefficient is controls the bunch size. Puring order is FIFO. Limits can be controlled dynamically. Event though
 * purging process is costly, it affects only the connection which overflow the limit.
 *
 * Real filter is a pair of event type hash and filter.
 *
 * @tparam Module Type of the authorization module.
 * @tparam EventType Type of the distributed events.
 * @tparam Filter Type of the filter.
 * @tparam Impl Type with a bool-returning Handle implementation; false reports transport failure.
 *
 * @concurrency Yes.
 *
 * @purging Events accumulate per connection and filter until they are answered, interrupted by the client, or purged.
 * Each connection is limited separately by its EventsData: when the limit is exceeded, the oldest events are purged in
 * a bunch sized by the purging coefficient. FailActiveEvents fails and releases events of all connections,
 * ClearActiveEventsForConnectionId releases all events of one connection without failing them. Events of a closed
 * connection are not released automatically, so ClearActiveEventsForConnectionId should be called on disconnection.
 * Destruction removes remaining events and invokes their termination callbacks with Cleanup.
 */
template <typename Module, typename EventType, typename Filter, typename Impl> class Distributor {
public:
	class EventsData;

	using FilterT = std::pair<uint64_t, Filter>;

	/**************************
	 * @brief Events under same filter.
	 *
	 * @concurrency Yes.
	 *
	 * @purging EraseEvent, EraseEventsByTimestamps, EraseEvents, FailEvents and Take decrease the stored events size of
	 * the owning EventsData. The emptied object stays in EventsData until FailActiveEvents or release of the whole
	 * EventsData.
	 */
	class Events {
	private:
		std::map<Timer, std::shared_ptr<EventType>> m_events;
		bool m_isReleased{};
		mutable Lock::AtomicRW m_eventsLock;
		EventsData& m_data;

	public:
		/**************************
		 * @brief Construct new events object.
		 *
		 * @locking Is not required.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE explicit Events(EventsData& data) noexcept;

		Events(const Events&) = delete;
		Events(Events&&) = delete;
		Events& operator=(const Events&) = delete;
		Events& operator=(Events&&) = delete;

		/**************************
		 * @brief Add new event with current timestamp, increase stored events size and purge events in case of limit
		 * exceeding.
		 *
		 * @attention Keeps the supplied event and limit usage unchanged when a timestamp already exists or storage is
		 * released.
		 *
		 * @tparam Universal R or L-reference of the event type.
		 *
		 * @param event Event to be added.
		 *
		 * @pre The caller retains EventsData ownership throughout this call.
		 *
		 * @locking Write locks m_eventsLock for insertion and counting, then releases it before limit checking.
		 * No external registry lock may span the call because purging invokes termination callbacks.
		 *
		 * @return True if registered, false if released or insertion fails.
		 *
		 * @test Yes.
		 */
		template <typename Universal>
			requires EventTypePtrT<Universal, EventType>
		FORCE_INLINE [[nodiscard]] bool AddEvent(Universal&& event);

		/**************************
		 * @brief Removes contained events, attempts failure delivery until the first transport error, then invokes
		 * termination callbacks for every entry removed by this call.
		 *
		 * @param error Error text satisfying SendFailed's character restrictions.
		 * @param reason Reason reported to removed events.
		 *
		 * @pre The caller retains EventsData ownership throughout this call.
		 *
		 * @locking Write locks m_eventsLock for removal and accounting, then releases it before failure delivery and
		 * termination callbacks. Callers hold no registry locks across the call.
		 *
		 * @return True if all failure frames are sent, false on transport failure.
		 *
		 * @test Yes.
		 *
		 * @todo Events can be failed in bunches.
		 */
		FORCE_INLINE [[nodiscard]] bool FailEvents(std::string_view error, TerminationReason reason);

		/**************************
		 * @brief Removes contained events, then invokes termination callbacks only for entries removed by this call.
		 *
		 * @param reason Reason reported to removed events.
		 *
		 * @pre The caller retains EventsData ownership throughout this call.
		 *
		 * @locking Write locks m_eventsLock; no external registry lock may span termination callbacks.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void EraseEvents(TerminationReason reason);

		/**************************
		 * @brief Prevent future insertion into a removed filter. Existing entries remain until removal notifies them.
		 *
		 * @locking Write locks m_eventsLock.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void Release() noexcept;

		/**************************
		 * @brief Erases an event by its timestamp and decreases stored events size. Invokes the termination callback
		 * only if this call removes the entry.
		 *
		 * @param timestamp Timestamp when event was added.
		 * @param reason Reason reported to the removed event.
		 *
		 * @pre The caller retains EventsData ownership throughout this call.
		 *
		 * @locking Write locks m_eventsLock; no external registry lock may span termination callbacks.
		 *
		 * @return True if this call removes the entry, false if it is already absent.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool EraseEvent(Timer timestamp, TerminationReason reason);

		/**************************
		 * @brief Erases events by their timestamps and decreases stored events size. Invokes termination callbacks only
		 * for entries removed by this call.
		 *
		 * @tparam Container Forward container with Timer types.
		 *
		 * @param timestamps Container with event timestamps to be erased.
		 *
		 * @pre The caller retains EventsData ownership throughout this call.
		 *
		 * @locking Write locks m_eventsLock; no external registry lock may span termination callbacks.
		 *
		 * @test Yes.
		 */
		template <template <typename> typename Container>
		FORCE_INLINE void EraseEventsByTimestamps(const Container<Timer>& timestamps);

		/**************************
		 * @return Read-write structure lock.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] Lock::AtomicRW& GetLock() const noexcept;

		/**************************
		 * @attention Is assumed to be used under structure locking.
		 *
		 * @return Readable reference to stored events.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] const std::map<Timer, std::shared_ptr<EventType>>& Get() const noexcept;

		/**************************
		 * @brief Moves ownership of events to the caller and decreases the stored events size.
		 *
		 * @pre The caller retains EventsData ownership throughout this call and finalizes every returned event.
		 *
		 * @locking Write locks m_eventsLock for removal and accounting.
		 *
		 * @return Owned events, or an empty map if none remain or the filter is released.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] std::map<Timer, std::shared_ptr<EventType>> Take() noexcept;
	};

	/**************************
	 * @brief Collection of events by their filter under same connection id, controls events limit. Default events size
	 * limit is 1024 and purging coefficient is 30%.
	 *
	 * @concurrency Yes.
	 *
	 * @purging CheckLimitAndPurge attempts to release the oldest events when the stored size exceeds the limit, in a
	 * bunch of the exceeding size plus the limit multiplied by the purging coefficient. If stored events are fewer than
	 * expected, all events are failed and released. FailActiveEvents fails and releases all events together with their
	 * per-filter containers. EraseEventsByFilter and EraseEvent release events but keep the per-filter containers.
	 *
	 * @todo Review the safety of decrementing the EventsData event counter after ownership transfer; consider the
	 * Guarantee class.
	 */
	class EventsData {
	public:
		/**************************
		 * @brief GetEvents policy: only look up existing events structure for filter.
		 *
		 * @todo Add static asserts in future test.
		 */
		static constexpr inline bool LOOKUP{ true };
		// GetEvents policy: create events structure for filter if it does not exist
		static constexpr inline bool CREATE{};

	private:
		const uint64_t m_connectionId;
		std::map<FilterT, std::shared_ptr<Events>> m_filterToEvents;
		bool m_isReleased{};
		mutable Lock::AtomicRW m_filterToEventsLock;
		std::atomic<int32_t> m_limit{ 1024 };
		std::atomic<float> m_purgingCoefficient{ 0.3f };
		std::atomic<int32_t> m_eventsSize{};
		mutable Lock::Atomic m_purgeLock;

	public:
		/**************************
		 * @brief Construct new events data object.
		 *
		 * @param connectionId Id of connection.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE explicit EventsData(uint64_t connectionId) noexcept;

		EventsData(const EventsData&) = delete;
		EventsData(EventsData&&) = delete;
		EventsData& operator=(const EventsData&) = delete;
		EventsData& operator=(EventsData&&) = delete;

		/**************************
		 * @return Connection identifier.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint64_t GetConnectionId() const noexcept;

		/**************************
		 * @brief Looks up filter storage and creates it when creation is requested and the filter is absent.
		 *
		 * @tparam IsLookupOnly True if only existed events are required, otherwise new events object will be created.
		 *
		 * @param filter Filter to find related events.
		 *
		 * @locking Read locks m_filterToEventsLock for lookup, then write locks it for creation if needed.
		 *
		 * @return Events by the specific filter, or nullptr if absent in lookup mode or if connection data is released.
		 *
		 * @test Yes.
		 */
		template <bool IsLookupOnly>
		FORCE_INLINE [[nodiscard]] std::shared_ptr<Events> GetEvents(const FilterT& filter) noexcept;

		/**************************
		 * @brief Fails and removes matching events, completing termination callbacks even if failure delivery fails.
		 *
		 * @param filter Events related filter.
		 * @param error Error text satisfying SendFailed's character restrictions.
		 *
		 * @locking Read locks m_filterToEventsLock for lookup, releases it, then write locks Events::m_eventsLock
		 * for removal. Callers hold no registry locks across the call; callbacks execute after all locks are released.
		 *
		 * @return True if all failure frames are sent or no events match, false on transport failure.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool FailEventsByFilter(const FilterT& filter, std::string_view error);

		/**************************
		 * @brief Erase events by filter.
		 *
		 * @param filter Events related filter.
		 * @param reason Reason reported to removed events.
		 *
		 * @locking Read locks m_filterToEventsLock for lookup, releases it, then write locks Events::m_eventsLock
		 * for removal. Callers hold no registry locks across the call; callbacks execute after all locks are released.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void EraseEventsByFilter(const FilterT& filter, TerminationReason reason);

		/**************************
		 * @brief Erase event by its id. Expensive operation as requires O(n) searching.
		 *
		 * @param id Event id to be erased.
		 *
		 * @locking Read locks m_filterToEventsLock then Events::m_eventsLock during lookup; releases both before
		 * write locking the target Events::m_eventsLock for removal. Callers hold no registry locks across callbacks.
		 *
		 * @return True if this call removes the requested event, false if absent or already removed concurrently.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool EraseEvent(uint64_t id);

		/**************************
		 * @brief Fails and removes stored events, completing termination callbacks even if failure delivery fails.
		 * Skips I/O for later filters on this connection after a transport error. Retires connection storage for
		 * reasons other than LimitExceeded.
		 *
		 * @param error Error text satisfying SendFailed's character restrictions.
		 * @param reason Reason reported to removed events.
		 *
		 * @locking Removes registry entries under write locks, then removes events under their write locks.
		 * Callbacks execute after all registry locks are released; callers must not hold registry locks.
		 *
		 * @return True if all failure frames are sent, false on transport failure.
		 *
		 * @test Yes.
		 *
		 * @todo Events can be failed in bunches.
		 */
		FORCE_INLINE [[nodiscard]] bool FailActiveEvents(std::string_view error, TerminationReason reason);

		/**************************
		 * @brief Decrease events size.
		 *
		 * @param value Value to subtract.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void DecreaseEventsSize(int32_t value) noexcept;

		/**************************
		 * @brief Removes all filters and their registered events, invoking any termination callbacks with the supplied
		 * reason. Sends no protocol messages.
		 *
		 * @param reason Reason for removal.
		 *
		 * @locking Write locks m_filterToEventsLock to remove filters, then each Events::m_eventsLock during removal.
		 * Callbacks run outside all registry locks.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void EraseEvents(TerminationReason reason);

		/**************************
		 * @brief Increases the atomic events size. The insertion caller checks limits after releasing its event lock.
		 *
		 * @attention The caller must ensure that the value added does not exceed the allowed limit.
		 *
		 * @param value Value to add.
		 *
		 * @locking Is not required. The insertion caller holds Events::m_eventsLock to order counting with removal.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void IncreaseEventsSize(int32_t value) noexcept;

		/**************************
		 * @return Events size limit.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] int32_t GetLimit() const noexcept;

		/**************************
		 * @brief Updates the event limit and attempts to purge excess events when reducing it.
		 *
		 * @see CheckLimitAndPurge for deferred enforcement when another check is active.
		 *
		 * @param value New limit, cannot be less than 64.
		 *
		 * @locking Updates m_limit atomically. Limit reduction follows CheckLimitAndPurge locking.
		 *
		 * @return True if changed, false if invalid or unchanged.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool SetLimit(int32_t value);

		/**************************
		 * @return Events purging coefficient.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] float GetPurgingCoefficient() const noexcept;

		/**************************
		 * @brief Set new purging events coefficient.
		 *
		 * @param value New coefficient, cannot be less than 5% or greater than 99%.
		 *
		 * @return True if coefficient is updated, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool SetPurgingCoefficient(float value);

		/**************************
		 * @brief Checks the event limit and purges excess events when no other purge check owns this connection's
		 * purge lock.
		 *
		 * @attention Contending or reentrant calls return immediately. Excess events may remain until a later
		 * registration or limit reduction triggers another check; skipped calls do not schedule a retry.
		 *
		 * @note Registration and other removal operations may still change the event count and candidate set during
		 * a purge.
		 *
		 * @locking Attempts exclusive m_purgeLock acquisition without waiting before reading the limit and count;
		 * retains it until return, including failure delivery and termination callbacks. Read locks
		 * m_filterToEventsLock then Events::m_eventsLock for selection, releasing both before write locking
		 * Events::m_eventsLock for removal. The full-purge fallback follows FailActiveEvents locking. Callers hold no
		 * registry locks across this call; registry locks are released before callbacks.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void CheckLimitAndPurge();
	};

private:
	/**************************
	 * @brief Retains transport data and any claimed events until delivery and termination notifications finish.
	 *
	 * @tparam IsSingleResponse True to finalize owned events, false for recipients that remain registered.
	 *
	 * @concurrency No.
	 */
	template <bool IsSingleResponse> class Destination {
	private:
		std::shared_ptr<Connection::Data> m_connectionData;
		std::map<Timer, std::shared_ptr<EventType>> m_ownedEvents;
		const uint64_t m_uidsOffset;
		uint64_t m_uidsSize{};
		// An unfinished owned batch fails if message construction exits before its delivery outcome is recorded.
		TerminationReason m_reason{ TerminationReason::Failed };

	public:
		/**************************
		 * @brief Creates a destination for recipients that remain registered.
		 *
		 * @param connectionData Transport data retained through delivery.
		 * @param uidsOffset Start of this destination's IDs in the shared UID string.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit Destination(
			const std::shared_ptr<Connection::Data>& connectionData, uint64_t uidsOffset) noexcept;

		/**************************
		 * @brief Takes ownership of claimed events and their termination notifications.
		 *
		 * @param events Claimed events on one connection.
		 * @param uidsOffset Start of this destination's IDs in the shared UID string.
		 *
		 * @pre The supplied map is not empty.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit Destination(
			std::map<Timer, std::shared_ptr<EventType>>&& events, uint64_t uidsOffset) noexcept;

		/**************************
		 * @brief Completes termination notifications for this destination's owned events.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE ~Destination() noexcept;

		Destination(const Destination&) = delete;
		/**************************
		 * @brief Transfers the destination and notification ownership, leaving the source without owned events.
		 *
		 * @param other Destination being moved.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE Destination(Destination&& other) noexcept;
		Destination& operator=(const Destination&) = delete;
		Destination& operator=(Destination&&) = delete;

		/**************************
		 * @return Retained destination connection.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Connection& GetConnection() const noexcept;

		/**************************
		 * @return Claimed events whose termination notifications belong to this destination.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const std::map<Timer, std::shared_ptr<EventType>>& GetOwnedEvents() const noexcept;

		/**************************
		 * @param uids Shared UID string.
		 *
		 * @return IDs addressed by this destination.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] std::string_view GetUids(const std::string& uids) const noexcept;

		/**************************
		 * @brief Records the formatted UID range.
		 *
		 * @param size Formatted UID length.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void SetUidsSize(uint64_t size) noexcept;

		/**************************
		 * @brief Records the outcome reported by this destination's termination notifications.
		 *
		 * @param reason Completed delivery outcome.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void SetTerminationReason(TerminationReason reason) noexcept;

		/**************************
		 * @brief Attempts failure delivery to owned events, stopping on the first failed write.
		 *
		 * @param error Error text satisfying SendFailed's character restrictions.
		 *
		 * @return True if all failure frames are sent, false on transport failure.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool FailOwnedEvents(std::string_view error);
	};

protected:
	// Send takes ownership of events and finalizes.
	static constexpr bool SINGLE_RESPONSE{ true };
	// Send takes lookup of events and delivers.
	static constexpr bool REPEATABLE_RESPONSE{};

private:
	Module& m_authorization;
	std::unordered_map<uint64_t, std::shared_ptr<typename EventType::BaseT::HandlerDataT>> m_hashToHandlerData;
	mutable Lock::AtomicRW m_hashToHandlerDataLock;
	std::unordered_map<uint64_t, std::shared_ptr<EventsData>> m_connectionIdToEventsData;
	mutable Lock::AtomicRW m_connectionToEventsDataLock;

public:
	/**************************
	 * @brief Constructs the new distributor object.
	 *
	 * @param authorization Authorization module to access checking.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE explicit Distributor(Module& authorization) noexcept;

	/**************************
	 * @brief Removes remaining events with Cleanup notifications. Sends no wire messages.
	 *
	 * @pre No concurrent operations remain; callback captures and authorization outlive the distributor. Callbacks
	 * must not register new subscriptions during destruction.
	 *
	 * @locking Removes connection data under write lock m_connectionToEventsDataLock, then removes events without
	 * enclosing registry locks. Notifications run after removal locks are released.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE ~Distributor() noexcept;

	Distributor(const Distributor&) = delete;
	Distributor(Distributor&&) = delete;
	Distributor& operator=(const Distributor&) = delete;
	Distributor& operator=(Distributor&&) = delete;

	/**************************
	 * @brief Collect newly received event, check if it an interruption, look for specific handler and verify access
	 * right to the data and call handler on success. Erase on interruption.
	 *
	 * @attention Initial handlers receive a reusable thread-local response string. They must not retain references,
	 * pointers, or views into that string after returning, or recursively collect another event through any distributor
	 * instance using the same Handle specialization on this thread. Such collection overwrites the active response.
	 *
	 * @param id Event id.
	 * @param hash Event type hash.
	 * @param connectionData Connection data of request.
	 * @param json Request json.
	 *
	 * @return True if processing completes without transport failure, including an application failure reported to
	 * the client; false if I/O fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Collect(
		uint64_t id, uint64_t hash, const std::shared_ptr<Connection::Data>& connectionData, Json&& json);

	/**************************
	 * @brief Registers a handler with permission requirements for an event type hash. Rejects duplicate hashes with a
	 * warning and preserves the original handler data.
	 *
	 * @tparam Handler Type of the event handler function.
	 *
	 * @param hash Event type hash.
	 * @param handler Handler function; error text produced on HandleResult::Fail follows SendFailed's character
	 * restrictions.
	 * @param terminationHandler Termination handler function.
	 * @param grade Required minimum grade.
	 *
	 * @locking Write locks m_hashToHandlerDataLock.
	 *
	 * @test Yes.
	 */
	template <typename Handler>
		requires HandlerT<Handler, EventType>
	FORCE_INLINE void SetHandlerWithPermissions(uint64_t hash, Handler&& handler,
		typename EventType::BaseT::TerminationHandlerT terminationHandler, Module::grade_t grade);

	/**************************
	 * @brief Registers a handler without permission requirements for an event type hash. Rejects duplicate hashes with
	 * a warning and preserves the original handler data.
	 *
	 * @tparam Handler Type of the event handler function.
	 *
	 * @param hash Event type hash.
	 * @param handler Handler function; error text produced on HandleResult::Fail follows SendFailed's character
	 * restrictions.
	 * @param terminationHandler Termination handler function.
	 *
	 * @locking Write locks m_hashToHandlerDataLock.
	 *
	 * @test Yes.
	 */
	template <typename Handler>
		requires HandlerT<Handler, EventType>
	FORCE_INLINE void SetHandlerWithoutPermissions(
		uint64_t hash, Handler&& handler, typename EventType::BaseT::TerminationHandlerT terminationHandler);

	/**************************
	 * @brief Fails and removes all stored events, completing termination callbacks with Cleanup even if failure
	 * delivery fails. Continues failure delivery to other connections after a write error.
	 *
	 * @param error Error text satisfying SendFailed's character restrictions.
	 *
	 * @locking Removes registry entries under write locks through the overload accepting a termination reason.
	 * Callbacks execute after all registry locks are released; callers must not hold registry locks.
	 *
	 * @return True if all failure frames are sent, false if any connection write fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool FailActiveEvents(std::string_view error);

	/**************************
	 * @brief Clear all stored events for specific connection id.
	 *
	 * @attention Reports ConnectionClosed for every removed event.
	 *
	 * @param connectionId Connection id of request.
	 *
	 * @locking Write locks m_connectionToEventsDataLock to remove the connection. Releases it before per-connection
	 * removal and all termination callbacks.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void ClearActiveEventsForConnectionId(uint64_t connectionId);

	/**************************
	 * @brief Sets the event limit for a connection, creating its storage if needed, and attempts to purge excess events
	 * when reducing the limit.
	 *
	 * @see EventsData::CheckLimitAndPurge for deferred enforcement when another check is active.
	 *
	 * @param connectionId Connection identifier.
	 * @param value Event limit, at least 64.
	 *
	 * @locking Read/write locks m_connectionToEventsDataLock during lookup and creation. Limit reduction follows
	 * EventsData::CheckLimitAndPurge locking. Callers hold no registry locks across callbacks.
	 *
	 * @return True if changed, false if invalid or unchanged.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SetEventsLimit(uint64_t connectionId, int32_t value);

	/**************************
	 * @brief Sets the purging coefficient for a connection, creating its storage if needed.
	 *
	 * @param connectionId Connection identifier.
	 * @param value Purging coefficient, from 0.05 to 0.99.
	 *
	 * @locking Read/write locks m_connectionToEventsDataLock during lookup and creation; coefficient updates are
	 * atomic.
	 *
	 * @return True if changed, false if invalid or unchanged.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SetEventsPurgingCoefficient(uint64_t connectionId, float value);

	/**************************
	 * @brief Fails and removes matching events on each connection, completing termination callbacks even if failure
	 * delivery fails. Continues failure delivery to other connections after a write error.
	 *
	 * @attention Retains connection data through recipient snapshot and removal.
	 *
	 * @param filter Filter to find related events.
	 * @param error Error text satisfying SendFailed's character restrictions.
	 *
	 * @locking Read locks m_connectionToEventsDataLock for the connection snapshot, then releases it before filter
	 * lookup and event removal locks. No registry lock spans a termination callback.
	 *
	 * @return True if all failure frames are sent or no events match, false if any connection write fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool FailEventsOnConnectionsByFilter(const FilterT& filter, std::string_view error);

	/**************************
	 * @brief Removes matching events on every connection and reports RemovedByFilter.
	 *
	 * @attention Retains connection data through recipient snapshot and removal.
	 *
	 * @param filter Filter to find related events.
	 *
	 * @locking Read locks m_connectionToEventsDataLock for the connection snapshot, then releases it before filter
	 * lookup and event removal locks. No registry lock spans a termination callback.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void EraseEventsOnConnectionsByFilter(const FilterT& filter);

protected:
	/**************************
	 * @brief Sends data to matching events on each connection. Single-response delivery takes ownership of
	 * events and finalizes only that batch, reporting Completed for successful writes and Failed for producer failures,
	 * including exceptions.
	 *
	 * @attention A lazy producer receives a reusable thread-local string. It must not retain references, pointers, or
	 * views into that string after returning. During message construction or frame delivery, callbacks must not
	 * recursively invoke this SendData specialization, directly or through CheckDelayed, on this thread, including
	 * through another distributor instance. Recursive calls overwrite the active message and recipient snapshot.
	 *
	 * @note Repeatable delivery uses a snapshot of eligible recipients. Distribution is not atomic with subscription
	 * changes: an event removed after selection may still receive the current message, and one added afterward may
	 * miss it. A termination notification does not cancel messages already selected or initiated by this call.
	 * Taking a single-response batch makes later interruption and purging ineligible to remove its events; the sender
	 * owns their notifications. Newly registered events remain for a later call.
	 *
	 * @tparam IsSingleResponse True to claim events for delivery, false to snapshot recipients.
	 * @tparam T Type of data, string view or callable to fill provided string.
	 *
	 * @param filter Related hash filter to events.
	 * @param getData Data to send, or a callable that writes a valid JSON value on true or error text satisfying
	 * SendFailed's character restrictions on false.
	 *
	 * @locking Read locks m_connectionToEventsDataLock then EventsData::m_filterToEventsLock during filter lookup,
	 * followed by Events::m_eventsLock for recipient selection. Single-response delivery uses Take write locks;
	 * repeatable delivery uses read locks. Releases these locks before invoking getData or sending data frames.
	 * Repeatable producer failure follows FailEventsOnConnectionsByFilter locking; transport failure follows
	 * ClearActiveEventsForConnectionId locking. Callers hold no registry locks across this call.
	 *
	 * @return Success if all data frames are sent, IOError if any write fails, Fail if response production fails and
	 * all failure frames are sent, or Nothing if no eligible events remain.
	 *
	 * @test Yes.
	 */
	template <bool IsSingleResponse, DataT T>
	FORCE_INLINE [[nodiscard]] SendResult SendData(const FilterT& filter, T getData);

	/**************************
	 * @brief Registers an initialized event under its own connection and the supplied filter.
	 *
	 * @param filter Event filter.
	 * @param event Initialized event to register, moved into storage.
	 *
	 * @pre The event and its connection data are not null. The initial handler accepts registration.
	 *
	 * @locking Uses GetEventsData lookup/creation locks, EventsData::GetEvents filter locks, then Events::AddEvent
	 * insertion and limit-checking locks. Retains connection storage through registration and purging callbacks.
	 *
	 * @return True if registered, false if storage was released or insertion fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool RegisterEvent(const FilterT& filter, std::shared_ptr<EventType> event);

	/**************************
	 * @brief Removes matching events using the lifecycle reason supplied by a derived distributor.
	 *
	 * @attention Retains connection data through recipient snapshot and removal.
	 *
	 * @param filter Filter to find related events.
	 * @param reason Reason reported to removed events.
	 *
	 * @locking Read locks m_connectionToEventsDataLock for the connection snapshot, then releases it before filter
	 * lookup and event removal locks. No registry lock spans a termination callback.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void EraseEventsOnConnectionsByFilter(const FilterT& filter, TerminationReason reason);

private:
	/**************************
	 * @brief Finds connection storage and creates it when absent.
	 *
	 * @param connectionId Related connection id.
	 *
	 * @locking Read locks m_connectionToEventsDataLock for lookup, then write locks it for insertion if not found.
	 *
	 * @return Events data by connection id.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] std::shared_ptr<EventsData> GetEventsData(uint64_t connectionId) noexcept;

	/**************************
	 * @brief Snapshots existing filter storage across registered connections.
	 *
	 * @param filter Filter to find related events.
	 *
	 * @locking Read locks m_connectionToEventsDataLock, then EventsData::m_filterToEventsLock during filter lookup.
	 *
	 * @return Filter storage found on each connection.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] std::vector<std::shared_ptr<Events>> GetEventsArray(
		const FilterT& filter) const noexcept;

	/**************************
	 * @brief Fails and removes all stored events, completing termination callbacks even if failure delivery fails.
	 * Continues failure delivery to other connections after a write error.
	 *
	 * @param error Error text satisfying SendFailed's character restrictions.
	 * @param reason Reason reported to removed events.
	 *
	 * @locking Write locks m_connectionToEventsDataLock to remove connections, then releases it before per-connection
	 * removal, failure delivery, and termination callbacks. Per-connection removal follows EventsData::FailActiveEvents
	 * locking. Callers hold no registry locks across this call.
	 *
	 * @return True if all failure frames are sent, false if any connection write fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool FailActiveEvents(std::string_view error, TerminationReason reason);

protected:
	/**************************
	 * @brief Invokes the event-specific initial handler through Event's private dispatch interface.
	 *
	 * @param event Event being initialized.
	 * @param payload Preformed response payload passed to the handler.
	 *
	 * @locking Is not required.
	 *
	 * @return Result reported by the event handler.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static HandleResult HandleEvent(EventType& event, std::string& payload);
};

/**************************
 * @brief Distributor of single events.
 *
 * @tparam Module Type of the authorization module.
 *
 * @purging Only delayed single events awaiting delivery are stored. CheckDelayed takes ownership of
 * stored events; interruption and base distributor purging act on the remaining events.
 */
template <typename Module>
class SinglesDistributor : public Distributor<Module, Single, IdentityFilter, SinglesDistributor<Module>> {
public:
	using BaseT = Distributor<Module, Single, IdentityFilter, SinglesDistributor<Module>>;

public:
	/**************************
	 * @brief Construct new singles distributor object.
	 *
	 * @param authorization Authorization module to access checking.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE explicit SinglesDistributor(Module& authorization) noexcept;

	SinglesDistributor(const SinglesDistributor&) = delete;
	SinglesDistributor(SinglesDistributor&&) = delete;
	SinglesDistributor& operator=(const SinglesDistributor&) = delete;
	SinglesDistributor& operator=(SinglesDistributor&&) = delete;

	/**************************
	 * @brief Takes ownership of delayed events and delivers their responses through SendData.
	 *
	 * @note Concurrent checks cannot claim the same event. Registration during delivery creates events for a
	 * later call. Completed identifies a successful local frame write, without implying peer acknowledgment.
	 *
	 * @see SendData for the producer's buffer lifetime and recursive-call restrictions.
	 *
	 * @tparam T Type of data, string view or callable to fill provided string.
	 *
	 * @param filter Related hash filter to events.
	 * @param getData Data to send, or a callable that writes a valid JSON value on true or error text satisfying
	 * SendFailed's character restrictions on false.
	 *
	 * @locking Follows SendData locking. Callers hold no registry locks across this call.
	 *
	 * @return The SendData result.
	 *
	 * @test Yes.
	 */
	template <DataT T> FORCE_INLINE [[nodiscard]] SendResult CheckDelayed(const BaseT::FilterT& filter, T getData);

private:
	/**************************
	 * @brief Handle newly collected event.
	 *
	 * @param id Event id.
	 * @param hash Event type hash.
	 * @param filter Event filter.
	 * @param connectionData Related connection data.
	 * @param json Request json.
	 * @param handlerData Related handler and access data.
	 *
	 * @return True unless transport I/O fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Handle(uint64_t id, uint64_t hash, IdentityFilter&& filter,
		const std::shared_ptr<Connection::Data>& connectionData, Json&& json,
		std::shared_ptr<typename Single::BaseT::HandlerDataT>&& handlerData);

	// Access to Handle
	friend BaseT;
};

/**************************
 * @brief Distributor of stream events.
 *
 * @tparam Module Type of the authorization module.
 *
 * @purging Each accepted stream event is stored and is not released by sending data. It is released when the client
 * interrupts it or by the base distributor purging.
 */
template <typename Module>
class StreamsDistributor : public Distributor<Module, Stream, IdentityFilter, StreamsDistributor<Module>> {
public:
	using BaseT = Distributor<Module, Stream, IdentityFilter, StreamsDistributor<Module>>;

public:
	/**************************
	 * @brief Construct new streams distributor object.
	 *
	 * @param authorization Authorization module to access checking.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE explicit StreamsDistributor(Module& authorization) noexcept;

	StreamsDistributor(const StreamsDistributor&) = delete;
	StreamsDistributor(StreamsDistributor&&) = delete;
	StreamsDistributor& operator=(const StreamsDistributor&) = delete;
	StreamsDistributor& operator=(StreamsDistributor&&) = delete;

	/**************************
	 * @brief Broadcasts data to matching stream subscriptions.
	 *
	 * @see BaseT::SendData for producer requirements and delivery behavior.
	 *
	 * @tparam T Type of data or lazy producer.
	 *
	 * @param filter Event type hash and filter.
	 * @param getData Data to send or lazy producer.
	 *
	 * @locking Follows BaseT::SendData locking.
	 *
	 * @return The BaseT::SendData result.
	 *
	 * @test Yes.
	 */
	template <DataT T> FORCE_INLINE [[nodiscard]] SendResult SendData(const BaseT::FilterT& filter, T getData);

private:
	/**************************
	 * @brief Handle newly collected event.
	 *
	 * @param id Event id.
	 * @param hash Event type hash.
	 * @param filter Event filter.
	 * @param connectionData Related connection data.
	 * @param json Request json.
	 * @param handlerData Related handler and access data.
	 *
	 * @return True unless transport I/O fails.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Handle(uint64_t id, uint64_t hash, IdentityFilter&& filter,
		const std::shared_ptr<Connection::Data>& connectionData, Json&& json,
		std::shared_ptr<typename Stream::BaseT::HandlerDataT>&& handlerData);

	// Access to Handle
	friend BaseT;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const Type type) noexcept
{
	static_assert(U(Type::Max) == 3, "Type::Max has changed, update EnumToString");
	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.

	switch (type) {
	case Type::Undefined:
		return "Undefined";
	case Type::Single:
		return "Single";
	case Type::Stream:
		return "Stream";
	case Type::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown event type: {}", U(type));
		return "Unknown";
	}
}

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const HandleResult result) noexcept
{
	static_assert(U(HandleResult::Max) == 6, "HandleResult::Max has changed, update EnumToString");
	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.

	switch (result) {
	case HandleResult::Undefined:
		return "Undefined";
	case HandleResult::Success:
		return "Success";
	case HandleResult::Fail:
		return "Fail";
	case HandleResult::Delay:
		return "Delay";
	case HandleResult::SuccessChunked:
		return "SuccessChunked";
	case HandleResult::IOError:
		return "IOError";
	case HandleResult::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown handle result: {}", U(result));
		return "Unknown";
	}
}

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const TerminationReason reason) noexcept
{
	static_assert(U(TerminationReason::Max) == 8, "TerminationReason::Max has changed, update EnumToString");

	switch (reason) {
	case TerminationReason::Undefined:
		return "Undefined";
	case TerminationReason::ClientInterrupted:
		return "ClientInterrupted";
	case TerminationReason::ConnectionClosed:
		return "ConnectionClosed";
	case TerminationReason::Cleanup:
		return "Cleanup";
	case TerminationReason::LimitExceeded:
		return "LimitExceeded";
	case TerminationReason::RemovedByFilter:
		return "RemovedByFilter";
	case TerminationReason::Failed:
		return "Failed";
	case TerminationReason::Completed:
		return "Completed";
	case TerminationReason::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown termination reason: {}", U(reason));
		return "Unknown";
	}
}

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const SendResult result) noexcept
{
	static_assert(U(SendResult::Max) == 5, "SendResult::Max has changed, update EnumToString");
	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.

	switch (result) {
	case SendResult::Undefined:
		return "Undefined";
	case SendResult::Success:
		return "Success";
	case SendResult::Fail:
		return "Fail";
	case SendResult::Nothing:
		return "Nothing";
	case SendResult::IOError:
		return "IOError";
	case SendResult::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown send result: {}", U(result));
		return "Unknown";
	}
}

FORCE_INLINE [[nodiscard]] bool SendFailed(const uint64_t id, Connection& connection, const std::string_view error)
{
	static_assert(static_cast<int32_t>(Stream::State::Failed) == 4, "Stream failed state is expected");
	LOG_PROTOCOL_NEW(
		"Send stream event failed state, stream id: {}, error: {}, connection id: {}", id, error, connection.GetId());
	thread_local std::string t_payload{};
	t_payload.clear();
	std::format_to(std::back_inserter(t_payload), "{{\"uids\":[{}],\"state\":4,\"error\":\"{}\"}}", id, error);
	Data data{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(t_payload.data()), t_payload.size()),
		Data::Opcode::Text, /*mask=*/0, /*isFinal=*/true, /*rsv1=*/false, /*rsv2=*/false, /*rsv3=*/false };
	return Send(connection, data);
}

/*---------------------------------------------------------------------------------
Event::HandlerData
---------------------------------------------------------------------------------*/

template <typename EventType>
FORCE_INLINE Event<EventType>::HandlerData::HandlerData(
	HandlerT&& handler, TerminationHandlerT&& terminationHandler, const int16_t grade) noexcept
	: m_handler{ std::move(handler) }
	, m_terminationHandler{ std::move(terminationHandler) }
	, m_grade{ grade }
	, m_isPermissionRequired{ true }
{
}

template <typename EventType>
FORCE_INLINE Event<EventType>::HandlerData::HandlerData(
	HandlerT&& handler, TerminationHandlerT&& terminationHandler) noexcept
	: m_handler{ std::move(handler) }
	, m_terminationHandler{ std::move(terminationHandler) }
	, m_grade{}
	, m_isPermissionRequired{}
{
}

template <typename EventType>
FORCE_INLINE [[nodiscard]] const typename Event<EventType>::HandlerT&
Event<EventType>::HandlerData::GetHandler() const noexcept
{
	return m_handler;
}

template <typename EventType>
FORCE_INLINE [[nodiscard]] const typename Event<EventType>::TerminationHandlerT&
Event<EventType>::HandlerData::GetTerminationHandler() const noexcept
{
	return m_terminationHandler;
}

template <typename EventType>
FORCE_INLINE [[nodiscard]] int16_t Event<EventType>::HandlerData::GetGrade() const noexcept
{
	return m_grade;
}

template <typename EventType>
FORCE_INLINE [[nodiscard]] bool Event<EventType>::HandlerData::IsPermissionRequired() const noexcept
{
	return m_isPermissionRequired;
}

/*---------------------------------------------------------------------------------
Event
---------------------------------------------------------------------------------*/

template <typename EventType>
FORCE_INLINE Event<EventType>::Event(const uint64_t id, std::shared_ptr<HandlerData>&& handlerData, Json&& json,
	const std::shared_ptr<Connection::Data>& connectionData) noexcept
	: m_id{ id }
	, m_handlerData{ std::move(handlerData) }
	, m_json{ std::move(json) }
	, m_connectionData{ connectionData }
{
}

template <typename EventType> FORCE_INLINE [[nodiscard]] uint64_t Event<EventType>::GetId() const noexcept
{
	return m_id;
}

template <typename EventType> FORCE_INLINE [[nodiscard]] const Json& Event<EventType>::GetJson() const noexcept
{
	return m_json;
}

template <typename EventType>
FORCE_INLINE [[nodiscard]] const std::shared_ptr<Connection::Data>& Event<EventType>::GetConnectionData() const noexcept
{
	return m_connectionData;
}

template <typename EventType> FORCE_INLINE [[nodiscard]] HandleResult Event<EventType>::Handle(std::string& payload)
{
	return m_handlerData->GetHandler()(payload, *static_cast<const EventType*>(this));
}

template <typename EventType>
FORCE_INLINE void Event<EventType>::HandleTermination(const TerminationReason reason) const noexcept
{
	const auto& handler{ m_handlerData->GetTerminationHandler() };
	if (handler == nullptr) {
		return;
	}

	try {
		handler(*static_cast<const EventType*>(this), reason);
	}
	catch (...) {
		LOG_ERROR_NEW("Event termination callback failed, event id: {}, connection id: {}", m_id,
			m_connectionData->GetConnectionId());
	}
}

/*---------------------------------------------------------------------------------
IdentityFilter
---------------------------------------------------------------------------------*/

FORCE_INLINE IdentityFilter::IdentityFilter() noexcept
	: identity{}
{
}

FORCE_INLINE IdentityFilter::IdentityFilter(const uint64_t identity) noexcept
	: identity{ identity }
{
}

FORCE_INLINE IdentityFilter::IdentityFilter(const Json& json) noexcept
{
	if (const auto* filterValue{ json.GetValueType<uint64_t>("filter") }; filterValue != nullptr) {
		identity = *filterValue;
		return;
	}

	identity = 0;
}

FORCE_INLINE [[nodiscard]] bool IdentityFilter::operator==(const IdentityFilter other) const noexcept
{
	return identity == other.identity;
}

FORCE_INLINE [[nodiscard]] bool IdentityFilter::operator<(const IdentityFilter other) const noexcept
{
	return identity < other.identity;
}

FORCE_INLINE [[nodiscard]] bool IdentityFilter::operator>(const IdentityFilter other) const noexcept
{
	return identity > other.identity;
}

/*---------------------------------------------------------------------------------
Single
---------------------------------------------------------------------------------*/

FORCE_INLINE Single::Single(const uint64_t id, std::shared_ptr<HandlerData>&& handlerData, Json&& json,
	const std::shared_ptr<Connection::Data>& connectionData) noexcept
	: BaseT{ id, std::move(handlerData), std::move(json), connectionData }
{
}

/*---------------------------------------------------------------------------------
Stream
---------------------------------------------------------------------------------*/

FORCE_INLINE Stream::Stream(const uint64_t id, std::shared_ptr<HandlerData>&& handlerData, Json&& json,
	const std::shared_ptr<Connection::Data>& connectionData) noexcept
	: BaseT{ id, std::move(handlerData), std::move(json), connectionData }
{
}

FORCE_INLINE [[nodiscard]] bool Stream::SendData(const std::string_view data) const
{
	thread_local std::string t_payload{};
	t_payload.clear();
	std::format_to(std::back_inserter(t_payload), "{{\"uids\":[{}],\"data\":{}}}", BaseT::GetId(), data);
	return Send(BaseT::GetConnectionData()->GetConnection(),
		{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(t_payload.data()), t_payload.size()),
			Data::Opcode::Text, /*mask=*/0, /*isFinal=*/true, /*rsv1=*/false, /*rsv2=*/false, /*rsv3=*/false });
}

FORCE_INLINE [[nodiscard]] bool Stream::SendState(const State state) const
{
	auto& connection{ BaseT::GetConnectionData()->GetConnection() };
	LOG_PROTOCOL_NEW("Send stream event {} state, stream id: {}, connection id: {}", EnumToString(state),
		BaseT::GetId(), connection.GetId());
	thread_local std::string t_payload{};
	t_payload.clear();
	std::format_to(std::back_inserter(t_payload), "{{\"uids\":[{}],\"state\":{}}}", BaseT::GetId(), U(state));
	Data data{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(t_payload.data()), t_payload.size()),
		Data::Opcode::Text, /*mask=*/0, /*isFinal=*/true, /*rsv1=*/false, /*rsv2=*/false, /*rsv3=*/false };
	return Send(connection, data);
}

FORCE_INLINE [[nodiscard]] constexpr std::string_view Stream::EnumToString(const State state) noexcept
{
	static_assert(U(State::Max) == 7, "State::Max has changed, update EnumToString");
	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.

	switch (state) {
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
		LOG_ERROR_NEW("Unknown stream state: {}", U(state));
		return "Unknown";
	}
}

/*---------------------------------------------------------------------------------
Distributor::Events
---------------------------------------------------------------------------------*/

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::Events::Events(EventsData& data) noexcept
	: m_data{ data }
{
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <typename Universal>
	requires EventTypePtrT<Universal, EventType>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::Events::AddEvent(Universal&& event)
{
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
		if (m_isReleased) {
			return false;
		}

		const Timer timestamp{};
		// Try to emplace guaranties that event remains unchanged if insertion fails.
		const auto result{ m_events.try_emplace(timestamp, std::forward<Universal>(event)) };
		if (!result.second) [[unlikely]] {
			LOG_WARNING_NEW("Event is not emplaced, event id: {} at timestamp: {}, connection id: {}", event->GetId(),
				timestamp.ToString(), m_data.GetConnectionId());
			return false;
		}

		// Count insertion under the removal lock; purging and callbacks follow after unlocking.
		m_data.IncreaseEventsSize(1);
	}

	// Check limits after unlocking; read/write locks inside.
	m_data.CheckLimitAndPurge();
	return true;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] std::map<Timer, std::shared_ptr<EventType>>
Distributor<Module, EventType, Filter, Impl>::Events::Take() noexcept
{
	std::map<Timer, std::shared_ptr<EventType>> owned;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
		if (!m_isReleased) {
			owned.swap(m_events);
			m_data.DecreaseEventsSize(static_cast<int32_t>(owned.size()));
		}
	}

	return owned;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::Events::FailEvents(
	const std::string_view error, const TerminationReason reason)
{
	std::map<Timer, std::shared_ptr<EventType>> removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
		removed.swap(m_events);
		m_data.DecreaseEventsSize(static_cast<int32_t>(removed.size()));
	}

	bool isSuccessful{ true };
	for (const auto& [timestamp, event] : removed) {
		if (!SendFailed(event->GetId(), event->GetConnectionData()->GetConnection(), error)) [[unlikely]] {
			isSuccessful = false;
			break;
		}
	}

	for (const auto& [timestamp, event] : removed) {
		// Distributor friendship permits notification after this entry's removal.
		event->HandleTermination(reason);
	}

	return isSuccessful;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::Events::EraseEvents(const TerminationReason reason)
{
	std::map<Timer, std::shared_ptr<EventType>> removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
		removed.swap(m_events);
		m_data.DecreaseEventsSize(static_cast<int32_t>(removed.size()));
	}

	for (const auto& [timestamp, event] : removed) {
		// Distributor friendship permits notification after this entry's removal.
		event->HandleTermination(reason);
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::Events::Release() noexcept
{
	const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
	m_isReleased = true;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::Events::EraseEvent(
	const Timer timestamp, const TerminationReason reason)
{
	std::shared_ptr<EventType> removed;

	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
		const auto it{ m_events.find(timestamp) };
		if (it == m_events.end()) [[unlikely]] {
			return false;
		}

		removed = std::move(it->second);
		m_events.erase(it);
		m_data.DecreaseEventsSize(1);
	}

	// Distributor friendship permits notification after this entry's removal.
	removed->HandleTermination(reason);
	return true;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <template <typename> typename Container>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::Events::EraseEventsByTimestamps(
	const Container<Timer>& timestamps)
{
	std::vector<std::shared_ptr<EventType>> removed;

	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_eventsLock };
		for (const auto timestamp : timestamps) {
			const auto it{ m_events.find(timestamp) };
			if (it != m_events.end()) {
				removed.emplace_back(std::move(it->second));
				m_events.erase(it);
			}
		}
		m_data.DecreaseEventsSize(static_cast<int32_t>(removed.size()));
	}

	for (const auto& event : removed) {
		// Distributor friendship permits notification after this entry's removal.
		event->HandleTermination(TerminationReason::LimitExceeded);
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] Lock::AtomicRW&
Distributor<Module, EventType, Filter, Impl>::Events::GetLock() const noexcept
{
	return m_eventsLock;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] const std::map<Timer, std::shared_ptr<EventType>>&
Distributor<Module, EventType, Filter, Impl>::Events::Get() const noexcept
{
	return m_events;
}

/*---------------------------------------------------------------------------------
Distributor::EventsData
---------------------------------------------------------------------------------*/

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::EventsData::EventsData(const uint64_t connectionId) noexcept
	: m_connectionId{ connectionId }
{
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] uint64_t
Distributor<Module, EventType, Filter, Impl>::EventsData::GetConnectionId() const noexcept
{
	return m_connectionId;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsLookupOnly>
FORCE_INLINE [[nodiscard]] std::shared_ptr<typename Distributor<Module, EventType, Filter, Impl>::Events>
Distributor<Module, EventType, Filter, Impl>::EventsData::GetEvents(const FilterT& filter) noexcept
{
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_filterToEventsLock };

		if (m_isReleased) {
			return {};
		}

		auto it{ m_filterToEvents.find(filter) };
		if (it != m_filterToEvents.end()) {
			return it->second;
		}
	}

	if constexpr (!IsLookupOnly) {
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_filterToEventsLock };

		if (m_isReleased) {
			return {};
		}

		return m_filterToEvents.emplace(filter, std::make_shared<Events>(*this)).first->second;
	}
	else {
		return {};
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::EventsData::FailEventsByFilter(
	const FilterT& filter, const std::string_view error)
{
	std::shared_ptr<Events> events;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_filterToEventsLock };
		auto it{ m_filterToEvents.find(filter) };
		if (it == m_filterToEvents.end()) {
			return true;
		}
		events = it->second;
	}

	return events->FailEvents(error, TerminationReason::Failed);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EventsData::EraseEventsByFilter(
	const FilterT& filter, const TerminationReason reason)
{
	std::shared_ptr<Events> events;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_filterToEventsLock };
		auto it{ m_filterToEvents.find(filter) };
		if (it == m_filterToEvents.end()) {
			return;
		}
		events = it->second;
	}

	events->EraseEvents(reason);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::EventsData::EraseEvent(const uint64_t id)
{
	std::shared_ptr<Events> target;
	Timer targetTimestamp{ 0, 0 };

	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_filterToEventsLock };
		for (auto& [filter, events] : m_filterToEvents) {
			const Lock::AtomicRW::Guard<Lock::READ> _{ events->GetLock() };
			for (const auto& [timestamp, event] : events->Get()) {
				if (event->GetId() == id) {
					target = events;
					targetTimestamp = timestamp;
					break;
				}
			}

			if (target != nullptr) {
				break;
			}
		}
	}

	// Remove the entry if still present; write lock inside.
	return target != nullptr && target->EraseEvent(targetTimestamp, TerminationReason::ClientInterrupted);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::EventsData::FailActiveEvents(
	const std::string_view error, const TerminationReason reason)
{
	std::map<FilterT, std::shared_ptr<Events>> removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_filterToEventsLock };
		// A full limit purge keeps this connection's storage registered so new events can create fresh filters.
		// Leave an already-set release flag intact so a concurrent cleanup cannot be undone.
		if (reason != TerminationReason::LimitExceeded) {
			m_isReleased = true;
		}

		for (const auto& [filter, events] : m_filterToEvents) {
			// Prevent insertion through selected filter handles; write lock inside.
			events->Release();
		}

		removed.swap(m_filterToEvents);
	}

	bool isSuccessful{ true };
	for (const auto& [filter, events] : removed) {
		if (isSuccessful) [[likely]] {
			// Write lock inside.
			isSuccessful = events->FailEvents(error, reason);
		}
		else {
			// Finish cleanup without more writes to the failed connection; write lock inside.
			events->EraseEvents(reason);
		}
	}

	return isSuccessful;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EventsData::EraseEvents(const TerminationReason reason)
{
	std::map<FilterT, std::shared_ptr<Events>> removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_filterToEventsLock };
		m_isReleased = true;
		for (const auto& [filter, events] : m_filterToEvents) {
			// Prevent insertion through selected filter handles; write lock inside.
			events->Release();
		}

		removed.swap(m_filterToEvents);
	}

	for (const auto& [filter, events] : removed) {
		// Write lock inside.
		events->EraseEvents(reason);
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EventsData::DecreaseEventsSize(
	const int32_t value) noexcept
{
	m_eventsSize.fetch_sub(value);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EventsData::IncreaseEventsSize(
	const int32_t value) noexcept
{
	m_eventsSize.fetch_add(value);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] int32_t Distributor<Module, EventType, Filter, Impl>::EventsData::GetLimit() const noexcept
{
	return m_limit.load();
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::EventsData::SetLimit(const int32_t value)
{
	if (value < 64) [[unlikely]] {
		LOG_WARNING_NEW(
			"Events purging limit cannot be less than 64, provided: {}, connection id: {}", value, m_connectionId);
		return false;
	}

	const auto old{ m_limit.load() };
	const auto ratio{ old - value };
	if (ratio == 0) [[unlikely]] {
		return false;
	}

	LOG_PROTOCOL_NEW("Events purging limit is changed from: {} to: {}, connection id: {}", old, value, m_connectionId);
	m_limit.store(value);
	if (ratio > 0) {
		CheckLimitAndPurge();
	}
	return true;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] float
Distributor<Module, EventType, Filter, Impl>::EventsData::GetPurgingCoefficient() const noexcept
{
	return m_purgingCoefficient.load();
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::EventsData::SetPurgingCoefficient(
	const float value)
{
	if (Helper::FloatLess(value, 0.05f)) [[unlikely]] {
		LOG_WARNING_NEW("Events purging coefficient cannot be less than 0.05, provided: {:.9f}, connection id: {}",
			value, m_connectionId);
		return false;
	}

	if (Helper::FloatGreater(value, 0.99f)) [[unlikely]] {
		LOG_WARNING_NEW("Events purging coefficient cannot be greater than 0.99, provided: {:.9f}, connection id: {}",
			value, m_connectionId);
		return false;
	}

	const auto current{ m_purgingCoefficient.load() };
	if (Helper::FloatEqual(current, value)) [[unlikely]] {
		return false;
	}

	LOG_PROTOCOL_NEW("Events purging coefficient is changed from: {:.9f} to: {:.9f}, connection id: {}", current, value,
		m_connectionId);
	m_purgingCoefficient.store(value);
	return true;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EventsData::CheckLimitAndPurge()
{
	const Lock::Atomic::TryGuard purgeLockGuard{ m_purgeLock };
	if (!purgeLockGuard.OwnsLock()) {
		return;
	}

	const auto limit{ m_limit.load() };
	auto toBePurged{ m_eventsSize.load() - limit };
	if (toBePurged <= 0) {
		return;
	}

	// Due to the difficulty of data structures and parallelism efficiency access algorithms it make sense to
	// purge bunch of events at once
	const auto purgingCoefficient{ m_purgingCoefficient.load() };
	toBePurged += static_cast<int32_t>(static_cast<float>(limit) * purgingCoefficient);

	std::map<Timer, std::shared_ptr<Events>> sortedEvents;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_filterToEventsLock };
		for (auto& [filter, events] : m_filterToEvents) {
			const Lock::AtomicRW::Guard<Lock::READ> _{ events->GetLock() };
			for (auto& [timestamp, event] : events->Get()) {
				sortedEvents.emplace(timestamp, events);
			}
		}
	}

	if (const auto size{ sortedEvents.size() }; size < static_cast<size_t>(toBePurged)) [[unlikely]] {
		LOG_WARNING_NEW("Unexpectedly size of sorted events: {} is less that should be purged: {}, purging "
						"coefficient: {:.9f}, connection id: {}. Purging all.",
			size, toBePurged, purgingCoefficient, m_connectionId);
		(void)FailActiveEvents("All events are purged due to unexpected processing", TerminationReason::LimitExceeded);
		return;
	}

	LOG_PROTOCOL_NEW("Events limit: {} is exceeded and {} events are going to be purged, connection id: {}", limit,
		toBePurged, m_connectionId);
	std::map<std::shared_ptr<Events>, std::vector<Timer>> sortedTimestamps;
	for (auto begin{ sortedEvents.begin() }, end{ sortedEvents.end() }; toBePurged > 0;) {
		sortedTimestamps[begin->second].emplace_back(begin->first);
		++begin;
		--toBePurged;
	}

	for (auto& [events, timestamps] : sortedTimestamps) {
		// Remove selected purge candidates outside selection locks; write lock inside.
		events->EraseEventsByTimestamps(timestamps);
	}
}

/*---------------------------------------------------------------------------------
Distributor::Destination
---------------------------------------------------------------------------------*/

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::Destination(
	const std::shared_ptr<Connection::Data>& connectionData, const uint64_t uidsOffset) noexcept
	: m_connectionData{ connectionData }
	, m_uidsOffset{ uidsOffset }
{
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::Destination(
	std::map<Timer, std::shared_ptr<EventType>>&& events, const uint64_t uidsOffset) noexcept
	: Destination{ events.begin()->second->GetConnectionData(), uidsOffset }
{
	m_ownedEvents.swap(events);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::~Destination() noexcept
{
	if constexpr (IsSingleResponse) {
		for (const auto& [timestamp, event] : m_ownedEvents) {
			// Friend access.
			event->HandleTermination(m_reason);
		}
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::Destination(
	Destination&& other) noexcept
	: m_connectionData{ std::move(other.m_connectionData) }
	, m_uidsOffset{ other.m_uidsOffset }
	, m_uidsSize{ other.m_uidsSize }
	, m_reason{ other.m_reason }
{
	if constexpr (IsSingleResponse) {
		m_ownedEvents.swap(other.m_ownedEvents);
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE [[nodiscard]] Connection&
Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::GetConnection() const noexcept
{
	return m_connectionData->GetConnection();
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE [[nodiscard]] const std::map<Timer, std::shared_ptr<EventType>>&
Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::GetOwnedEvents() const noexcept
{
	return m_ownedEvents;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE [[nodiscard]] std::string_view
Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::GetUids(
	const std::string& uids) const noexcept
{
	return { uids.data() + m_uidsOffset, m_uidsSize };
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::SetUidsSize(
	const uint64_t size) noexcept
{
	m_uidsSize = size;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::SetTerminationReason(
	const TerminationReason reason) noexcept
{
	m_reason = reason;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse>
FORCE_INLINE [[nodiscard]] bool
Distributor<Module, EventType, Filter, Impl>::Destination<IsSingleResponse>::FailOwnedEvents(
	const std::string_view error)
{
	for (const auto& [timestamp, event] : m_ownedEvents) {
		if (!SendFailed(event->GetId(), GetConnection(), error)) [[unlikely]] {
			return false;
		}
	}

	return true;
}

/*---------------------------------------------------------------------------------
Distributor
---------------------------------------------------------------------------------*/

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::Distributor(Module& authorization) noexcept
	: m_authorization{ authorization }
{
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE Distributor<Module, EventType, Filter, Impl>::~Distributor() noexcept
{
	decltype(m_connectionIdToEventsData) removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_connectionToEventsDataLock };
		removed.swap(m_connectionIdToEventsData);
	}

	for (const auto& [connectionId, data] : removed) {
		// Write locks inside.
		data->EraseEvents(TerminationReason::Cleanup);
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::Collect(
	const uint64_t id, const uint64_t hash, const std::shared_ptr<Connection::Data>& connectionData, Json&& json)
{
	if (json.GetValueType<bool>("interrupt") != nullptr) {
		const auto connectionId{ connectionData->GetConnectionId() };
		std::shared_ptr<EventsData> eventsData{ GetEventsData(connectionId) };
		// Read/write locks inside.
		if (eventsData->EraseEvent(id)) {
			LOG_PROTOCOL_NEW("Event id: {}, hash: {} for connection id: {} is interrupted", id, hash, connectionId);
			return true;
		}

		LOG_PROTOCOL_NEW("Event id: {}, hash: {} for connection id: {} is not found", id, hash, connectionId);
		return true;
	}

	std::shared_ptr<typename EventType::BaseT::HandlerDataT> handlerData;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_hashToHandlerDataLock };
		const auto it{ m_hashToHandlerData.find(hash) };
		if (it != m_hashToHandlerData.end()) {
			handlerData = it->second;
		}
	}

	if (handlerData == nullptr) {
		return SendFailed(id, connectionData->GetConnection(), "Unknown hash of the event");
	}

	if (!handlerData->IsPermissionRequired()) {
		Filter filter{ json };
		return static_cast<Impl*>(this)->Handle(
			id, hash, std::move(filter), connectionData, std::move(json), std::move(handlerData));
	}

	if (!m_authorization.IsAccessGranted(
			connectionData->GetConnectionId(), static_cast<Module::grade_t>(handlerData->GetGrade()))) {
		return SendFailed(id, connectionData->GetConnection(), "Access is not granted");
	}

	Filter filter{ json };
	return static_cast<Impl*>(this)->Handle(
		id, hash, std::move(filter), connectionData, std::move(json), std::move(handlerData));
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <typename Handler>
	requires HandlerT<Handler, EventType>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::SetHandlerWithPermissions(const uint64_t hash,
	Handler&& handler, typename EventType::BaseT::TerminationHandlerT terminationHandler, const Module::grade_t grade)
{
	static_assert(sizeof(int16_t) == sizeof(typename Module::grade_t), "Grade size is expected");
	static_assert(std::is_integral_v<int16_t> && std::is_integral_v<std::underlying_type_t<typename Module::grade_t>>,
		"Grade type category is expected");

	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_hashToHandlerDataLock };
		if (!m_hashToHandlerData.contains(hash)) [[likely]] {
			const auto result{ m_hashToHandlerData.emplace(hash,
				std::make_shared<typename EventType::BaseT::HandlerDataT>(
					std::move(handler), std::move(terminationHandler), static_cast<int16_t>(grade))) };
			if (result.second) [[likely]] {
				return;
			}
		}
	}

	LOG_WARNING_NEW("Event handler is already registered, hash: {}", hash);
	return;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <typename Handler>
	requires HandlerT<Handler, EventType>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::SetHandlerWithoutPermissions(
	const uint64_t hash, Handler&& handler, typename EventType::BaseT::TerminationHandlerT terminationHandler)
{
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_hashToHandlerDataLock };
		if (!m_hashToHandlerData.contains(hash)) [[likely]] {
			const auto result{ m_hashToHandlerData.emplace(hash,
				std::make_shared<typename EventType::BaseT::HandlerDataT>(
					std::move(handler), std::move(terminationHandler))) };
			if (result.second) [[likely]] {
				return;
			}
		}
	}

	LOG_WARNING_NEW("Event handler is already registered, hash: {}", hash);
	return;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::FailActiveEvents(
	const std::string_view error)
{
	// Write locks inside
	return FailActiveEvents(error, TerminationReason::Cleanup);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::FailActiveEvents(
	const std::string_view error, const TerminationReason reason)
{
	decltype(m_connectionIdToEventsData) removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_connectionToEventsDataLock };
		removed.swap(m_connectionIdToEventsData);
	}

	bool isSuccessful{ true };
	for (const auto& [connectionId, data] : removed) {
		// Write locks inside.
		if (!data->FailActiveEvents(error, reason)) {
			isSuccessful = false;
		}
	}

	return isSuccessful;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::ClearActiveEventsForConnectionId(
	const uint64_t connectionId)
{
	std::shared_ptr<EventsData> removed;
	{
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_connectionToEventsDataLock };
		const auto it{ m_connectionIdToEventsData.find(connectionId) };
		if (it == m_connectionIdToEventsData.end()) {
			return;
		}

		removed = std::move(it->second);
		m_connectionIdToEventsData.erase(it);
	}

	// Retain connection storage through removal and callbacks; write locks inside.
	removed->EraseEvents(TerminationReason::ConnectionClosed);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
template <bool IsSingleResponse, DataT T>
FORCE_INLINE [[nodiscard]] SendResult Distributor<Module, EventType, Filter, Impl>::SendData(
	const FilterT& filter, const T getData)
{
	// Read/Write lock inside.
	const std::vector<std::shared_ptr<Events>> eventsArray{ GetEventsArray(filter) };
	if (eventsArray.empty()) {
		return SendResult::Nothing;
	}

	thread_local std::string t_uids{};
	t_uids.clear();
	size_t maxUidsSize{};
	std::vector<Destination<IsSingleResponse>> destinations;
	const auto addDestination{ [&destinations, &maxUidsSize](const std::map<Timer, std::shared_ptr<EventType>>& items,
								   Destination<IsSingleResponse>&& destination) {
		auto begin{ items.begin() };
		const auto end{ items.end() };
		const auto uidsOffset{ t_uids.size() };
		auto backIt{ std::back_inserter(t_uids) };
		std::format_to(backIt, "{}", begin->second->GetId());

		while (++begin != end) {
			std::format_to(backIt, ",{}", begin->second->GetId());
		}

		const auto size{ t_uids.size() - uidsOffset };
		if (maxUidsSize < size) {
			maxUidsSize = size;
		}

		destination.SetUidsSize(size);
		destinations.emplace_back(std::move(destination));
	} };

	for (const auto& events : eventsArray) {
		if constexpr (IsSingleResponse) {
			// Read/Write lock inside.
			auto owned{ events->Take() };
			if (!owned.empty()) {
				Destination<IsSingleResponse> destination{ std::move(owned), t_uids.size() };
				addDestination(destination.GetOwnedEvents(), std::move(destination));
			}
		}
		else {
			const Lock::AtomicRW::Guard<Lock::READ> _{ events->GetLock() };
			const auto& items{ events->Get() };
			if (!items.empty()) {
				Destination<IsSingleResponse> destination{ items.begin()->second->GetConnectionData(), t_uids.size() };
				addDestination(items, std::move(destination));
			}
		}
	}

	if (destinations.empty()) {
		return SendResult::Nothing;
	}

	std::vector<uint64_t> failedConnections;
	const auto clearFailedConnections{ [this, &failedConnections] {
		for (const auto connectionId : failedConnections) {
			// Finish all delivery attempts before termination callbacks.
			// Read/Write lock inside.
			ClearActiveEventsForConnectionId(connectionId);
		}
	} };

	const auto failEvents{ [this, &filter, &destinations, &failedConnections, &clearFailedConnections](
							   const std::string_view error) {
		if constexpr (!IsSingleResponse) {
			// Read/Write lock inside.
			return FailEventsOnConnectionsByFilter(filter, error);
		}
		else {
			bool isSuccessful{ true };
			for (auto& destination : destinations) {
				if (!destination.FailOwnedEvents(error)) [[unlikely]] {
					isSuccessful = false;
					failedConnections.emplace_back(destination.GetConnection().GetId());
				}
			}

			clearFailedConnections();
			return isSuccessful;
		}
	} };

	maxUidsSize += 9;
	if (static_cast<int64_t>(maxUidsSize) < 0) [[unlikely]] {
		return failEvents("Data destination is unexpected") ? SendResult::Fail : SendResult::IOError;
	}

	thread_local std::string t_payload{};
	t_payload.clear();
	t_payload.resize(maxUidsSize, ' ');
	if constexpr (std::is_same_v<std::string_view, T>) {
		std::format_to(std::back_inserter(t_payload), "],\"data\":{}}}", getData);
	}
	else {
		thread_local std::string t_data{};
		t_data.clear();
		bool isProduced{};
		try {
			isProduced = getData(t_data);
		}
		catch (...) {
			LOG_ERROR_NEW("Event data producer failed, hash: {}", filter.first);
			return failEvents("Event data production failed") ? SendResult::Fail : SendResult::IOError;
		}
		if (!isProduced) {
			return failEvents(t_data) ? SendResult::Fail : SendResult::IOError;
		}
		std::format_to(std::back_inserter(t_payload), "],\"data\":{}}}", t_data);
	}

	const auto payloadTotalSize{ t_payload.size() };
	SendResult result{ SendResult::Success };
	for (auto& destination : destinations) {
		const auto destinationUids{ destination.GetUids(t_uids) };
		LOG_PROTOCOL_NEW(
			"Send data to events, uids: [{}], connection id: {}", destinationUids, destination.GetConnection().GetId());
		const auto headerSize{ destinationUids.size() + 9 };
		const auto headerShift{ maxUidsSize - headerSize };
		std::format_to_n(
			t_payload.data() + headerShift, static_cast<int64_t>(headerSize), "{{\"uids\":[{}", destinationUids);
		if (!Send(destination.GetConnection(),
				{ std::span<const uint8_t>(
					  reinterpret_cast<const uint8_t*>(t_payload.data() + headerShift), payloadTotalSize - headerShift),
					Data::Opcode::Text, /*mask=*/0, /*isFinal=*/true, /*rsv1=*/false, /*rsv2=*/false, /*rsv3=*/false }))
			[[unlikely]] {

			result = SendResult::IOError;

			if constexpr (IsSingleResponse) {
				destination.SetTerminationReason(TerminationReason::ConnectionClosed);
			}

			failedConnections.emplace_back(destination.GetConnection().GetId());
		}
		else if constexpr (IsSingleResponse) {
			destination.SetTerminationReason(TerminationReason::Completed);
		}
	}

	clearFailedConnections();

	return result;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::SetEventsLimit(
	const uint64_t connectionId, const int32_t value)
{
	// Read/write locks inside.
	const auto data{ GetEventsData(connectionId) };
	// Read/write locks inside when reducing the limit.
	return data->SetLimit(value);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::SetEventsPurgingCoefficient(
	const uint64_t connectionId, const float value)
{
	// Read/write locks inside.
	const auto data{ GetEventsData(connectionId) };
	return data->SetPurgingCoefficient(value);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] std::shared_ptr<typename Distributor<Module, EventType, Filter, Impl>::EventsData>
Distributor<Module, EventType, Filter, Impl>::GetEventsData(const uint64_t connectionId) noexcept
{
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_connectionToEventsDataLock };
		auto it{ m_connectionIdToEventsData.find(connectionId) };
		if (it != m_connectionIdToEventsData.end()) {
			return it->second;
		}
	}

	const Lock::AtomicRW::Guard<Lock::WRITE> _{ m_connectionToEventsDataLock };
	return m_connectionIdToEventsData.emplace(connectionId, std::make_shared<EventsData>(connectionId)).first->second;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] std::vector<std::shared_ptr<typename Distributor<Module, EventType, Filter, Impl>::Events>>
Distributor<Module, EventType, Filter, Impl>::GetEventsArray(const FilterT& filter) const noexcept
{
	std::shared_ptr<Events> events;
	std::vector<std::shared_ptr<Events>> eventsArray;
	const Lock::AtomicRW::Guard<Lock::READ> _{ m_connectionToEventsDataLock };
	for (const auto& [connectionId, eventsData] : m_connectionIdToEventsData) {
		events = eventsData->template GetEvents<EventsData::LOOKUP>(filter);
		if (events.get()) {
			// Interraction with events is required locking, same as for checking on empty
			// That is worth to lock once at the usage cycle
			eventsArray.emplace_back(std::move(events));
		}
	}

	return eventsArray;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::FailEventsOnConnectionsByFilter(
	const FilterT& filter, const std::string_view error)
{
	std::vector<std::shared_ptr<EventsData>> recipients;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_connectionToEventsDataLock };
		for (const auto& [connectionId, data] : m_connectionIdToEventsData) {
			recipients.emplace_back(data);
		}
	}

	bool isSuccessful{ true };
	for (const auto& data : recipients) {
		// Retain parent ownership during filtered failure and removal; read/write locks inside.
		if (!data->FailEventsByFilter(filter, error)) {
			isSuccessful = false;
		}
	}

	return isSuccessful;
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] bool Distributor<Module, EventType, Filter, Impl>::RegisterEvent(
	const FilterT& filter, std::shared_ptr<EventType> event)
{
	// Read/write locks inside.
	const auto data{ GetEventsData(event->GetConnectionData()->GetConnectionId()) };
	// Read/write locks inside.
	const auto events{ data->template GetEvents<EventsData::CREATE>(filter) };
	// Write locks and limit checking inside; data retains the accounting owner through purging callbacks.
	return events != nullptr && events->AddEvent(std::move(event));
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EraseEventsOnConnectionsByFilter(const FilterT& filter)
{
	// Read/write locks inside.
	EraseEventsOnConnectionsByFilter(filter, TerminationReason::RemovedByFilter);
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE void Distributor<Module, EventType, Filter, Impl>::EraseEventsOnConnectionsByFilter(
	const FilterT& filter, const TerminationReason reason)
{
	std::vector<std::shared_ptr<EventsData>> recipients;
	{
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_connectionToEventsDataLock };
		for (const auto& [connectionId, data] : m_connectionIdToEventsData) {
			recipients.emplace_back(data);
		}
	}

	for (const auto& data : recipients) {
		// Retain parent ownership during filtered removal; read/write locks inside.
		data->EraseEventsByFilter(filter, reason);
	}
}

template <typename Module, typename EventType, typename Filter, typename Impl>
FORCE_INLINE [[nodiscard]] HandleResult Distributor<Module, EventType, Filter, Impl>::HandleEvent(
	EventType& event, std::string& payload)
{
	// Event friendship reserves initial handler dispatch for distributor implementations.
	return event.Handle(payload);
}

/*---------------------------------------------------------------------------------
SinglesDistributor
---------------------------------------------------------------------------------*/

template <typename Module>
FORCE_INLINE SinglesDistributor<Module>::SinglesDistributor(Module& authorization) noexcept
	: BaseT{ authorization }
{
}

template <typename Module>
template <DataT T>
FORCE_INLINE [[nodiscard]] SendResult SinglesDistributor<Module>::CheckDelayed(const BaseT::FilterT& filter, T getData)
{
	// Read/Write lock inside.
	return BaseT::template SendData<BaseT::SINGLE_RESPONSE>(filter, std::move(getData));
}

template <typename Module>
FORCE_INLINE [[nodiscard]] bool SinglesDistributor<Module>::Handle(const uint64_t id, const uint64_t hash,
	IdentityFilter&& filter, const std::shared_ptr<Connection::Data>& connectionData, Json&& json,
	std::shared_ptr<typename Single::BaseT::HandlerDataT>&& handlerData)
{
	const auto connectionId{ connectionData->GetConnectionId() };
	LOG_PROTOCOL_NEW("New single event id: {} type hash: {} connection id: {}", id, hash, connectionId);

	thread_local std::string t_payload{};
	t_payload.clear();
	std::format_to(std::back_inserter(t_payload), "{{\"uids\":[{}],\"data\":", id);
	Single single{ id, std::move(handlerData), std::move(json), connectionData };

	const auto result{ BaseT::HandleEvent(single, t_payload) };
	switch (result) {
	case HandleResult::Success:
		t_payload += '}';
		LOG_PROTOCOL_NEW("Send single event id: {} success response, connection id: {}", id, connectionId);
		return Send(connectionData->GetConnection(),
			{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(t_payload.data()), t_payload.size()),
				Data::Opcode::Text, /*mask=*/0, /*isFinal=*/true, /*rsv1=*/false, /*rsv2=*/false, /*rsv3=*/false });
	case HandleResult::Fail:
		return SendFailed(id, connectionData->GetConnection(), t_payload);
	case HandleResult::Delay: {
		LOG_PROTOCOL_NEW("New single event id: {} is delayed, connection id: {}", id, connectionId);
		// Read/write locks inside.
		if (!BaseT::RegisterEvent({ hash, std::move(filter) }, std::make_shared<Single>(std::move(single))))
			[[unlikely]] {

			return SendFailed(id, connectionData->GetConnection(), "Event registration failed");
		}

		return true;
	}
	case HandleResult::IOError:
		return false;
	default:
		LOG_WARNING_NEW("Unexpected result of single event handling: {}", EnumToString(result));
		return true;
	}
}

/*---------------------------------------------------------------------------------
StreamsDistributor
---------------------------------------------------------------------------------*/

template <typename Module>
FORCE_INLINE StreamsDistributor<Module>::StreamsDistributor(Module& authorization) noexcept
	: BaseT{ authorization }
{
}

template <typename Module>
template <DataT T>
FORCE_INLINE [[nodiscard]] SendResult StreamsDistributor<Module>::SendData(const BaseT::FilterT& filter, T getData)
{
	// Read/Write lock inside.
	return BaseT::template SendData<BaseT::REPEATABLE_RESPONSE>(filter, std::move(getData));
}

template <typename Module>
FORCE_INLINE [[nodiscard]] bool StreamsDistributor<Module>::Handle(const uint64_t id, const uint64_t hash,
	IdentityFilter&& filter, const std::shared_ptr<Connection::Data>& connectionData, Json&& json,
	std::shared_ptr<typename Stream::BaseT::HandlerDataT>&& handlerData)
{
	const auto connectionId{ connectionData->GetConnectionId() };
	LOG_PROTOCOL_NEW("New stream event, id {} event type hash {} connection id {}", id, hash, connectionId);

	// Preserve the existing prefixed callback contract even when SuccessChunked leaves this buffer unused.
	// Its initialization cost is intentionally accepted; conditional initialization adds unjustified complexity.
	thread_local std::string t_payload{};
	t_payload.clear();
	std::format_to(std::back_inserter(t_payload), "{{\"uids\":[{}],\"data\":", id);
	auto stream{ std::make_shared<Stream>(id, std::move(handlerData), std::move(json), connectionData) };

	if (!stream->SendState(Stream::State::Opened)) [[unlikely]] {
		BaseT::ClearActiveEventsForConnectionId(connectionId);
		return false;
	}

	const auto result{ BaseT::HandleEvent(*stream, t_payload) };
	switch (result) {
	case HandleResult::Success:
	case HandleResult::SuccessChunked: {
		// Read/write locks inside.
		if (!BaseT::RegisterEvent({ hash, std::move(filter) }, stream)) [[unlikely]] {
			return SendFailed(id, connectionData->GetConnection(), "Stream registration failed");
		}

		if (result == HandleResult::Success) {
			t_payload += '}';
			if (!Send(connectionData->GetConnection(),
					{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(t_payload.data()), t_payload.size()),
						Data::Opcode::Text, /*mask=*/0, /*isFinal=*/true, /*rsv1=*/false, /*rsv2=*/false,
						/*rsv3=*/false })) [[unlikely]] {
				BaseT::ClearActiveEventsForConnectionId(connectionId);
				return false;
			}
		}

		if (!stream->SendState(Stream::State::Done)) [[unlikely]] {
			BaseT::ClearActiveEventsForConnectionId(connectionId);
			return false;
		}

		return true;
	}
	case HandleResult::IOError:
		BaseT::ClearActiveEventsForConnectionId(connectionId);
		return false;
	case HandleResult::Fail:
		return SendFailed(id, connectionData->GetConnection(), t_payload);
	default:
		LOG_WARNING_NEW("Unexpected result of stream event handling: {}", EnumToString(result));
		return true;
	}
}

} // namespace Events

} // namespace WebSocket

} // namespace Protocol

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_WEB_SOCKET_EVENTS_INL