/**************************
 * @file        webSocketEvents.inl
 * @version     6.0
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
 */

#ifndef MSAPI_PROTOCOL_WEBSOCKET_EVENTS_INL
#define MSAPI_PROTOCOL_WEBSOCKET_EVENTS_INL

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

/**
 * @brief Event communication model types.
 */
enum class Type : int8_t { Undefined, Single, Stream, Max };

/**
 * @return String interpretation of event type enum.
 *
 * @todo Add unit test.
 */
FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const Type type)
{
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

/**
 * @brief Handler return result.
 */
enum class HandleResult : int8_t { Undefined, Success, Fail, Delay, Max };

/**
 * @return String interpretation of handle result enum.
 *
 * @todo Add unit test.
 */
FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const HandleResult result)
{
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
	case HandleResult::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown handle result: {}", U(result));
		return "Unknown";
	}
}

/**
 * @brief Send return result.
 */
enum class SendResult : int8_t { Undefined, Success, Fail, Nothing, Max };

/**
 * @return String interpretation of send result enum.
 *
 * @todo Add unit test.
 */
FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const SendResult result)
{
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
	case SendResult::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown send result: {}", U(result));
		return "Unknown";
	}
}

/**
 * @brief Event holder, contains common data and handler function.
 *
 * @tparam EventType Type of an event.
 */
template <typename EventType> class Event {
public:
	using handler_t = std::function<HandleResult(std::string&, const EventType&)>;

	/**
	 * @brief Handler and access requirements holder.
	 */
	struct HandlerData {
		const handler_t handler;
		const int16_t grade;
		const bool isPermissionRequired;

		/**
		 * @brief Construct object with requirements.
		 *
		 * @param handler Event type specific handler.
		 * @param grade Minimal required grade.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE HandlerData(handler_t&& handler, const int16_t grade) noexcept
			: handler{ std::move(handler) }
			, grade{ grade }
			, isPermissionRequired{ true }
		{
		}

		/**
		 * @brief Construct object without requirements.
		 *
		 * @param handler Event type specific handler.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE HandlerData(handler_t&& handler) noexcept
			: handler{ std::move(handler) }
			, grade{}
			, isPermissionRequired{ false }
		{
		}

		HandlerData(const HandlerData& other) = delete;
		HandlerData(HandlerData&& other) = delete;
		HandlerData& operator=(const HandlerData& other) = delete;
		HandlerData& operator=(HandlerData&& other) = delete;
	};

	using handlerData_t = HandlerData;

private:
	const uint64_t m_uid;
	std::shared_ptr<HandlerData> m_handlerData;
	const Json m_json;
	const int32_t m_connection;

public:
	/**
	 * @brief Construct event holder.
	 *
	 * @param uid Event uid.
	 * @param handlerData Specific handler and access requirements holder.
	 * @param json Request json.
	 * @param connection Request connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Event(
		const uint64_t uid, std::shared_ptr<HandlerData>&& handlerData, Json&& json, const int32_t connection) noexcept
		: m_uid{ uid }
		, m_handlerData(std::move(handlerData))
		, m_json{ std::move(json) }
		, m_connection{ connection }
	{
	}

	Event(const Event& other) = delete;
	Event(Event&& other) = default;
	Event& operator=(const Event& other) = delete;
	Event& operator=(Event&& other) = default;

	/**
	 * @return Uid of event.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetUid() const noexcept { return m_uid; }

	/**
	 * @return Request json.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] const Json& GetJson() const noexcept { return m_json; }

	/**
	 * @return Request connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] int32_t GetConnection() const noexcept { return m_connection; }

	/**
	 * @brief Handle event by the specific handler.
	 *
	 * @param payload Preformed payload, is expected to be a valid json value in case of success, only error message on
	 * fail.
	 *
	 * @return Value of handler return result type.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] HandleResult Handle(std::string& payload)
	{
		return m_handlerData->handler(payload, *static_cast<const EventType*>(this));
	}
};

/**
 * @brief Filter by event identity.
 */
class IdentityFilter {
private:
	uint64_t identity;

public:
	/**
	 * @brief Create empty filter.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE IdentityFilter() noexcept
		: identity{}
	{
	}

	/**
	 * @brief Create filter.
	 *
	 * @param identity Event identity.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE IdentityFilter(const uint64_t identity) noexcept
		: identity{ identity }
	{
	}

	/**
	 * @brief Create filter from json. Filter is expected to be under "filter" key with unsigned type, empty filter is
	 * created by default.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE IdentityFilter(const Json& json) noexcept
	{
		if (const auto* filterValue{ json.GetValueType<uint64_t>("filter") }; filterValue != nullptr) {
			identity = *filterValue;
			return;
		}

		identity = 0;
	}

	IdentityFilter(const IdentityFilter& other) = default;
	IdentityFilter(IdentityFilter&& other) = default;
	IdentityFilter& operator=(const IdentityFilter& other) = default;
	IdentityFilter& operator=(IdentityFilter&& other) = default;

	/**
	 * @return True if values are equal.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool operator==(const IdentityFilter other) const noexcept
	{
		return identity == other.identity;
	}

	/**
	 * @return True if self value is less.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool operator<(const IdentityFilter other) const noexcept
	{
		return identity < other.identity;
	}

	/**
	 * @return True if self value is greater.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] bool operator>(const IdentityFilter other) const noexcept { return !operator<(other); }
};

/**
 * @brief The representation of the request with only one response, which can be delayed and responded later.
 */
class Single : public Event<Single> {
public:
	using base_t = Event<Single>;

public:
	/**
	 * @brief Construct single event holder.
	 *
	 * @param uid Event uid.
	 * @param handlerData Specific handler and access requirements holder.
	 * @param json Request json.
	 * @param connection Request connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Single(
		const uint64_t uid, std::shared_ptr<HandlerData>&& handlerData, Json&& json, const int32_t connection) noexcept
		: base_t{ uid, std::move(handlerData), std::move(json), connection }
	{
	}
};

/**
 * @brief The representation of the request with many responses. First call of handler will be addressed to just opened
 * stream. In places, where additional data can be sent, all related opened streams will be notified with it.
 *
 * Distributor does not contain stream event, but send updates to client in next way: pending -> opened (send available
 * data) -> snapshot done -> send new data -> closed by the client or failed.
 */
class Stream : public Event<Stream> {
public:
	using base_t = Event<Stream>;

	/**
	 * @brief State of stream.
	 */
	enum class State : int8_t { Undefined, Pending, Opened, Done, Failed, Closed, Removed, Max };

public:
	/**
	 * @brief Construct stream event holder.
	 *
	 * @param uid Event uid.
	 * @param handlerData Specific handler and access requirements holder.
	 * @param json Request json.
	 * @param connection Request connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Stream(
		const uint64_t uid, std::shared_ptr<HandlerData>&& handlerData, Json&& json, const int32_t connection) noexcept
		: base_t{ uid, std::move(handlerData), std::move(json), connection }
	{
	}

	/**
	 * @brief Send new stream state.
	 *
	 * @param state New state.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void SendState(const State state) const
	{
		LOG_PROTOCOL_NEW(
			"Send stream event {} state, uid {} connection {}", EnumToString(state), GetUid(), GetConnection());
		std::string payload{ std::format("{{\"uids\":[{}],\"state\":{}}}", GetUid(), U(state)) };
		Data data{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
			Data::Opcode::Text };
		Send(GetConnection(), data);
	}

	/**
	 * @return String interpretation of stream state enum.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] static std::string_view EnumToString(const State state)
	{
		// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.
		// Undefined, Pending, Opened, Done, Failed, Closed, Removed, Max
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
};

/**
 * @brief Send failed event state with error.
 *
 * @param uid Event uid.
 * @param connection Request connection.
 * @param error Description of the failure.
 *
 * @todo Add unit test.
 */
FORCE_INLINE static void SendFailed(const uint64_t uid, const int32_t connection, const std::string_view error)
{
	static_assert(static_cast<int32_t>(Stream::State::Failed) == 4, "Stream failed state is expected");
	LOG_PROTOCOL_NEW("Send stream event failed state, uid {} error {} connection {}", uid, error, connection);
	std::string payload{ std::format("{{\"uids\":[{}],\"state\":4,\"error\":\"{}\"}}", uid, error) };
	Data data{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
		Data::Opcode::Text };
	Send(connection, data);
}

/**
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
 * @tparam Impl Type with Handle implementation.
 */
template <typename Module, typename EventType, typename Filter, typename Impl> class Distributor {
public:
	class EventsData;

	using filter_t = std::pair<uint64_t, Filter>;

	/**
	 * @brief Events under same filter.
	 */
	class Events {
	private:
		std::map<Timer, std::shared_ptr<EventType>> m_events;
		Pthread::AtomicRWLock m_eventsLock;
		EventsData& m_data;

	public:
		/**
		 * @brief Construct new events object.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE Events(EventsData& data) noexcept
			: m_data{ data }
		{
		}

		Events(const Events& other) = delete;
		Events(Events&& other) = delete;
		Events& operator=(const Events& other) = delete;
		Events& operator=(Events&& other) = delete;

		/**
		 * @brief Add new event with current timestamp, increase stored events size and purge events in case of limit
		 * exceeding.
		 *
		 * @attention Write locks the structure. Does not change limit usage in case of emplace failure.
		 *
		 * @tparam Universal R or L-reference of the event type.
		 *
		 * @param event Event to be added.
		 *
		 * @todo Add unit test.
		 */
		template <typename Universal>
			requires std::is_same_v<std::decay_t<Universal>, std::shared_ptr<EventType>>
		FORCE_INLINE void AddEvent(Universal&& event) noexcept
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_eventsLock };
			const Timer timestamp{};
			const auto result{ m_events.emplace(timestamp, std::forward<Universal>(event)) };
			if (!result.second) [[unlikely]] {
				LOG_WARNING_NEW(
					"Events with uid {} is not emplaced, connection ", event->GetUid(), m_data.GetConnection());
				return;
			}

			LOG_PROTOCOL_NEW("Event uid {} is added at {}, connection {}", result.first->second->GetUid(),
				timestamp.ToString(), m_data.GetConnection());
			m_data.IncreaseEventsSize(1);
		}

		/**
		 * @brief Fail contained events and erase them.
		 *
		 * @attention Write locks structure.
		 *
		 * @param error Description of the failure.
		 *
		 * @todo Add unit test.
		 * @todo Events can be failed in bunches.
		 */
		FORCE_INLINE void FailEvents(const std::string_view error)
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_eventsLock };
			const auto size{ m_events.size() };
			if (size == 0) {
				return;
			}

			for (const auto& event : m_events) {
				SendFailed(event.second->GetUid(), event.second->GetConnection(), error);
			}

			m_events.clear();
			m_data.DecreaseEventsSize(static_cast<int32_t>(size));
		}

		/**
		 * @brief Erase contained events.
		 *
		 * @attention Write locks structure.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void EraseEvents()
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_eventsLock };
			const auto size{ m_events.size() };
			if (size == 0) {
				return;
			}

			LOG_PROTOCOL_NEW("{} Events are removed, connection {}", size, m_data.GetConnection());

			m_events.clear();
			m_data.DecreaseEventsSize(static_cast<int32_t>(size));
		}

		/**
		 * @brief Erase event by its timestamp and decrease stored events size.
		 *
		 * @attention Write locks the structure. Does not change limit usage in case of erase failure.
		 *
		 * @param timestamp Timestamp when event was added.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void EraseEvent(const Timer timestamp) noexcept
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_eventsLock };
			if (m_events.erase(timestamp) == 0) [[unlikely]] {
				LOG_WARNING_NEW("Events with timestamp {} is not erased, connection ", timestamp.ToString(),
					m_data.GetConnection());
				return;
			}

			LOG_PROTOCOL_NEW(
				"Event is removed, timestamp {} connection {}", timestamp.ToString(), m_data.GetConnection());
			m_data.DecreaseEventsSize(1);
		}

		/**
		 * @brief Erase bunch of events by their timestamps and decrease stored events size.
		 *
		 * @attention Write locks the structure. Change limit usage only on number of successfully erased events.
		 *
		 * @tparam Container Forward container with Timer types.
		 *
		 * @param timestamps Container with event timestamps to be erased.
		 *
		 * @todo Add unit test.
		 */
		template <template <typename> typename Container>
		FORCE_INLINE void EraseEventsByTimestamps(const Container<Timer>& timestamps)
		{
			int32_t erased{};
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_eventsLock };
			for (const auto timestamp : timestamps) {
				if (m_events.erase(timestamp) == 0) [[unlikely]] {
					LOG_WARNING_NEW("Events with timestamp {} is not erased, connection ", timestamp.ToString(),
						m_data.GetConnection());
					continue;
				}

				LOG_PROTOCOL_NEW(
					"Event is removed, timestamp {} connection {}", timestamp.ToString(), m_data.GetConnection());
				++erased;
			}

			m_data.DecreaseEventsSize(erased);
		}

		/**
		 * @return Read-write structure lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] Pthread::AtomicRWLock& GetLock() noexcept { return m_eventsLock; }

		/**
		 * @attention Is assumed to be used under structure locking.
		 *
		 * @return Readable reference to stored events.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] const std::map<Timer, std::shared_ptr<EventType>>& Get() const noexcept
		{
			return m_events;
		}
	};

	/**
	 * @brief Collection of events by their filter under same connection, controls events limit Default events size
	 * limit is 1024 and purging coefficient is 30%.
	 */
	class EventsData {
	private:
		std::map<filter_t, std::shared_ptr<Events>> m_filterToEvents;
		Pthread::AtomicRWLock m_filterToEventsLock;
		std::atomic<int32_t> m_limit{ 1024 };
		std::atomic<float> m_purgingCoefficient{ 0.3f };
		std::atomic<int32_t> m_eventsSize{};
		const int32_t m_connection;

	public:
		/**
		 * @brief Construct new events data object.
		 *
		 * @param connection Related connection.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE EventsData(const int32_t connection) noexcept
			: m_connection{ connection }
		{
		}

		EventsData(const EventsData& other) = delete;
		EventsData(EventsData&& other) = delete;
		EventsData& operator=(const EventsData& other) = delete;
		EventsData& operator=(EventsData&& other) = delete;

		/**
		 * @return Related connection.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] int32_t GetConnection() const noexcept { return m_connection; }

		/**
		 * @attention Read locks structure. Write locks structure and create new events structure for filter if does not
		 * exist.
		 *
		 * @param filter Filter to find related events.
		 *
		 * @return Events by the specific filter.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] std::shared_ptr<Events> GetEvents(const filter_t& filter) noexcept
		{
			{
				const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_filterToEventsLock };
				auto it{ m_filterToEvents.find(filter) };
				if (it != m_filterToEvents.end()) {
					return it->second;
				}
			}

			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_filterToEventsLock };
			return m_filterToEvents.emplace(filter, std::make_shared<Events>(*this)).first->second;
		}

		/**
		 * @brief Fail events by filter.
		 *
		 * @attention Read lock structure to find events and write locks events structure to perform the action.
		 *
		 * @param filter Events related filter.
		 * @param error Description of the failure.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void FailEventsByFilter(const filter_t& filter, const std::string_view error)
		{
			std::shared_ptr<Events> events;
			{
				const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_filterToEventsLock };
				auto it{ m_filterToEvents.find(filter) };
				if (it == m_filterToEvents.end()) {
					return;
				}
				events = it->second;
			}

			events->FailEvents(error);
		}

		/**
		 * @brief Erase events by filter.
		 *
		 * @attention Read lock structure to find events and write locks events structure to perform the action.
		 *
		 * @param filter Events related filter.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void EraseEventsByFilter(const filter_t& filter)
		{
			std::shared_ptr<Events> events;
			{
				const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_filterToEventsLock };
				auto it{ m_filterToEvents.find(filter) };
				if (it == m_filterToEvents.end()) {
					return;
				}
				events = it->second;
			}

			events->EraseEvents();
		}

		/**
		 * @brief Erase event by its uid. Expensive operation as requires O(n) searching.
		 *
		 * @attention Read locks structure. Read locks events structures one by one during searching and write lock
		 * during erasing.
		 *
		 * @param uid Event uid to be erased.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool EraseEvent(const uint64_t uid) noexcept
		{
			Timer targetTimestamp{ 0 };
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_filterToEventsLock };
			for (auto& [filter, events] : m_filterToEvents) {
				{
					const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ events->GetLock() };
					const auto& items{ events->Get() };
					for (const auto& [timestamp, event] : items) {
						if (event->GetUid() == uid) {
							targetTimestamp = timestamp;
							break;
						}
					}
				}

				if (!targetTimestamp.Empty()) {
					events->EraseEvent(targetTimestamp);
					return true;
				}
			}

			return false;
		}

		/**
		 * @brief Fail all stored events with error message and decrease stored events size.
		 *
		 * @attention Write locks structure and events structures one by one during failing.
		 *
		 * @param error Description of the failure.
		 *
		 * @todo Add unit test.
		 * @todo Events can be failed in bunches.
		 */
		FORCE_INLINE void FailActiveEvents(const std::string_view error)
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_filterToEventsLock };
			auto eventsBegin{ m_filterToEvents.begin() };
			auto eventsEnd{ m_filterToEvents.end() };

			for (; eventsBegin != eventsEnd;) {
				const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ eventsBegin->second->GetLock() };
				const auto& events{ eventsBegin->second->Get() };
				for (const auto& [timestamp, event] : events) {
					SendFailed(event->GetUid(), event->GetConnection(), error);
				}

				DecreaseEventsSize(static_cast<int32_t>(events.size()));
				eventsBegin = m_filterToEvents.erase(eventsBegin);
			}
		}

		/**
		 * @brief Decrease events size.
		 *
		 * @param value Value to subtract.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void DecreaseEventsSize(const int32_t value) noexcept { m_eventsSize.fetch_sub(value); }

		/**
		 * @brief Increase events size.
		 *
		 * @param value Value to add.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void IncreaseEventsSize(const int32_t value)
		{
			m_eventsSize.fetch_add(value);
			CheckLimitAndPurge();
		}

		/**
		 * @return Events size limit.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] int32_t GetLimit() const noexcept { return m_limit.load(); }

		/**
		 * @brief Set new events limit. If new limit is less than current, the limit checking is performed.
		 *
		 * @param value New limit, cannot be less than 64.
		 *
		 * @return True if limit is updated, false otherwise.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool SetLimit(const int32_t value)
		{
			if (value < 64) [[unlikely]] {
				LOG_WARNING_NEW(
					"Events purging limit cannot be less than 64, provided {}, connection {}", value, m_connection);
				return false;
			}

			const auto old{ m_limit.load() };
			const auto ratio{ old - value };
			if (ratio == 0) [[unlikely]] {
				return false;
			}

			LOG_PROTOCOL_NEW("Events purging limit is changed from {} to {}, connection {}", old, value, m_connection);
			m_limit.store(value);
			if (ratio < 0) {
				CheckLimitAndPurge();
			}
			return true;
		}

		/**
		 * @return Events purging coefficient.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] float GetPurgingCoefficient() const noexcept { return m_purgingCoefficient.load(); }

		/**
		 * @brief Set new purging events coefficient.
		 *
		 * @param value New coefficient, cannot be less than 5% or greater than 99%.
		 *
		 * @return True if coefficient is updated, false otherwise.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE [[nodiscard]] bool SetPurgingCoefficient(const float value)
		{
			if (Helper::FloatLess(value, 0.05f)) [[unlikely]] {
				LOG_WARNING_NEW("Events purging coefficient cannot be less than 0.05, provided {:.9f}, connection {}",
					value, m_connection);
				return false;
			}

			if (Helper::FloatGreater(value, 0.99f)) [[unlikely]] {
				LOG_WARNING_NEW(
					"Events purging coefficient cannot be greater than 0.99, provided {:.9f}, connection {}", value,
					m_connection);
				return false;
			}

			const auto current{ m_purgingCoefficient.load() };
			if (Helper::FloatEqual(current, value)) [[unlikely]] {
				return false;
			}

			LOG_PROTOCOL_NEW("Events purging coefficient is changed from {:.9f} to {:.9f}, connection {}", current,
				value, m_connection);
			m_purgingCoefficient.store(value);
			return true;
		}

		/**
		 * @brief Check if limit is exceeded and perform purging.
		 *
		 * @attention Read locks structure and events structures on events sorting and write locks structure and events
		 * structures on erasing.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE void CheckLimitAndPurge()
		{
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
				const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_filterToEventsLock };
				for (auto& [filter, events] : m_filterToEvents) {
					const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ events->GetLock() };
					for (auto& [timestamp, event] : events->Get()) {
						sortedEvents.emplace(timestamp, events);
					}
				}
			}

			if (const auto size{ sortedEvents.size() }; size < static_cast<size_t>(toBePurged)) [[unlikely]] {
				LOG_WARNING_NEW("Unexpectedly size of sorted events {} is less that should be purged {}, purging "
								"coefficient {:.9f}, connection {}. Purging all.",
					size, toBePurged, purgingCoefficient, m_connection);
				FailActiveEvents("All events are purged due to unexpected processing");
				return;
			}

			LOG_PROTOCOL_NEW("Events limit {} is exceeded and {} events are going to be purged, connection {}", limit,
				toBePurged, m_connection);
			std::map<std::shared_ptr<Events>, std::vector<Timer>> sortedTimestamps;
			for (auto begin{ sortedEvents.begin() }, end{ sortedEvents.end() }; toBePurged > 0;) {
				sortedTimestamps[begin->second].emplace_back(begin->first);
				++begin;
				--toBePurged;
			}

			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_filterToEventsLock };
			for (auto& [events, timestamps] : sortedTimestamps) {
				events->EraseEventsByTimestamps(timestamps);
			}
		}
	};

private:
	Module& m_authorization;
	std::map<uint64_t, std::shared_ptr<typename EventType::base_t::handlerData_t>> m_hashToHandlerData;
	Pthread::AtomicRWLock m_hashToHandlerDataLock;
	std::map<int32_t, std::shared_ptr<EventsData>> m_connectionToEventsData;
	Pthread::AtomicRWLock m_connectionToEventsDataLock;

public:
	/**
	 * @brief Construct new distributor object.
	 *
	 * @param authorization Authorization module to access checking.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Distributor(Module& authorization) noexcept
		: m_authorization{ authorization }
	{
	}

	Distributor(const Distributor& other) = delete;
	Distributor(Distributor&& other) = delete;
	Distributor& operator=(const Distributor& other) = delete;
	Distributor& operator=(Distributor&& other) = delete;

	/**
	 * @brief Collect newly received event, check if it an interruption, look for specific handler and verify access
	 * right to the data and call handler on success. Erase on interruption.
	 *
	 * @param uid Event uid.
	 * @param hash Event type hash.
	 * @param connection Request connection.
	 * @param json Request json.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Collect(const uint64_t uid, const uint64_t hash, const int32_t connection, Json&& json)
	{
		if (json.GetValueType<bool>("interrupt") != nullptr) {
			std::shared_ptr<EventsData> eventsData{ GetEventsData(connection) };
			if (eventsData->EraseEvent(uid)) {
				LOG_PROTOCOL_NEW("Event for connection {} with uid {} hash {} is interrupted", connection, uid, hash);
				return;
			}

			LOG_PROTOCOL_NEW("Event for connection {} with uid {} hash {} is not found", connection, uid, hash);
			return;
		}

		std::shared_ptr<typename EventType::base_t::handlerData_t> handlerData;
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_hashToHandlerDataLock };
			const auto it{ m_hashToHandlerData.find(hash) };
			if (it == m_hashToHandlerData.end()) {
				SendFailed(uid, connection, "Unknown hash of the event");
				return;
			}

			handlerData = it->second;
		}

		if (!handlerData->isPermissionRequired) {
			Filter filter{ json };
			static_cast<Impl*>(this)->Handle(
				uid, hash, std::move(filter), connection, std::move(json), std::move(handlerData));
			return;
		}

		if (!m_authorization.IsAccessGranted(connection, static_cast<Module::grade_t>(handlerData->grade))) {
			SendFailed(uid, connection, "Access is not granted");
			return;
		}

		Filter filter{ json };
		static_cast<Impl*>(this)->Handle(
			uid, hash, std::move(filter), connection, std::move(json), std::move(handlerData));
	}

	/**
	 * @brief Set handler with permission requirements to specific event type hash.
	 *
	 * @attention Write locks handler data structure.
	 *
	 * @tparam Handler Type of the event handler function.
	 *
	 * @param hash Event type hash.
	 * @param handler Handler function.
	 * @param grade Required minimum grade.
	 *
	 * @todo Add unit test.
	 */
	template <typename Handler>
		requires std::is_convertible_v<Handler, typename EventType::base_t::handler_t>
	FORCE_INLINE void SetHandlerWithPermissions(const uint64_t hash, Handler&& handler, const Module::grade_t grade)
	{
		static_assert(sizeof(int16_t) == sizeof(typename Module::grade_t), "Grade size is expected");
		static_assert(
			std::is_integral_v<int16_t> && std::is_integral_v<std::underlying_type_t<typename Module::grade_t>>,
			"Grade type category is expected");

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_hashToHandlerDataLock };
		m_hashToHandlerData.emplace(hash,
			std::make_shared<typename EventType::base_t::handlerData_t>(
				std::move(handler), static_cast<int16_t>(grade)));
	}

	/**
	 * @brief Set handler without permission requirements to specific event type hash.
	 *
	 * @attention Write locks handler data structure.
	 *
	 * @tparam Handler Type of the event handler function.
	 *
	 * @param hash Event type hash.
	 * @param handler Handler function.
	 *
	 * @todo Add unit test.
	 */
	template <typename Handler>
		requires std::is_convertible_v<Handler, typename EventType::base_t::handler_t>
	FORCE_INLINE void SetHandlerWithoutPermissions(const uint64_t hash, Handler&& handler)
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_hashToHandlerDataLock };
		m_hashToHandlerData.emplace(
			hash, std::make_shared<typename EventType::base_t::handlerData_t>(std::move(handler)));
	}

	/**
	 * @brief Fail all stored events with error message.
	 *
	 * @attention Write locks structure, data events structures and events structures one by one during failing.
	 *
	 * @param error Description of the failure.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void FailActiveEvents(const std::string_view error)
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_connectionToEventsDataLock };
		auto eventsDataBegin{ m_connectionToEventsData.begin() };
		auto eventsDataEnd{ m_connectionToEventsData.end() };

		for (; eventsDataBegin != eventsDataEnd;) {
			eventsDataBegin->second->FailActiveEvents(error);
			eventsDataBegin = m_connectionToEventsData.erase(eventsDataBegin);
		}
	}

	/**
	 * @brief Clear all stored events for specific connection.
	 *
	 * @attention Write locks structure.
	 *
	 * @param connection Related connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void ClearActiveEventsForConnection(const int32_t connection)
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_connectionToEventsDataLock };
		m_connectionToEventsData.erase(connection);
	}

	/**
	 * @brief Send data to bunch of events by filter for each connection.
	 *
	 * @tparam T Type of data, string view or callable to fill provided string.
	 *
	 * @param filter Related hash filter to events.
	 * @param getData Data to be sent or lazy invoked function, should fill string argument with valid json key on true
	 * return and fill with error description otherwise.
	 *
	 * @return Send result enum.
	 *
	 * @todo Add unit test.
	 */
	template <typename T>
		requires(std::is_same_v<std::string_view, T> || std::is_convertible_v<T, std::function<bool(std::string&)>>)
	FORCE_INLINE [[nodiscard]] SendResult SendData(const filter_t& filter, const T getData)
	{
		std::vector<std::shared_ptr<Events>> eventsArray{ GetEventsArray(filter) };
		if (eventsArray.empty()) {
			return SendResult::Nothing;
		}

		// Trade of from locking while sending to once locking but string allocating
		struct Destination {
			const int32_t connection;
			std::string uids;

			FORCE_INLINE Destination(const int32_t connection) noexcept
				: connection{ connection }
			{
			}
		};

		size_t maxUidsSize{};
		std::vector<Destination> destinations;
		for (auto& events : eventsArray) {
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ events->GetLock() };
			const auto& items{ events->Get() };
			auto begin{ items.begin() };
			const auto end{ items.end() };

			if (begin != end) {
				Destination destination{ begin->second->GetConnection() };
				auto backIt{ std::back_inserter(destination.uids) };
				std::format_to(backIt, "{}", begin->second->GetUid());

				while (++begin != end) {
					std::format_to(backIt, ",{}", begin->second->GetUid());
				}

				const auto size{ destination.uids.size() };
				if (maxUidsSize < size) {
					maxUidsSize = size;
				}

				destinations.emplace_back(std::move(destination));
			}
		}

		if (destinations.empty()) {
			return SendResult::Nothing;
		}

		maxUidsSize += 9;
		if (static_cast<int64_t>(maxUidsSize) < 0) [[unlikely]] {
			FailEventsOnConnectionsByFilter(filter, "Data destination is unexpected");
			return SendResult::Fail;
		}

		std::string payload(maxUidsSize, ' ');
		if constexpr (std::is_same_v<std::string_view, T>) {
			std::format_to(std::back_inserter(payload), "],\"data\":{}}}", getData);
		}
		else {
			std::string data;
			if (!getData(data)) {
				FailEventsOnConnectionsByFilter(filter, data);
				return SendResult::Fail;
			}
			std::format_to(std::back_inserter(payload), "],\"data\":{}}}", data);
		}

		const auto payloadTotalSize{ payload.size() };
		for (const auto& destination : destinations) {
			LOG_PROTOCOL_NEW("Send data to events, uids [{}] connection {}", destination.uids, destination.connection);
			const auto headerSize{ destination.uids.size() + 9 };
			const auto headerShift{ maxUidsSize - headerSize };
			std::format_to_n(
				payload.data() + headerShift, static_cast<int64_t>(headerSize), "{{\"uids\":[{}", destination.uids);
			Send(destination.connection,
				{ std::span<const uint8_t>(
					  reinterpret_cast<const uint8_t*>(payload.data() + headerShift), payloadTotalSize - headerShift),
					Data::Opcode::Text });
		}

		return SendResult::Success;
	}

	/**
	 * @attention Read locks structure. Write locks structure and create new events data structure for filter if does
	 * not exist.
	 *
	 * @param connection Related connection.
	 *
	 * @return Events data by connection.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::shared_ptr<EventsData> GetEventsData(const int32_t connection) noexcept
	{
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_connectionToEventsDataLock };
			auto it{ m_connectionToEventsData.find(connection) };
			if (it != m_connectionToEventsData.end()) {
				return it->second;
			}
		}

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> _{ m_connectionToEventsDataLock };
		return m_connectionToEventsData.emplace(connection, std::make_shared<EventsData>(connection)).first->second;
	}

	/**
	 * @attention Read locks structure.
	 *
	 * @param filter Filter to find related events.
	 *
	 * @return Array of non empty events data from all stored connections.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] std::vector<std::shared_ptr<Events>> GetEventsArray(const filter_t& filter) noexcept
	{
		std::vector<std::shared_ptr<Events>> eventsArray;
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_connectionToEventsDataLock };
		for (const auto& [connection, eventsData] : m_connectionToEventsData) {
			eventsArray.emplace_back(std::move(eventsData->GetEvents(filter)));
		}

		return eventsArray;
	}

	/**
	 * @brief Fail events by filter on each connection.
	 *
	 * @attention Read locks structure, read lock events data structures and write locks events structures.
	 *
	 * @param filter Filter to find related events.
	 * @param error Description of the failure.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void FailEventsOnConnectionsByFilter(const filter_t& filter, std::string_view error)
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_connectionToEventsDataLock };
		for (const auto& [connection, eventsData] : m_connectionToEventsData) {
			eventsData->FailEventsByFilter(filter, error);
		}
	}

	/**
	 * @brief Erase events by filter on each connection.
	 *
	 * @attention Read locks structure, read lock events data structures and write locks events structures.
	 *
	 * @param filter Filter to find related events.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void EraseEventsOnConnectionsByFilter(const filter_t& filter)
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> _{ m_connectionToEventsDataLock };
		for (const auto& [connection, eventsData] : m_connectionToEventsData) {
			eventsData->EraseEventsByFilter(filter);
		}
	}
};

/**
 * @brief Distributor of single events.
 *
 * @tparam Module Type of the authorization module.
 */
template <typename Module>
class SinglesDistributor : public Distributor<Module, Single, IdentityFilter, SinglesDistributor<Module>> {
public:
	using base_t = Distributor<Module, Single, IdentityFilter, SinglesDistributor<Module>>;

	/**
	 * @brief Construct new singles distributor object.
	 *
	 * @param authorization Authorization module to access checking.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE SinglesDistributor(Module& authorization) noexcept
		: base_t{ authorization }
	{
	}

	/**
	 * @brief Send data to related events and erase them on success.
	 *
	 * @tparam T Type of data, string view or callable to fill provided string.
	 *
	 * @param filter Related hash filter to events.
	 * @param getData Data to be sent or lazy invoked function, should fill string argument with valid json key on true
	 * return and fill with error description otherwise.
	 *
	 * @todo Add unit test.
	 */
	template <typename T>
		requires(std::is_same_v<std::string_view, T> || std::is_convertible_v<T, std::function<bool(std::string&)>>)
	FORCE_INLINE void CheckDelayed(const base_t::filter_t& filter, const T getData)
	{
		if (this->SendData(filter, getData) == SendResult::Success) {
			this->EraseEventsOnConnectionsByFilter(filter);
		}
	}

private:
	/**
	 * @brief Handle newly collected event.
	 *
	 * @param uid Event uid.
	 * @param hash Event type hash.
	 * @param filter Event filter.
	 * @param connection Related connection.
	 * @param json Request json.
	 * @param handlerData Related handler and access data.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Handle(const uint64_t uid, const uint64_t hash, IdentityFilter&& filter, const int32_t connection,
		Json&& json, std::shared_ptr<typename Single::base_t::handlerData_t>&& handlerData)
	{
		LOG_PROTOCOL_NEW("New single event, uid {} event type hash {} connection {}", uid, hash, connection);

		std::string payload{ std::format("{{\"uids\":[{}],\"data\":", uid) };
		Single single{ uid, std::move(handlerData), std::move(json), connection };

		const auto result{ single.Handle(payload) };
		switch (result) {
		case HandleResult::Success:
			payload += '}';
			LOG_PROTOCOL_NEW("Send single event success response, uid {} connection {}", uid, connection);
			Send(connection,
				{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
					Data::Opcode::Text });
			return;
		case HandleResult::Fail:
			SendFailed(uid, connection, payload);
			return;
		case HandleResult::Delay: {
			std::shared_ptr<typename base_t::EventsData> eventsData{ this->GetEventsData(connection) };
			std::shared_ptr<typename base_t::Events> events{ eventsData->GetEvents(
				typename base_t::filter_t{ hash, std::move(filter) }) };

			LOG_PROTOCOL_NEW("New single event is delayed, uid {} connection {}", uid, connection);
			events->AddEvent(std::make_shared<Single>(std::move(single)));
			return;
		}
		default:
			LOG_WARNING_NEW("Unexpected result of single event handling: {}", EnumToString(result));
			return;
		}
	}

	// Access to Handle
	friend base_t;
};

/**
 * @brief Distributor of stream events.
 *
 * @tparam Module Type of the authorization module.
 */
template <typename Module>
class StreamsDistributor : public Distributor<Module, Stream, IdentityFilter, StreamsDistributor<Module>> {
public:
	using base_t = Distributor<Module, Stream, IdentityFilter, StreamsDistributor<Module>>;

	/**
	 * @brief Construct new streams distributor object.
	 *
	 * @param authorization Authorization module to access checking.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE StreamsDistributor(Module& authorization) noexcept
		: base_t{ authorization }
	{
	}

private:
	/**
	 * @brief Handle newly collected event.
	 *
	 * @param uid Event uid.
	 * @param hash Event type hash.
	 * @param filter Event filter.
	 * @param connection Related connection.
	 * @param json Request json.
	 * @param handlerData Related handler and access data.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Handle(const uint64_t uid, const uint64_t hash, IdentityFilter&& filter, const int32_t connection,
		Json&& json, std::shared_ptr<typename Stream::base_t::handlerData_t>&& handlerData)
	{
		LOG_PROTOCOL_NEW("New stream event, uid {} event type hash {} connection {}", uid, hash, connection);

		std::string payload{ std::format("{{\"uids\":[{}],\"data\":", uid) };
		auto stream{ std::make_shared<Stream>(uid, std::move(handlerData), std::move(json), connection) };

		const auto result{ stream->Handle(payload) };
		switch (result) {
		case HandleResult::Success: {
			stream->SendState(Stream::State::Opened);
			std::shared_ptr<typename base_t::EventsData> eventsData{ this->GetEventsData(connection) };
			std::shared_ptr<typename base_t::Events> events{ eventsData->GetEvents(
				typename base_t::filter_t{ hash, std::move(filter) }) };
			events->AddEvent(stream);

			payload += '}';
			LOG_PROTOCOL_NEW("Send stream event success response, uid {} connection {}", uid, connection);
			Send(connection,
				{ std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
					Data::Opcode::Text });
			stream->SendState(Stream::State::Done);
			return;
		}
		case HandleResult::Fail:
			SendFailed(uid, connection, payload);
			return;
		default:
			LOG_WARNING_NEW("Unexpected result of single event handling: {}", EnumToString(result));
			return;
		}
	}

	// Access to Handle
	friend base_t;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

} // namespace Events

} // namespace WebSocket

} // namespace Protocol

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_WEBSOCKET_EVENTS_INL