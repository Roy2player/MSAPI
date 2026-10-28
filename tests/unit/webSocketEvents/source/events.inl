/**************************
 * @file        main.cpp
 * @date        2026-10-24
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

#ifndef MSAPI_UNIT_TEST_WEB_SOCKET_EVENTS_INL
#define MSAPI_UNIT_TEST_WEB_SOCKET_EVENTS_INL

#include "../../../../library/source/protocol/webSocketEvents.inl"
#include "../../../../library/source/test/test.inl"
#include <barrier>
#include <stdexcept>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Authorization stub for public and permission-checked handler registration.
 *
 * @concurrency No.
 */
class EventsAuthorization {
public:
	/**************************
	 * @brief Minimum grade used by the authorization stub.
	 */
	enum class Grade : int16_t { Reader };
	using grade_t = Grade;

public:
	FORCE_INLINE [[nodiscard]] bool IsAccessGranted(uint64_t id, Grade grade) const noexcept;
};

/**************************
 * @brief Creates a connection whose sender records decoded text payloads from real WebSocket frames.
 *
 * @param messages Destination for payloads; outlives the connection and all send operations.
 *
 * @return Connection data or nullptr if socket creation fails.
 */
FORCE_INLINE [[nodiscard]] std::shared_ptr<MSAPI::Connection::Data> MakeEventsConnection(
	std::vector<std::string>& messages);

/**************************
 * @brief Tests synchronous regular, chunked, partial and failed initialization, recipient isolation, broadcasts,
 * owned delayed batches, every termination reason, reentrant callbacks, deferred purges, exception containment and
 * racing removals, enum numeric values and string conversions.
 *
 * @return True if all assertions pass.
 */
FORCE_INLINE [[nodiscard]] bool WebSocketEvents();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
EventsAuthorization
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool EventsAuthorization::IsAccessGranted(
	[[maybe_unused]] const uint64_t id, [[maybe_unused]] const Grade grade) const noexcept
{
	return true;
}

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] std::shared_ptr<MSAPI::Connection::Data> MakeEventsConnection(
	std::vector<std::string>& messages)
{
	// Real descriptors keep Connection cleanup valid.
	int32_t sockets[2]{};
	if (socketpair(AF_UNIX, SOCK_STREAM, /*protocol=*/0, sockets) != 0) {
		return {};
	}

	auto connection{ std::make_unique<MSAPI::Connection>(sockets[0]) };
	connection->SetSend([&messages](int32_t, const void* buffer, const uint64_t size, int32_t) -> int64_t {
		const auto* bytes{ static_cast<const uint8_t*>(buffer) };
		if (size < 2 || bytes[0] != 0x81 || (bytes[1] & 0x80) != 0) {
			return -1;
		}

		uint64_t length{ static_cast<uint64_t>(bytes[1] & 0x7f) };
		uint64_t offset{ 2 };
		if (length >= 126) {
			const uint64_t extra{ length == 126 ? uint64_t{ 2 } : uint64_t{ 8 } };
			if (size < offset + extra) {
				return -1;
			}

			length = 0;
			for (uint64_t i{}; i < extra; ++i) {
				length = (length << 8) | bytes[offset++];
			}
		}

		if (size != offset + length) {
			return -1;
		}

		messages.emplace_back(reinterpret_cast<const char*>(bytes + offset), length);
		return static_cast<int64_t>(size);
	});

	return std::make_shared<MSAPI::Connection::Data>(std::move(connection), MSAPI::SString<16>{},
		/*ip=*/0, /*port=*/0, MSAPI::Connection::Type::Income, /*doReconnection=*/false);
}

FORCE_INLINE [[nodiscard]] bool WebSocketEvents()
{
	namespace E = MSAPI::Protocol::WebSocket::Events;
	using DistributorT = E::StreamsDistributor<EventsAuthorization>;

	static_assert(U(E::Type::Undefined) == 0);
	static_assert(E::EnumToString(E::Type::Undefined) == "Undefined");
	static_assert(U(E::Type::Single) == 1);
	static_assert(E::EnumToString(E::Type::Single) == "Single");
	static_assert(U(E::Type::Stream) == 2);
	static_assert(E::EnumToString(E::Type::Stream) == "Stream");
	static_assert(U(E::Type::Max) == 3);
	static_assert(E::EnumToString(E::Type::Max) == "Max");

	static_assert(U(E::HandleResult::Undefined) == 0);
	static_assert(E::EnumToString(E::HandleResult::Undefined) == "Undefined");
	static_assert(U(E::HandleResult::Success) == 1);
	static_assert(E::EnumToString(E::HandleResult::Success) == "Success");
	static_assert(U(E::HandleResult::Fail) == 2);
	static_assert(E::EnumToString(E::HandleResult::Fail) == "Fail");
	static_assert(U(E::HandleResult::Delay) == 3);
	static_assert(E::EnumToString(E::HandleResult::Delay) == "Delay");
	static_assert(U(E::HandleResult::SuccessChunked) == 4);
	static_assert(E::EnumToString(E::HandleResult::SuccessChunked) == "SuccessChunked");
	static_assert(U(E::HandleResult::IOError) == 5);
	static_assert(E::EnumToString(E::HandleResult::IOError) == "IOError");
	static_assert(U(E::HandleResult::Max) == 6);
	static_assert(E::EnumToString(E::HandleResult::Max) == "Max");

	static_assert(U(E::TerminationReason::Undefined) == 0);
	static_assert(E::EnumToString(E::TerminationReason::Undefined) == "Undefined");
	static_assert(U(E::TerminationReason::ClientInterrupted) == 1);
	static_assert(E::EnumToString(E::TerminationReason::ClientInterrupted) == "ClientInterrupted");
	static_assert(U(E::TerminationReason::ConnectionClosed) == 2);
	static_assert(E::EnumToString(E::TerminationReason::ConnectionClosed) == "ConnectionClosed");
	static_assert(U(E::TerminationReason::Cleanup) == 3);
	static_assert(E::EnumToString(E::TerminationReason::Cleanup) == "Cleanup");
	static_assert(U(E::TerminationReason::LimitExceeded) == 4);
	static_assert(E::EnumToString(E::TerminationReason::LimitExceeded) == "LimitExceeded");
	static_assert(U(E::TerminationReason::RemovedByFilter) == 5);
	static_assert(E::EnumToString(E::TerminationReason::RemovedByFilter) == "RemovedByFilter");
	static_assert(U(E::TerminationReason::Failed) == 6);
	static_assert(E::EnumToString(E::TerminationReason::Failed) == "Failed");
	static_assert(U(E::TerminationReason::Completed) == 7);
	static_assert(E::EnumToString(E::TerminationReason::Completed) == "Completed");
	static_assert(U(E::TerminationReason::Max) == 8);
	static_assert(E::EnumToString(E::TerminationReason::Max) == "Max");

	static_assert(U(E::SendResult::Undefined) == 0);
	static_assert(E::EnumToString(E::SendResult::Undefined) == "Undefined");
	static_assert(U(E::SendResult::Success) == 1);
	static_assert(E::EnumToString(E::SendResult::Success) == "Success");
	static_assert(U(E::SendResult::Fail) == 2);
	static_assert(E::EnumToString(E::SendResult::Fail) == "Fail");
	static_assert(U(E::SendResult::Nothing) == 3);
	static_assert(E::EnumToString(E::SendResult::Nothing) == "Nothing");
	static_assert(U(E::SendResult::IOError) == 4);
	static_assert(E::EnumToString(E::SendResult::IOError) == "IOError");
	static_assert(U(E::SendResult::Max) == 5);
	static_assert(E::EnumToString(E::SendResult::Max) == "Max");

	static_assert(U(E::Stream::State::Undefined) == 0);
	static_assert(E::Stream::EnumToString(E::Stream::State::Undefined) == "Undefined");
	static_assert(U(E::Stream::State::Pending) == 1);
	static_assert(E::Stream::EnumToString(E::Stream::State::Pending) == "Pending");
	static_assert(U(E::Stream::State::Opened) == 2);
	static_assert(E::Stream::EnumToString(E::Stream::State::Opened) == "Opened");
	static_assert(U(E::Stream::State::Done) == 3);
	static_assert(E::Stream::EnumToString(E::Stream::State::Done) == "Done");
	static_assert(U(E::Stream::State::Failed) == 4);
	static_assert(E::Stream::EnumToString(E::Stream::State::Failed) == "Failed");
	static_assert(U(E::Stream::State::Closed) == 5);
	static_assert(E::Stream::EnumToString(E::Stream::State::Closed) == "Closed");
	static_assert(U(E::Stream::State::Removed) == 6);
	static_assert(E::Stream::EnumToString(E::Stream::State::Removed) == "Removed");
	static_assert(U(E::Stream::State::Max) == 7);
	static_assert(E::Stream::EnumToString(E::Stream::State::Max) == "Max");

	LOG_INFO("MSAPI UNIT TEST WebSocket events");
	MSAPI::Test::Test t;

	// Unknown values below Undefined and above Max exercise the logging fallback at runtime.
	for (const int8_t unknown : { int8_t{ -1 }, int8_t{ 127 } }) {
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::Type>(unknown)), std::string_view{ "Unknown" },
			"Unknown event type has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::HandleResult>(unknown)), std::string_view{ "Unknown" },
			"Unknown handle result has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::TerminationReason>(unknown)),
			std::string_view{ "Unknown" }, "Unknown termination reason has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::SendResult>(unknown)), std::string_view{ "Unknown" },
			"Unknown send result has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::Stream::EnumToString(static_cast<E::Stream::State>(unknown)),
			std::string_view{ "Unknown" }, "Unknown stream state has a fallback string"));
	}

	EventsAuthorization authorization;
	std::vector<std::string> messages;
	std::vector<std::string> otherMessages;
	const auto connection{ MakeEventsConnection(messages) };
	const auto otherConnection{ MakeEventsConnection(otherMessages) };

	RETURN_IF_FALSE(t.Assert(connection != nullptr && otherConnection != nullptr, true, "Connections are created"));

	// Matches event hash 1 and the default identity across connections.
	const DistributorT::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

	// Existing handlers keep the prefixed output contract and register before ordinary broadcast updates.
	{
		DistributorT distributor{ authorization };
		bool isOpenedBeforeCallback{};
		distributor.SetHandlerWithoutPermissions(1,
			[&messages, &isOpenedBeforeCallback](std::string& out, const E::Stream&) {
				isOpenedBeforeCallback = messages == std::vector<std::string>{ R"({"uids":[1],"state":2})" };
				out += "[1,2]";
				return E::HandleResult::Success;
			},
			/*terminationHandler=*/{});

		RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(t.Assert(isOpenedBeforeCallback, true, "Opened precedes the synchronous callback"));
		RETURN_IF_FALSE(t.Assert(messages
				== std::vector<std::string>{ R"({"uids":[1],"state":2})", R"({"uids":[1],"data":[1,2]})",
					R"({"uids":[1],"state":3})" },
			true, "Regular snapshot wire sequence"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "3" }), E::SendResult::Success,
			"Existing broadcast succeeds"));
		RETURN_IF_FALSE(t.Assert(messages.back(), std::string{ R"({"uids":[1],"data":3})" }, "Broadcast wire format"));
	}
	messages.clear();

	// Two existing recipients, including one on the same connection, must not receive another stream's chunks.
	{
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithPermissions(
			1,
			[](std::string& out, const E::Stream& stream) {
				if (stream.GetId() != 3) {
					out += "0";
					return E::HandleResult::Success;
				}

				if (!stream.SendData("[10]") || !stream.SendData("[20]")) {
					return E::HandleResult::IOError;
				}

				out = "UNUSED BUFFER MUST NOT BE SENT";
				return E::HandleResult::SuccessChunked;
			},
			/*terminationHandler=*/{}, EventsAuthorization::Grade::Reader);

		RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/2, /*hash=*/1, otherConnection, MSAPI::Json{ "{}" }));
		messages.clear();
		otherMessages.clear();
		RETURN_IF_FALSE(distributor.Collect(/*id=*/3, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(t.Assert(messages
				== std::vector<std::string>{ R"({"uids":[3],"state":2})", R"({"uids":[3],"data":[10]})",
					R"({"uids":[3],"data":[20]})", R"({"uids":[3],"state":3})" },
			true, "Chunks are ordered, individually addressed, followed by Done, and omit out"));
		RETURN_IF_FALSE(t.Assert(otherMessages.empty(), true, "Other connection receives no initialization chunks"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "4" }), E::SendResult::Success,
			"Chunked success registers for broadcasts"));
		RETURN_IF_FALSE(t.Assert(messages.back(), std::string{ R"({"uids":[1,3],"data":4})" },
			"Both same-connection streams receive subsequent broadcasts"));
		RETURN_IF_FALSE(t.Assert(otherMessages.back(), std::string{ R"({"uids":[2],"data":4})" },
			"Other connection receives subsequent broadcasts"));
	}

	// A chunked callback may leave the original prefix untouched, or clear it, without affecting explicit mode.
	for (const bool shouldClearOutput : { false, true }) {
		messages.clear();
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(1,
			[shouldClearOutput](std::string& out, const E::Stream&) {
				if (shouldClearOutput) {
					out.clear();
				}

				return E::HandleResult::SuccessChunked;
			},
			/*terminationHandler=*/{});

		RETURN_IF_FALSE(distributor.Collect(/*id=*/4, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(
			t.Assert(messages == std::vector<std::string>{ R"({"uids":[4],"state":2})", R"({"uids":[4],"state":3})" },
				true, "Zero chunks skip both prefixed and empty output buffers"));
	}

	// Failed initialization never becomes a subscription or triggers a termination callback.
	for (const bool hasPartialData : { false, true }) {
		messages.clear();
		uint64_t terminated{};
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1,
			[hasPartialData](std::string& out, const E::Stream& stream) {
				if (hasPartialData && !stream.SendData("42")) {
					return E::HandleResult::IOError;
				}

				out = "snapshot failure";
				return E::HandleResult::Fail;
			},
			[&terminated](const E::Stream&, E::TerminationReason) { ++terminated; });

		RETURN_IF_FALSE(distributor.Collect(/*id=*/5, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		std::vector<std::string> expected{ R"({"uids":[5],"state":2})" };
		if (hasPartialData) {
			expected.emplace_back(R"({"uids":[5],"data":42})");
		}
		expected.emplace_back(R"({"uids":[5],"state":4,"error":"snapshot failure"})");
		RETURN_IF_FALSE(
			t.Assert(messages == expected, true, "Failure preserves any delivered chunks and sends Failed"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "0" }), E::SendResult::Nothing,
			"Failed initialization is not registered"));
		RETURN_IF_FALSE(distributor.FailActiveEvents("cleanup"));
		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{}, "Initialization failure does not notify termination"));
	}

	// Every removal path permits callbacks to reacquire connection, filter and event registry locks.
	for (const auto reason : { E::TerminationReason::ClientInterrupted, E::TerminationReason::ConnectionClosed,
			 E::TerminationReason::Cleanup, E::TerminationReason::RemovedByFilter, E::TerminationReason::Failed }) {

		messages.clear();
		uint64_t count{};
		bool isContextCorrect{};
		bool areLocksReleased{};
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1,
			[](std::string& out, const E::Stream&) {
				out += "null";
				return E::HandleResult::Success;
			},
			[&distributor, &count, &isContextCorrect, &areLocksReleased, &connection, filterHash1, reason](
				const E::Stream& stream, const E::TerminationReason actual) {
				if (stream.GetId() != 7) {
					return;
				}

				++count;
				isContextCorrect = actual == reason && stream.GetId() == 7 && stream.GetConnectionData() == connection
					&& stream.GetJson().GetValueType<uint64_t>("context") != nullptr;
				// Reenter lookup, registration and removal through the public API after the original entry is removed.
				areLocksReleased = distributor.SendData(filterHash1, std::string_view{ "0" }) == E::SendResult::Nothing;
				const auto isRegistered{ distributor.Collect(/*id=*/8, /*hash=*/1, connection, MSAPI::Json{ "{}" }) };
				const auto isInterrupted{ distributor.Collect(
					/*id=*/8, /*hash=*/1, connection, MSAPI::Json{ R"({"interrupt":true})" }) };
				areLocksReleased = areLocksReleased && isRegistered && isInterrupted;
			});

		RETURN_IF_FALSE(distributor.Collect(/*id=*/7, /*hash=*/1, connection, MSAPI::Json{ R"({"context":17})" }));
		switch (reason) {
		case E::TerminationReason::ClientInterrupted:
			RETURN_IF_FALSE(
				distributor.Collect(/*id=*/7, /*hash=*/1, connection, MSAPI::Json{ R"({"interrupt":true})" }));
			break;
		case E::TerminationReason::ConnectionClosed:
			distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
			break;
		case E::TerminationReason::Cleanup:
			RETURN_IF_FALSE(distributor.FailActiveEvents("shutdown"));
			break;
		case E::TerminationReason::RemovedByFilter:
			distributor.EraseEventsOnConnectionsByFilter(filterHash1);
			break;
		case E::TerminationReason::Failed:
			RETURN_IF_FALSE(distributor.FailEventsOnConnectionsByFilter(filterHash1, "administrative failure"));
			break;
		default:
			return false;
		}
		distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
		RETURN_IF_FALSE(t.Assert(count, uint64_t{ 1 }, "Only the remover invokes termination"));
		RETURN_IF_FALSE(t.Assert(isContextCorrect, true, "Callback identifies stream, request, connection and reason"));
		RETURN_IF_FALSE(t.Assert(areLocksReleased, true, "Callback reacquires every registry lock after removal"));
	}

	// Insertion-triggered purge and limit reduction both notify without retaining the insertion lock.
	{
		messages.clear();
		uint64_t purged{};
		bool isReasonCorrect{ true };
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1,
			[](std::string& out, const E::Stream&) {
				out += "0";
				return E::HandleResult::Success;
			},
			[&distributor, &purged, &isReasonCorrect, filterHash1](
				const E::Stream&, const E::TerminationReason reason) {
				++purged;
				isReasonCorrect = isReasonCorrect && reason == E::TerminationReason::LimitExceeded;
				if (reason == E::TerminationReason::LimitExceeded) {
					isReasonCorrect = isReasonCorrect
						&& distributor.SendData(filterHash1, std::string_view{ "0" }) == E::SendResult::Success;
				}
			});

		RETURN_IF_FALSE(
			t.Assert(distributor.SetEventsLimit(connection->GetConnectionId(), 64), true, "Set minimum limit"));
		RETURN_IF_FALSE(distributor.SetEventsPurgingCoefficient(connection->GetConnectionId(), 0.4f));
		RETURN_IF_FALSE(distributor.SetEventsPurgingCoefficient(connection->GetConnectionId(), 0.3f));
		for (uint64_t id{ 1 }; id <= 65; ++id) {
			RETURN_IF_FALSE(distributor.Collect(/*id=*/id, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		// Purged: (65 - 64) + floor(64 * 0.3) = 1 + 19 = 20 events.
		RETURN_IF_FALSE(t.Assert(purged, uint64_t{ 20 }, "Overflow purges the oldest bunch"));
		RETURN_IF_FALSE(t.Assert(isReasonCorrect, true, "Overflow reports LimitExceeded"));
		// Remove the remainder before its destructor: its cleanup reason intentionally differs.
		distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
	}

	// Removed filter and connection handles reject stale registration attempts.
	{
		const auto data{ std::make_shared<DistributorT::EventsData>(connection->GetConnectionId()) };
		const auto events{ data->template GetEvents<DistributorT::EventsData::CREATE>(filterHash1) };
		data->EraseEvents(E::TerminationReason::ConnectionClosed);

		const auto handler{ std::make_shared<E::Stream::HandlerData>(
			[](std::string&, const E::Stream&) { return E::HandleResult::SuccessChunked; },
			E::Stream::TerminationHandlerT{}) };
		auto stream{ std::make_shared<E::Stream>(
			1, std::shared_ptr<E::Stream::HandlerData>{ handler }, MSAPI::Json{ "{}" }, connection) };

		RETURN_IF_FALSE(t.Assert(events->AddEvent(stream), false, "Removed filter rejects registration"));
		// Limit maintenance cannot reactivate released standalone storage.
		RETURN_IF_FALSE(data->SetLimit(64));
		RETURN_IF_FALSE(t.Assert(data->template GetEvents<DistributorT::EventsData::CREATE>(filterHash1) == nullptr,
			true, "Removed connection data rejects new filters"));
	}

	// Reducing limits also purges by age and preserves the existing bunch policy.
	{
		uint64_t purged{};
		bool isOldestFirst{ true };
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Stream&) { return E::HandleResult::SuccessChunked; },
			[&purged, &isOldestFirst](const E::Stream& stream, const E::TerminationReason reason) {
				if (reason == E::TerminationReason::LimitExceeded) {
					isOldestFirst = isOldestFirst && stream.GetId() == ++purged;
				}
			});

		RETURN_IF_FALSE(
			t.Assert(distributor.SetEventsLimit(connection->GetConnectionId(), 128), true, "Set initial larger limit"));
		for (uint64_t id{ 1 }; id <= 100; ++id) {
			RETURN_IF_FALSE(distributor.Collect(/*id=*/id, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		RETURN_IF_FALSE(
			t.Assert(distributor.SetEventsLimit(connection->GetConnectionId(), 64), true, "Reduce subscription limit"));

		// Purged: (100 - 64) + floor(64 * 0.3) = 36 + 19 = 55 events.
		RETURN_IF_FALSE(t.Assert(purged, uint64_t{ 55 }, "Limit reduction purges exceeding size plus bunch"));
		RETURN_IF_FALSE(t.Assert(isOldestFirst, true, "Oldest streams are purged first"));
		distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
	}

	// Reentrant configuration changes and registration defer enforcement until the next incoming event.
	{
		messages.clear();
		uint64_t purged{};
		uint64_t completed{};
		bool isLimitChanged{};
		bool isCoefficientChanged{};
		bool isCallbackRegistrationSuccessful{};
		bool areEventsCorrect{ true };
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&singles, &connection, &purged, &completed, &isLimitChanged, &isCoefficientChanged,
				&isCallbackRegistrationSuccessful,
				&areEventsCorrect](const E::Single& event, const E::TerminationReason reason) {
				if (reason == E::TerminationReason::LimitExceeded) {
					++purged;
					areEventsCorrect = areEventsCorrect && event.GetId() == purged;
					if (event.GetId() == 1) {
						isLimitChanged = singles.SetEventsLimit(connection->GetConnectionId(), 64);
						isCoefficientChanged = singles.SetEventsPurgingCoefficient(connection->GetConnectionId(), 0.5f);
						isCallbackRegistrationSuccessful
							= singles.Collect(/*id=*/130, /*hash=*/1, connection, MSAPI::Json{ "{}" });
					}
					return;
				}

				if (reason == E::TerminationReason::Completed) {
					++completed;
					areEventsCorrect = areEventsCorrect && event.GetId() == 99 + completed;
					return;
				}

				areEventsCorrect = false;
			});

		RETURN_IF_FALSE(singles.SetEventsLimit(connection->GetConnectionId(), 128));
		for (uint64_t id{ 1 }; id <= 129; ++id) {
			RETURN_IF_FALSE(singles.Collect(/*id=*/id, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		RETURN_IF_FALSE(t.Assert(isLimitChanged, true, "A purge callback can reduce the connection limit"));
		RETURN_IF_FALSE(t.Assert(isCoefficientChanged, true, "A purge callback can change the purging coefficient"));
		RETURN_IF_FALSE(t.Assert(
			isCallbackRegistrationSuccessful, true, "A purge callback can register without waiting for its own purge"));

		// Original batch: (129 - 128) + floor(128 * 0.3) = 1 + 38 = 39 removals, fixed before callbacks run.
		RETURN_IF_FALSE(t.Assert(purged, uint64_t{ 39 }, "Reentrant checks do not extend the active purge batch"));
		RETURN_IF_FALSE(t.Assert(messages.empty(), true, "Purging delayed events does not send data responses"));
		RETURN_IF_FALSE(singles.Collect(/*id=*/131, /*hash=*/1, connection, MSAPI::Json{ "{}" }));

		// Pending: 129 + 1 callback registration + 1 incoming event - 39 purged = 92 events.
		// Next purge: (92 - 64) + floor(64 * 0.5) = 60; total purged: 39 + 60 = 99 events.
		RETURN_IF_FALSE(
			t.Assert(purged, uint64_t{ 99 }, "The next incoming event enforces the changed purge settings"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "0" }), E::SendResult::Success,
			"Events retained after deferred purging remain deliverable"));

		// Remaining: 131 registrations - 99 purged = 32 completed events.
		RETURN_IF_FALSE(t.Assert(completed, uint64_t{ 32 }, "Only the retained delayed events complete"));
		RETURN_IF_FALSE(t.Assert(areEventsCorrect, true, "Purging preserves age order and finalizes each event once"));
		RETURN_IF_FALSE(t.Assert(messages.size(), uint64_t{ 1 }, "Retained events share one response frame"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "1" }), E::SendResult::Nothing,
			"Deferred purging and completion leave no events registered"));
	}

	// A contending receive operation and limit reduction return while another thread owns the purge.
	{
		messages.clear();
		std::atomic<uint64_t> purged{};
		std::atomic<uint64_t> completed{};
		std::atomic<bool> areEventsCorrect{ true };
		std::barrier callbackEntered{ 2 };
		std::barrier finishCallback{ 2 };
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&purged, &completed, &areEventsCorrect, &callbackEntered, &finishCallback](
				const E::Single& event, const E::TerminationReason reason) {
				if (reason == E::TerminationReason::LimitExceeded) {
					if (event.GetId() != purged.fetch_add(1) + 1) {
						areEventsCorrect.store(false);
					}
					if (event.GetId() == 1) {
						callbackEntered.arrive_and_wait();
						finishCallback.arrive_and_wait();
					}
					return;
				}

				if (reason == E::TerminationReason::Completed) {
					if (event.GetId() != 87 + completed.fetch_add(1)) {
						areEventsCorrect.store(false);
					}
					return;
				}

				areEventsCorrect.store(false);
			});

		RETURN_IF_FALSE(singles.SetEventsLimit(connection->GetConnectionId(), 128));
		for (uint64_t id{ 1 }; id <= 128; ++id) {
			RETURN_IF_FALSE(singles.Collect(/*id=*/id, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		bool isOwnerRegistrationSuccessful{};
		std::thread owner{ [&singles, &connection, &isOwnerRegistrationSuccessful] {
			isOwnerRegistrationSuccessful = singles.Collect(/*id=*/129, /*hash=*/1, connection, MSAPI::Json{ "{}" });
		} };

		callbackEntered.arrive_and_wait();
		const auto isContendingRegistrationSuccessful{ singles.Collect(
			/*id=*/130, /*hash=*/1, connection, MSAPI::Json{ "{}" }) };
		const auto isLimitChanged{ singles.SetEventsLimit(connection->GetConnectionId(), 64) };
		const auto notificationsWhileBlocked{ purged.load() };
		// Release and join the owner before any assertion can return from the test.
		finishCallback.arrive_and_wait();
		owner.join();
		RETURN_IF_FALSE(
			t.Assert(isOwnerRegistrationSuccessful, true, "The original receive operation finishes purging"));
		RETURN_IF_FALSE(t.Assert(
			isContendingRegistrationSuccessful, true, "Another thread registers while a purge callback is blocked"));
		RETURN_IF_FALSE(t.Assert(isLimitChanged, true, "Another thread reduces the limit without waiting for purging"));
		RETURN_IF_FALSE(t.Assert(
			notificationsWhileBlocked, uint64_t{ 1 }, "Contending checks do not run a second purge concurrently"));

		// Original batch: (129 - 128) + floor(128 * 0.3) = 1 + 38 = 39 removals.
		RETURN_IF_FALSE(
			t.Assert(purged.load(), uint64_t{ 39 }, "Only the original purge batch runs before new traffic"));
		RETURN_IF_FALSE(singles.Collect(/*id=*/131, /*hash=*/1, connection, MSAPI::Json{ "{}" }));

		// Pending: 128 initial + 1 owner + 1 contender + 1 incoming - 39 purged = 92 events.
		// Next purge: (92 - 64) + floor(64 * 0.3) = 47; total purged: 39 + 47 = 86 events.
		RETURN_IF_FALSE(
			t.Assert(purged.load(), uint64_t{ 86 }, "Later traffic retries the deferred limit enforcement"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "0" }), E::SendResult::Success,
			"Remaining events complete after contending purge checks"));

		// Remaining: 131 registrations - 86 purged = 45 completed events.
		RETURN_IF_FALSE(t.Assert(completed.load(), uint64_t{ 45 }, "The changed limit retains the expected batch"));
		RETURN_IF_FALSE(
			t.Assert(areEventsCorrect.load(), true, "Competing purge checks preserve event identity and reason"));
	}

	// Removal after broadcast selection does not cancel the already selected outgoing message.
	{
		messages.clear();
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(1,
			[](std::string&, const E::Stream&) { return E::HandleResult::SuccessChunked; }, /*terminationHandler=*/{});
		RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		const auto result{ distributor.SendData(filterHash1, [&distributor, filterHash1](std::string& out) {
			distributor.EraseEventsOnConnectionsByFilter(filterHash1);
			out = "5";
			return true;
		}) };
		RETURN_IF_FALSE(t.Assert(result, E::SendResult::Success, "Selected message survives concurrent removal"));
		RETURN_IF_FALSE(
			t.Assert(messages.back(), std::string{ R"({"uids":[1],"data":5})" }, "Recipient snapshot is retained"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "6" }), E::SendResult::Nothing,
			"Later broadcasts exclude the removed stream"));
	}

	// Single events notify only when a registered delayed request is removed; callback failures stay contained.
	{
		messages.clear();
		uint64_t terminated{};
		bool isContextCorrect{};
		bool areLocksReleased{};
		uint64_t failedTerminations{};
		E::TerminationReason failureReason{};
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		const E::SinglesDistributor<EventsAuthorization>::FilterT delayedFilter{ 3, E::IdentityFilter{} };

		const auto onTermination{ [&singles, &terminated, &isContextCorrect, &areLocksReleased, &connection,
									  delayedFilter](const E::Single& single, const E::TerminationReason reason) {
			if (single.GetId() != 3) {
				return;
			}

			++terminated;
			isContextCorrect = single.GetId() == 3 && single.GetConnectionData() == connection
				&& single.GetJson().GetValueType<uint64_t>("marker") != nullptr
				&& reason == E::TerminationReason::Completed;

			// Reenter recipient lookup for the same filter; no completed event remains.
			areLocksReleased = singles.CheckDelayed(delayedFilter, std::string_view{ "0" }) == E::SendResult::Nothing;
			const auto isRegistered{ singles.Collect(/*id=*/6, /*hash=*/3, connection, MSAPI::Json{ "{}" }) };
			const auto isInterrupted{ singles.Collect(
				/*id=*/6, /*hash=*/3, connection, MSAPI::Json{ R"({"interrupt":true})" }) };
			areLocksReleased = areLocksReleased && isRegistered && isInterrupted;

			throw std::runtime_error{ "single event termination callback" };
		} };

		singles.SetHandlerWithoutPermissions(
			1,
			[](std::string& out, const E::Single&) {
				out += "7";
				return E::HandleResult::Success;
			},
			onTermination);
		singles.SetHandlerWithoutPermissions(
			2,
			[](std::string& out, const E::Single&) {
				out = "single failure";
				return E::HandleResult::Fail;
			},
			onTermination);
		singles.SetHandlerWithoutPermissions(
			3, [](std::string&, const E::Single&) { return E::HandleResult::Delay; }, onTermination);

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(singles.Collect(/*id=*/2, /*hash=*/2, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(singles.Collect(/*id=*/3, /*hash=*/3, connection, MSAPI::Json{ R"({"marker":17})" }));

		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{}, "Immediate single responses do not notify termination"));
		RETURN_IF_FALSE((singles.CheckDelayed(delayedFilter, std::string_view{ "8" }) == E::SendResult::Success));
		RETURN_IF_FALSE(t.Assert(messages
				== std::vector<std::string>{ R"({"uids":[1],"data":7})",
					R"({"uids":[2],"state":4,"error":"single failure"})", R"({"uids":[3],"data":8})" },
			true, "Single-event wire formats and delayed response remain unchanged"));
		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{ 1 }, "Delayed single removal notifies its handler"));
		RETURN_IF_FALSE(
			t.Assert(isContextCorrect, true, "Single termination supplies the request, connection and reason"));
		RETURN_IF_FALSE(t.Assert(areLocksReleased, true, "Single termination callbacks run outside registry locks"));
		RETURN_IF_FALSE((singles.CheckDelayed(delayedFilter, std::string_view{ "9" }) == E::SendResult::Nothing));
		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{ 1 }, "A completed single event is not notified again"));

		singles.SetHandlerWithoutPermissions(
			4, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&failedTerminations, &failureReason](const E::Single&, const E::TerminationReason reason) {
				++failedTerminations;
				failureReason = reason;
			});
		RETURN_IF_FALSE(singles.Collect(/*id=*/4, /*hash=*/4, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE((singles.CheckDelayed({ 4, E::IdentityFilter{} }, [](std::string& error) {
			error = "deferred response failed";
			return false;
		}) == E::SendResult::Fail));
		RETURN_IF_FALSE(t.Assert(failedTerminations, uint64_t{ 1 }, "Failed delayed response removes its event"));
		RETURN_IF_FALSE(t.Assert(failureReason, E::TerminationReason::Failed, "Failed response is not completed"));
		RETURN_IF_FALSE(
			t.Assert(messages.back(), std::string{ R"({"uids":[4],"state":4,"error":"deferred response failed"})" },
				"Failed delayed response preserves the failure wire format"));
	}

	// An additional derived event uses the same notification path without specializing Event.
	{
		/**************************
		 * @brief Exercises termination for an event type outside the built-in single and stream types.
		 *
		 * @concurrency No.
		 */
		class AdditionalEvent : public E::Event<AdditionalEvent> {
		public:
			using BaseT = E::Event<AdditionalEvent>;

		public:
			using BaseT::BaseT;
			AdditionalEvent(const AdditionalEvent&) = delete;
			AdditionalEvent(AdditionalEvent&&) = delete;
			AdditionalEvent& operator=(const AdditionalEvent&) = delete;
			AdditionalEvent& operator=(AdditionalEvent&&) = delete;
		};

		/**************************
		 * @brief Supplies registry storage for the additional event type without a wire collection handler.
		 *
		 * @concurrency Yes.
		 */
		class AdditionalDistributor
			: public E::Distributor<EventsAuthorization, AdditionalEvent, E::IdentityFilter, AdditionalDistributor> {
		public:
			using BaseT
				= E::Distributor<EventsAuthorization, AdditionalEvent, E::IdentityFilter, AdditionalDistributor>;

		public:
			using BaseT::BaseT;
			AdditionalDistributor(const AdditionalDistributor&) = delete;
			AdditionalDistributor(AdditionalDistributor&&) = delete;
			AdditionalDistributor& operator=(const AdditionalDistributor&) = delete;
			AdditionalDistributor& operator=(AdditionalDistributor&&) = delete;

			/**************************
			 * @brief Initializes and registers the additional event through the distributor extension interface.
			 *
			 * @param filter Event filter.
			 * @param event Event to initialize and register.
			 *
			 * @locking Uses the base registration locking contract.
			 *
			 * @return True if initialized and registered.
			 *
			 * @test Yes.
			 */
			FORCE_INLINE [[nodiscard]] bool Register(
				const BaseT::FilterT& filter, const std::shared_ptr<AdditionalEvent>& event)
			{
				std::string payload;
				return BaseT::HandleEvent(*event, payload) == E::HandleResult::Success
					&& BaseT::RegisterEvent(filter, event);
			}
		};

		uint64_t terminated{};
		bool isContextCorrect{};
		AdditionalDistributor distributor{ authorization };
		auto handlerData{ std::make_shared<AdditionalEvent::HandlerData>(
			[](std::string&, const AdditionalEvent&) { return E::HandleResult::Success; },
			[&terminated, &isContextCorrect, &connection](
				const AdditionalEvent& event, const E::TerminationReason reason) {
				++terminated;
				isContextCorrect = event.GetId() == 99 && event.GetConnectionData() == connection
					&& reason == E::TerminationReason::ConnectionClosed;
			}) };

		auto event{ std::make_shared<AdditionalEvent>(99, std::move(handlerData), MSAPI::Json{ "{}" }, connection) };
		RETURN_IF_FALSE(
			t.Assert(distributor.Register(filterHash1, event), true, "Additional derived event is registered"));
		distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
		distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{ 1 }, "Additional event receives one termination notification"));
		RETURN_IF_FALSE(
			t.Assert(isContextCorrect, true, "Additional event receives its concrete type and removal reason"));
	}

	// Competing removers start together. Callback counters need no registry locks and fire once per stream.
	{
		messages.clear();
		std::atomic<uint64_t> count{};
		std::atomic<bool> isReasonValid{ true };
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1,
			[](std::string& out, const E::Stream&) {
				out += "0";
				return E::HandleResult::Success;
			},
			[&count, &isReasonValid](const E::Stream&, const E::TerminationReason reason) {
				count.fetch_add(1);
				if (reason != E::TerminationReason::ConnectionClosed
					&& reason != E::TerminationReason::ClientInterrupted
					&& reason != E::TerminationReason::RemovedByFilter) {
					isReasonValid.store(false);
				}
			});

		for (uint64_t round{}; round < 40; ++round) {
			RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
			std::barrier start{ 4 };

			std::thread interrupt{ [&distributor, &connection, &start] {
				start.arrive_and_wait();
				(void)distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ R"({"interrupt":true})" });
			} };
			std::thread clear{ [&distributor, &connection, &start] {
				start.arrive_and_wait();
				distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
			} };
			std::thread erase{ [&distributor, &start, filterHash1] {
				start.arrive_and_wait();
				distributor.EraseEventsOnConnectionsByFilter(filterHash1);
			} };

			start.arrive_and_wait();
			interrupt.join();
			clear.join();
			erase.join();
		}

		// Notifications: 40 rounds * 1 registered stream per round = 40, regardless of the three competing removers.
		RETURN_IF_FALSE(t.Assert(count.load(), uint64_t{ 40 }, "Racing removers notify each stream exactly once"));
		RETURN_IF_FALSE(t.Assert(isReasonValid.load(), true, "Racing winner supplies its actual reason"));
	}

	// Exceptions from one callback do not interrupt cleanup of other entries.
	{
		uint64_t count{};
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Stream&) { return E::HandleResult::SuccessChunked; },
			[&count](const E::Stream&, E::TerminationReason) {
				++count;
				throw std::runtime_error{ "application callback" };
			});

		RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/2, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.FailActiveEvents("cleanup"));
		RETURN_IF_FALSE(t.Assert(count, uint64_t{ 2 }, "Throwing callbacks are contained and cleanup continues"));
	}

	// Destruction notifies remaining registered streams without sending protocol messages.
	// Callback captures outlive the distributor.
	{
		uint64_t count{};
		bool isReasonCorrect{};
		const auto size{ messages.size() };
		{
			DistributorT distributor{ authorization };
			distributor.SetHandlerWithoutPermissions(
				1, [](std::string&, const E::Stream&) { return E::HandleResult::SuccessChunked; },
				[&count, &isReasonCorrect](const E::Stream&, const E::TerminationReason reason) {
					++count;
					isReasonCorrect = reason == E::TerminationReason::Cleanup;
				});

			RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		RETURN_IF_FALSE(t.Assert(count, uint64_t{ 1 }, "Destruction notifies registered streams"));
		RETURN_IF_FALSE(t.Assert(isReasonCorrect, true, "Destruction reports Cleanup"));

		// Added frames: 1 Opened + 1 Done = 2; destruction adds none.
		RETURN_IF_FALSE(t.Assert(messages.size(), size + 2, "Destruction sends no wire messages"));
	}

	// Transport failures stop initialization at Opened, the ordinary snapshot, or Done.
	for (const auto failAt : { uint64_t{ 1 }, uint64_t{ 2 }, uint64_t{ 3 } }) {
		std::vector<std::string> unused;
		const auto failing{ MakeEventsConnection(unused) };
		RETURN_IF_FALSE((failing != nullptr));
		uint64_t writes{};
		uint64_t handled{};
		uint64_t terminated{};
		failing->GetConnection().SetSend(
			[&writes, failAt](int32_t, const void*, const uint64_t size, int32_t) -> int64_t {
				if (++writes == failAt) {
					errno = EPIPE;
					return -1;
				}

				return static_cast<int64_t>(size);
			});

		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(
			1,
			[&handled](std::string& out, const MSAPI::Protocol::WebSocket::Events::Stream&) {
				++handled;
				out += '1';
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
			},
			[&terminated](const MSAPI::Protocol::WebSocket::Events::Stream&,
				const MSAPI::Protocol::WebSocket::Events::TerminationReason reason) {
				if (reason == MSAPI::Protocol::WebSocket::Events::TerminationReason::ConnectionClosed) {
					++terminated;
				}
			});

		RETURN_IF_FALSE(t.Assert(distributor.Collect(/*id=*/1, /*hash=*/1, failing, MSAPI::Json{ "{}" }), false,
			"Collection exposes transport failure"));
		RETURN_IF_FALSE(t.Assert(writes, failAt, "Initialization sends no later frames after failure"));
		RETURN_IF_FALSE(
			t.Assert(handled, uint64_t{ failAt == 1 ? 0u : 1u }, "Failed Opened skips snapshot production"));
		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{ failAt == 1 ? 0u : 1u },
			"Failed snapshot or Done removes already registered streams"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "0" }), E::SendResult::Nothing,
			"Initialization transport failure leaves no active streams on that connection"));
	}

	// Chunk producers observe the send result and stop before formatting more snapshot data.
	{
		std::vector<std::string> unused;
		const auto failing{ MakeEventsConnection(unused) };
		RETURN_IF_FALSE((failing != nullptr));
		uint64_t writes{};
		uint64_t produced{};
		failing->GetConnection().SetSend([&writes](int32_t, const void*, const uint64_t size, int32_t) -> int64_t {
			if (++writes == 3) {
				errno = EPIPE;
				return -1;
			}

			return static_cast<int64_t>(size);
		});

		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(1,
			[&produced](std::string&, const MSAPI::Protocol::WebSocket::Events::Stream& stream) {
				for (uint64_t chunk{}; chunk < 10; ++chunk) {
					++produced;
					if (!stream.SendData("1")) {
						return MSAPI::Protocol::WebSocket::Events::HandleResult::IOError;
					}
				}

				return MSAPI::Protocol::WebSocket::Events::HandleResult::SuccessChunked;
			},
			/*terminationHandler=*/{});

		RETURN_IF_FALSE(t.Assert(distributor.Collect(/*id=*/1, /*hash=*/1, failing, MSAPI::Json{ "{}" }), false,
			"Chunk transport failure reaches the collection caller"));

		// Produced chunks: 3 attempted writes - 1 Opened frame = 2, including the chunk whose send fails.
		RETURN_IF_FALSE(t.Assert(produced, uint64_t{ 2 }, "Chunk producer stops after its failed send"));

		// Writes: 1 Opened + 1 successful chunk + 1 failed chunk = 3.
		RETURN_IF_FALSE(t.Assert(writes, uint64_t{ 3 }, "Chunk failure sends neither Done nor another failure frame"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "0" }), E::SendResult::Nothing,
			"Failed chunk initialization does not register a stream"));
	}

	// Failure writes hold no event locks; failed connections still complete all cleanup and notifications.
	{
		std::vector<std::string> unused;
		std::vector<std::string> healthyMessages;
		const auto failing{ MakeEventsConnection(unused) };
		const auto healthy{ MakeEventsConnection(healthyMessages) };
		RETURN_IF_FALSE((failing != nullptr && healthy != nullptr));
		uint64_t writes{};
		uint64_t terminated{};
		bool isLockReleased{};
		DistributorT distributor{ authorization };
		const auto handler{ [](std::string&, const MSAPI::Protocol::WebSocket::Events::Stream&) {
			return MSAPI::Protocol::WebSocket::Events::HandleResult::SuccessChunked;
		} };
		const auto onTermination{ [&terminated](const MSAPI::Protocol::WebSocket::Events::Stream&,
									  const MSAPI::Protocol::WebSocket::Events::TerminationReason) { ++terminated; } };

		distributor.SetHandlerWithoutPermissions(1, handler, onTermination);
		distributor.SetHandlerWithoutPermissions(2, handler, onTermination);
		for (uint64_t id{ 1 }; id <= 5; ++id) {
			RETURN_IF_FALSE(distributor.Collect(/*id=*/id, /*hash=*/id <= 3 ? 1 : 2, failing, MSAPI::Json{ "{}" }));
		}

		RETURN_IF_FALSE(distributor.Collect(/*id=*/6, /*hash=*/1, healthy, MSAPI::Json{ "{}" }));
		healthyMessages.clear();
		failing->GetConnection().SetSend(
			[&writes, &isLockReleased, &distributor, filterHash1](int32_t, const void*, uint64_t, int32_t) -> int64_t {
				const auto result{ distributor.SendData(filterHash1, std::string_view{ "0" }) };
				isLockReleased = result == E::SendResult::Nothing || result == E::SendResult::Success;
				++writes;
				errno = EPIPE;
				return -1;
			});

		RETURN_IF_FALSE(t.Assert(distributor.FailEventsOnConnectionsByFilter(filterHash1, "cleanup"), false,
			"Filtered failure reports its transport error"));
		RETURN_IF_FALSE(
			t.Assert(distributor.FailActiveEvents("cleanup"), false, "Bulk failure reports its transport error"));
		RETURN_IF_FALSE(t.Assert(isLockReleased, true, "Failure I/O executes outside event write locks"));
		RETURN_IF_FALSE(t.Assert(writes, uint64_t{ 1 }, "Failed connection is written only once"));

		// Terminations: 5 events on the failed connection + 1 on the healthy connection = 6.
		RETURN_IF_FALSE(
			t.Assert(terminated, uint64_t{ 6 }, "Every removed event is notified despite transport failure"));
		RETURN_IF_FALSE(t.Assert(
			std::find(healthyMessages.begin(), healthyMessages.end(), R"({"uids":[6],"state":4,"error":"cleanup"})")
				!= healthyMessages.end(),
			true, "Other connections still receive failure frames"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "0" }), E::SendResult::Nothing,
			"Bulk failure removes all entries"));
	}

	// Broadcast reports partial transport failure while delivering to independent healthy connections.
	{
		std::vector<std::string> unused;
		std::vector<std::string> healthyMessages;
		const auto failing{ MakeEventsConnection(unused) };
		const auto healthy{ MakeEventsConnection(healthyMessages) };
		RETURN_IF_FALSE((failing != nullptr && healthy != nullptr));
		uint64_t terminated{};
		bool areReasonsCorrect{ true };
		DistributorT distributor{ authorization };
		const auto handler{ [](std::string&, const MSAPI::Protocol::WebSocket::Events::Stream&) {
			return MSAPI::Protocol::WebSocket::Events::HandleResult::SuccessChunked;
		} };
		const auto onTermination{ [&terminated, &areReasonsCorrect](
									  const MSAPI::Protocol::WebSocket::Events::Stream& stream,
									  const MSAPI::Protocol::WebSocket::Events::TerminationReason reason) {
			++terminated;
			areReasonsCorrect = areReasonsCorrect && (stream.GetId() == 1 || stream.GetId() == 3)
				&& reason == MSAPI::Protocol::WebSocket::Events::TerminationReason::ConnectionClosed;
		} };

		distributor.SetHandlerWithoutPermissions(1, handler, onTermination);
		distributor.SetHandlerWithoutPermissions(2, handler, onTermination);

		RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, failing, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/2, /*hash=*/1, healthy, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/3, /*hash=*/2, failing, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/4, /*hash=*/2, healthy, MSAPI::Json{ "{}" }));
		healthyMessages.clear();
		failing->GetConnection().SetSend([](int32_t, const void*, uint64_t, int32_t) -> int64_t {
			errno = EPIPE;
			return -1;
		});

		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "7" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::IOError, "Broadcast exposes transport failure"));
		RETURN_IF_FALSE(t.Assert(healthyMessages == std::vector<std::string>{ R"({"uids":[2],"data":7})" }, true,
			"Healthy connections receive data without a failure notification"));

		// Failed-connection terminations: 1 event on hash 1 + 1 event on hash 2 = 2.
		RETURN_IF_FALSE(
			t.Assert(terminated, uint64_t{ 2 }, "SendData removes every filter on only the failed connection"));
		RETURN_IF_FALSE(t.Assert(areReasonsCorrect, true, "Failed connection cleanup reports ConnectionClosed"));
		distributor.ClearActiveEventsForConnectionId(failing->GetConnectionId());
		RETURN_IF_FALSE(
			t.Assert(terminated, uint64_t{ 2 }, "Connection teardown does not notify removed streams twice"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, std::string_view{ "8" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::Success, "Healthy matching streams remain registered"));
		RETURN_IF_FALSE(t.Assert(
			distributor.SendData({ 2, MSAPI::Protocol::WebSocket::Events::IdentityFilter{} }, std::string_view{ "9" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::Success,
			"Healthy streams on other filters remain registered"));
		RETURN_IF_FALSE(t.Assert(healthyMessages
				== std::vector<std::string>{ R"({"uids":[2],"data":7})", R"({"uids":[2],"data":8})",
					R"({"uids":[4],"data":9})" },
			true, "Later broadcasts reach only the healthy connection"));
	}

	// Failed delayed operations remove their events, including failures while delivering a failure response.
	for (const bool shouldFailProduction : { false, true }) {
		std::vector<std::string> unused;
		const auto failing{ MakeEventsConnection(unused) };
		RETURN_IF_FALSE((failing != nullptr));
		uint64_t terminated{};
		MSAPI::Protocol::WebSocket::Events::TerminationReason reason{};
		MSAPI::Protocol::WebSocket::Events::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1,
			[](std::string&, const MSAPI::Protocol::WebSocket::Events::Single&) {
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Delay;
			},
			[&terminated, &reason](const MSAPI::Protocol::WebSocket::Events::Single&,
				const MSAPI::Protocol::WebSocket::Events::TerminationReason actual) {
				++terminated;
				reason = actual;
			});

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, failing, MSAPI::Json{ "{}" }));

		failing->GetConnection().SetSend([](int32_t, const void*, uint64_t, int32_t) -> int64_t {
			errno = EPIPE;
			return -1;
		});

		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1,
									 [shouldFailProduction](std::string& out) {
										 out = shouldFailProduction ? "response failed" : "1";
										 return !shouldFailProduction;
									 }),
			MSAPI::Protocol::WebSocket::Events::SendResult::IOError,
			"Delayed send exposes data or failure frame I/O errors"));
		RETURN_IF_FALSE(
			t.Assert(terminated, uint64_t{ 1 }, "Failed data or failure-frame delivery removes the event once"));
		RETURN_IF_FALSE(t.Assert(reason,
			shouldFailProduction ? MSAPI::Protocol::WebSocket::Events::TerminationReason::Failed
								 : MSAPI::Protocol::WebSocket::Events::TerminationReason::ConnectionClosed,
			"Termination distinguishes response-production failure from a failed data connection"));
		uint64_t repeatedProductions{};
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1,
									 [&repeatedProductions](std::string& out) {
										 ++repeatedProductions;
										 out = "1";
										 return true;
									 }),
			MSAPI::Protocol::WebSocket::Events::SendResult::Nothing, "Failed delayed events are no longer pending"));
		RETURN_IF_FALSE(t.Assert(repeatedProductions, uint64_t{}, "Nothing skips further response production"));
		RETURN_IF_FALSE(t.Assert(terminated, uint64_t{ 1 }, "Nothing does not repeat termination notifications"));
	}

	// Partial delayed delivery closes the failed connection and completes healthy recipients without retrying them.
	{
		std::vector<std::string> unused;
		std::vector<std::string> healthyMessages;
		const auto failing{ MakeEventsConnection(unused) };
		const auto healthy{ MakeEventsConnection(healthyMessages) };
		RETURN_IF_FALSE((failing != nullptr && healthy != nullptr));
		uint64_t writes{};
		uint64_t closedTerminations{};
		uint64_t completedTerminations{};
		bool areReasonsCorrect{ true };
		MSAPI::Protocol::WebSocket::Events::SinglesDistributor<EventsAuthorization> singles{ authorization };
		const auto handler{ [](std::string&, const MSAPI::Protocol::WebSocket::Events::Single&) {
			return MSAPI::Protocol::WebSocket::Events::HandleResult::Delay;
		} };
		const auto onTermination{ [&closedTerminations, &completedTerminations, &areReasonsCorrect](
									  const MSAPI::Protocol::WebSocket::Events::Single& single,
									  const MSAPI::Protocol::WebSocket::Events::TerminationReason reason) {
			if (single.GetId() == 1 || single.GetId() == 4) {
				++closedTerminations;
				areReasonsCorrect = areReasonsCorrect
					&& reason == MSAPI::Protocol::WebSocket::Events::TerminationReason::ConnectionClosed;
				return;
			}

			++completedTerminations;
			areReasonsCorrect
				= areReasonsCorrect && reason == MSAPI::Protocol::WebSocket::Events::TerminationReason::Completed;
		} };

		singles.SetHandlerWithoutPermissions(1, handler, onTermination);
		singles.SetHandlerWithoutPermissions(2, handler, onTermination);

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, failing, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(singles.Collect(/*id=*/2, /*hash=*/1, healthy, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(singles.Collect(/*id=*/3, /*hash=*/2, healthy, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(singles.Collect(/*id=*/4, /*hash=*/2, failing, MSAPI::Json{ "{}" }));

		failing->GetConnection().SetSend([&writes](int32_t, const void*, uint64_t, int32_t) -> int64_t {
			++writes;
			errno = EPIPE;
			return -1;
		});

		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "7" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::IOError,
			"Partial delayed delivery exposes the transport error"));
		RETURN_IF_FALSE(t.Assert(healthyMessages == std::vector<std::string>{ R"({"uids":[2],"data":7})" }, true,
			"Healthy delayed recipients receive data without failure notifications"));

		// Closed events: 1 on hash 1 + 1 on hash 2 = 2; the healthy connection retains its hash-2 event.
		RETURN_IF_FALSE(t.Assert(
			closedTerminations, uint64_t{ 2 }, "Partial delivery removes every event on the failed connection"));
		RETURN_IF_FALSE(t.Assert(
			completedTerminations, uint64_t{ 1 }, "Partial delivery completes the healthy connection's claimed event"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "8" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::Nothing,
			"A retry cannot select already completed recipients"));
		RETURN_IF_FALSE(t.Assert(healthyMessages == std::vector<std::string>{ R"({"uids":[2],"data":7})" }, true,
			"Retrying after partial failure does not duplicate successful responses"));
		RETURN_IF_FALSE(t.Assert(writes, uint64_t{ 1 }, "I/O failure cleanup attempts no additional writes"));
		RETURN_IF_FALSE(t.Assert(
			singles.CheckDelayed({ 2, MSAPI::Protocol::WebSocket::Events::IdentityFilter{} }, std::string_view{ "9" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::Success,
			"Other filters remain registered after the failed batch is removed"));

		// Completed: 1 healthy event on hash 1 + 1 healthy event on hash 2 = 2.
		RETURN_IF_FALSE(t.Assert(completedTerminations == 2 && areReasonsCorrect, true,
			"Closed connections and completed responses receive their respective termination reasons"));
	}

	// Failure cleanup runs once in SendData; callback registrations belong to a later delayed operation.
	for (const bool shouldFailProduction : { false, true }) {
		std::vector<std::string> unused;
		std::vector<std::string> retryMessages;
		const auto failing{ MakeEventsConnection(unused) };
		const auto retryConnection{ MakeEventsConnection(retryMessages) };
		RETURN_IF_FALSE((failing != nullptr && retryConnection != nullptr));
		uint64_t originalTerminations{};
		uint64_t retryTerminations{};
		bool isRetryRegistered{};
		bool areReasonsCorrect{ true };
		MSAPI::Protocol::WebSocket::Events::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1,
			[](std::string&, const MSAPI::Protocol::WebSocket::Events::Single&) {
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Delay;
			},
			[&singles, &retryConnection, &originalTerminations, &retryTerminations, &isRetryRegistered,
				&areReasonsCorrect, shouldFailProduction](const MSAPI::Protocol::WebSocket::Events::Single& single,
				const MSAPI::Protocol::WebSocket::Events::TerminationReason reason) {
				if (single.GetId() == 1) {
					++originalTerminations;
					const auto expected{ shouldFailProduction
							? MSAPI::Protocol::WebSocket::Events::TerminationReason::Failed
							: MSAPI::Protocol::WebSocket::Events::TerminationReason::ConnectionClosed };
					areReasonsCorrect = areReasonsCorrect && reason == expected;
					isRetryRegistered = singles.Collect(/*id=*/2, /*hash=*/1, retryConnection, MSAPI::Json{ "{}" });
					return;
				}

				++retryTerminations;
				areReasonsCorrect
					= areReasonsCorrect && reason == MSAPI::Protocol::WebSocket::Events::TerminationReason::Completed;
			});

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, failing, MSAPI::Json{ "{}" }));
		failing->GetConnection().SetSend([](int32_t, const void*, uint64_t, int32_t) -> int64_t {
			errno = EPIPE;
			return -1;
		});

		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1,
									 [shouldFailProduction](std::string& out) {
										 out = shouldFailProduction ? "response failed" : "1";
										 return !shouldFailProduction;
									 }),
			MSAPI::Protocol::WebSocket::Events::SendResult::IOError,
			"Failed data or failure delivery exposes its I/O error"));
		RETURN_IF_FALSE(t.Assert(originalTerminations, uint64_t{ 1 }, "The failed operation notifies its event once"));
		RETURN_IF_FALSE(t.Assert(isRetryRegistered, true, "A failure callback can register a later operation"));
		RETURN_IF_FALSE(t.Assert(retryTerminations, uint64_t{}, "CheckDelayed does not repeat failure cleanup"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "9" }),
			MSAPI::Protocol::WebSocket::Events::SendResult::Success, "The callback registration remains available"));
		RETURN_IF_FALSE(t.Assert(retryMessages == std::vector<std::string>{ R"({"uids":[2],"data":9})" }, true,
			"The later operation receives its response"));
		RETURN_IF_FALSE(t.Assert(retryTerminations == 1 && areReasonsCorrect, true,
			"The later operation terminates only after its successful response"));
	}

	// Initial response strings retain capacity while each invocation receives a fresh prefix.
	for (const auto type : { E::Type::Single, E::Type::Stream }) {
		messages.clear();
		uint64_t retainedCapacity{};
		bool isCapacityReused{};
		const auto handler{ [&retainedCapacity, &isCapacityReused](std::string& out, const auto& event) {
			if (event.GetId() == 1) {
				const uint64_t extraCapacity{ 8192 };
				out.reserve(out.capacity() + extraCapacity);
				retainedCapacity = out.capacity();
			}
			else {
				isCapacityReused = out.capacity() >= retainedCapacity;
			}

			out += "1";
			return E::HandleResult::Success;
		} };

		if (type == E::Type::Single) {
			E::SinglesDistributor<EventsAuthorization> singles{ authorization };
			singles.SetHandlerWithoutPermissions(1, handler, /*terminationHandler=*/{});
			RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
			RETURN_IF_FALSE(singles.Collect(/*id=*/2, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}
		else {
			DistributorT distributor{ authorization };
			distributor.SetHandlerWithoutPermissions(1, handler, /*terminationHandler=*/{});
			RETURN_IF_FALSE(distributor.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
			RETURN_IF_FALSE(distributor.Collect(/*id=*/2, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		RETURN_IF_FALSE(
			t.Assert(isCapacityReused, true, "Initial responses reuse previously reserved string capacity"));
		RETURN_IF_FALSE(
			t.Assert(std::find(messages.begin(), messages.end(), R"({"uids":[1],"data":1})") != messages.end()
					&& std::find(messages.begin(), messages.end(), R"({"uids":[2],"data":1})") != messages.end(),
				true, "Reused initial buffers contain only the current response"));
	}

	// Lazy broadcasts reuse their producer string, clearing it before each callback.
	{
		DistributorT distributor{ authorization };
		distributor.SetHandlerWithoutPermissions(1,
			[](std::string&, const E::Stream&) { return E::HandleResult::SuccessChunked; }, /*terminationHandler=*/{});
		RETURN_IF_FALSE(distributor.Collect(/*id=*/11, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/22, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(distributor.Collect(/*id=*/33333, /*hash=*/1, otherConnection, MSAPI::Json{ "{}" }));
		messages.clear();
		otherMessages.clear();
		uint64_t calls{};
		uint64_t retainedCapacity{};
		bool isInputEmpty{ true };
		bool isCapacityReused{};

		const auto produce{ [&calls, &retainedCapacity, &isInputEmpty, &isCapacityReused](std::string& out) {
			isInputEmpty = isInputEmpty && out.empty();
			if (calls++ == 0) {
				const uint64_t extraCapacity{ 8192 };
				out.reserve(out.capacity() + extraCapacity);
				retainedCapacity = out.capacity();
				out += "12345";
			}
			else {
				isCapacityReused = out.capacity() >= retainedCapacity;
				out += "6";
			}

			return true;
		} };

		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, produce), E::SendResult::Success,
			"The producer's output is wrapped in a broadcast response"));
		RETURN_IF_FALSE(t.Assert(distributor.SendData(filterHash1, produce), E::SendResult::Success,
			"A later broadcast reuses the same construction buffer"));
		RETURN_IF_FALSE(t.Assert(isInputEmpty, true, "Each producer invocation receives an empty string"));
		RETURN_IF_FALSE(t.Assert(isCapacityReused, true, "Broadcast output retains string capacity between calls"));
		RETURN_IF_FALSE(t.Assert(
			messages == std::vector<std::string>{ R"({"uids":[11,22],"data":12345})", R"({"uids":[11,22],"data":6})" },
			true, "Reused envelopes preserve multiple recipient IDs and contain no stale output"));
		RETURN_IF_FALSE(t.Assert(otherMessages
				== std::vector<std::string>{ R"({"uids":[33333],"data":12345})", R"({"uids":[33333],"data":6})" },
			true, "Each connection receives its own header from the shared payload"));
	}

	// Duplicate hashes are rejected across both registration methods without replacing callbacks or moving handlers.
	for (const bool isOriginalPermissionRequired : { false, true }) {
		for (const bool isRejectedPermissionRequired : { false, true }) {
			uint64_t originalCalls{};
			uint64_t rejectedCalls{};
			uint64_t originalTerminations{};
			uint64_t rejectedTerminations{};
			DistributorT distributor{ authorization };

			const auto originalHandler{ [&originalCalls](std::string&, const E::Stream&) {
				++originalCalls;
				return E::HandleResult::SuccessChunked;
			} };
			const auto originalTermination{ [&originalTerminations](const E::Stream&, const E::TerminationReason) {
				++originalTerminations;
			} };

			if (isOriginalPermissionRequired) {
				distributor.SetHandlerWithPermissions(
					77, originalHandler, originalTermination, EventsAuthorization::Grade::Reader);
			}
			else {
				distributor.SetHandlerWithoutPermissions(77, originalHandler, originalTermination);
			}

			E::Stream::BaseT::HandlerT rejectedHandler{ [&rejectedCalls](std::string&, const E::Stream&) {
				++rejectedCalls;
				return E::HandleResult::SuccessChunked;
			} };
			const auto rejectedTermination{ [&rejectedTerminations](const E::Stream&, const E::TerminationReason) {
				++rejectedTerminations;
			} };

			if (isRejectedPermissionRequired) {
				distributor.SetHandlerWithPermissions(
					77, std::move(rejectedHandler), rejectedTermination, EventsAuthorization::Grade::Reader);
			}
			else {
				distributor.SetHandlerWithoutPermissions(77, std::move(rejectedHandler), rejectedTermination);
			}

			RETURN_IF_FALSE(
				t.Assert(rejectedHandler != nullptr, true, "Rejected registration does not move its handler"));
			RETURN_IF_FALSE(distributor.Collect(/*id=*/7, /*hash=*/77, connection, MSAPI::Json{ "{}" }));
			RETURN_IF_FALSE(
				t.Assert(originalCalls, uint64_t{ 1 }, "Duplicate registration preserves the original handler"));
			RETURN_IF_FALSE(t.Assert(rejectedCalls, uint64_t{}, "Rejected handlers are never invoked"));
			distributor.ClearActiveEventsForConnectionId(connection->GetConnectionId());
			RETURN_IF_FALSE(
				t.Assert(originalTerminations, uint64_t{ 1 }, "The original termination callback remains registered"));
			RETURN_IF_FALSE(
				t.Assert(rejectedTerminations, uint64_t{}, "Rejected termination callbacks are never invoked"));
		}
	}

	// Registrations made by the producer or a completion callback belong to the next delivery, on success or failure.
	for (const bool shouldFailProduction : { false, true }) {
		messages.clear();
		uint64_t originalTerminations{};
		uint64_t laterTerminations{};
		bool isProducerRegistrationSuccessful{};
		bool isCallbackRegistrationSuccessful{};
		bool areReasonsCorrect{ true };
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&singles, &connection, &originalTerminations, &laterTerminations, &isCallbackRegistrationSuccessful,
				&areReasonsCorrect, shouldFailProduction](const E::Single& single, const E::TerminationReason reason) {
				if (single.GetId() == 1) {
					++originalTerminations;
					areReasonsCorrect = areReasonsCorrect
						&& reason
							== (shouldFailProduction ? E::TerminationReason::Failed : E::TerminationReason::Completed);
					isCallbackRegistrationSuccessful
						= singles.Collect(/*id=*/3, /*hash=*/1, connection, MSAPI::Json{ "{}" });
					return;
				}

				++laterTerminations;
				areReasonsCorrect = areReasonsCorrect && reason == E::TerminationReason::Completed;
			});

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		const auto result{ singles.CheckDelayed(filterHash1,
			[&singles, &connection, &isProducerRegistrationSuccessful, shouldFailProduction](std::string& out) {
				isProducerRegistrationSuccessful
					= singles.Collect(/*id=*/2, /*hash=*/1, connection, MSAPI::Json{ "{}" });
				out = shouldFailProduction ? "owned response failed" : "7";

				return !shouldFailProduction;
			}) };

		RETURN_IF_FALSE(t.Assert(result, shouldFailProduction ? E::SendResult::Fail : E::SendResult::Success,
			"Owned delivery reports the producer's outcome"));
		RETURN_IF_FALSE(t.Assert(isProducerRegistrationSuccessful, true, "The producer can register a later event"));
		RETURN_IF_FALSE(t.Assert(isCallbackRegistrationSuccessful, true, "Termination can register a later event"));
		RETURN_IF_FALSE(t.Assert(originalTerminations, uint64_t{ 1 }, "Only the claimed event terminates initially"));
		RETURN_IF_FALSE(
			t.Assert(laterTerminations, uint64_t{}, "Later registrations are not removed with the claimed batch"));
		const std::string expected{ shouldFailProduction ? R"({"uids":[1],"state":4,"error":"owned response failed"})"
														 : R"({"uids":[1],"data":7})" };
		RETURN_IF_FALSE(t.Assert(messages == std::vector<std::string>{ expected }, true,
			"Success and failure delivery address only the claimed event"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "8" }), E::SendResult::Success,
			"Later registrations remain available for another delivery"));
		RETURN_IF_FALSE(t.Assert(messages.back(), std::string{ R"({"uids":[2,3],"data":8})" },
			"Both later registrations receive their own response"));

		// Later terminations: 1 registration from the producer + 1 from the termination callback = 2.
		RETURN_IF_FALSE(t.Assert(laterTerminations == 2 && areReasonsCorrect, true,
			"Each owned batch reports its own termination outcome exactly once"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "9" }), E::SendResult::Nothing,
			"No completed event can be delivered again"));
	}

	// One blocked producer owns its batch while another thread can check or deliver newly registered events.
	{
		messages.clear();
		std::atomic<uint64_t> productions{};
		std::atomic<uint64_t> notifications{};
		std::atomic<bool> areReasonsCorrect{ true };
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&notifications, &areReasonsCorrect](const E::Single&, const E::TerminationReason reason) {
				notifications.fetch_add(1);
				if (reason != E::TerminationReason::Completed) {
					areReasonsCorrect.store(false);
				}
			});

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		std::barrier claimed{ 2 };
		std::barrier finish{ 2 };
		E::SendResult firstResult{};
		std::thread first{ [&singles, &productions, &claimed, &finish, &firstResult, filterHash1] {
			firstResult = singles.CheckDelayed(filterHash1, [&productions, &claimed, &finish](std::string& out) {
				productions.fetch_add(1);
				claimed.arrive_and_wait();
				finish.arrive_and_wait();
				out = "7";

				return true;
			});
		} };

		claimed.arrive_and_wait();
		const auto secondResult{ singles.CheckDelayed(filterHash1, [&productions](std::string& out) {
			productions.fetch_add(1);
			out = "8";

			return true;
		}) };

		const auto isRegistered{ singles.Collect(/*id=*/2, /*hash=*/1, connection, MSAPI::Json{ "{}" }) };
		const auto laterResult{ singles.CheckDelayed(filterHash1, std::string_view{ "8" }) };
		// Join before any assertion can return from the test and leave a blocked sender behind.
		finish.arrive_and_wait();
		first.join();
		RETURN_IF_FALSE(t.Assert(firstResult, E::SendResult::Success, "The first owner finishes its blocked delivery"));
		RETURN_IF_FALSE(
			t.Assert(secondResult, E::SendResult::Nothing, "A concurrent check cannot reclaim the same event"));
		RETURN_IF_FALSE(
			t.Assert(productions.load(), uint64_t{ 1 }, "No producer runs when another delivery owns all events"));
		RETURN_IF_FALSE(t.Assert(isRegistered, true, "Registration proceeds while the first producer is blocked"));
		RETURN_IF_FALSE(t.Assert(laterResult, E::SendResult::Success, "A later batch can complete independently"));
		RETURN_IF_FALSE(
			t.Assert(messages == std::vector<std::string>{ R"({"uids":[2],"data":8})", R"({"uids":[1],"data":7})" },
				true, "Concurrent deliveries each address their own batch once"));

		// Notifications: 1 event in the blocked batch + 1 in the later batch = 2.
		RETURN_IF_FALSE(t.Assert(notifications.load(), uint64_t{ 2 }, "Both claimed events are notified exactly once"));
		RETURN_IF_FALSE(t.Assert(areReasonsCorrect.load(), true, "Concurrent successful batches report Completed"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "9" }), E::SendResult::Nothing,
			"No events remain after both owners finish"));
	}

	// Registry removal after a batch is claimed cannot notify or cancel that batch, or invalidate its transport data.
	for (const auto removal : { E::TerminationReason::ClientInterrupted, E::TerminationReason::ConnectionClosed,
			 E::TerminationReason::Cleanup, E::TerminationReason::RemovedByFilter, E::TerminationReason::Failed }) {
		std::vector<std::string> ownedMessages;
		auto ownedConnection{ MakeEventsConnection(ownedMessages) };
		RETURN_IF_FALSE((ownedConnection != nullptr));
		const auto connectionId{ ownedConnection->GetConnectionId() };
		uint64_t notifications{};
		bool isContextCorrect{};
		bool isRemovalSuccessful{ true };
		bool isNotifiedBeforeSend{};
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&notifications, &isContextCorrect, connectionId](
				const E::Single& single, const E::TerminationReason reason) {
				++notifications;
				isContextCorrect = single.GetId() == 1 && single.GetConnectionData()->GetConnectionId() == connectionId
					&& reason == E::TerminationReason::Completed;
			});

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, ownedConnection, MSAPI::Json{ "{}" }));
		const auto result{ singles.CheckDelayed(filterHash1,
			[&singles, &ownedConnection, &notifications, &isRemovalSuccessful, &isNotifiedBeforeSend, connectionId,
				filterHash1, removal](std::string& out) {
				switch (removal) {
				case E::TerminationReason::ClientInterrupted:
					isRemovalSuccessful = singles.Collect(
						/*id=*/1, /*hash=*/1, ownedConnection, MSAPI::Json{ R"({"interrupt":true})" });
					break;
				case E::TerminationReason::ConnectionClosed:
					singles.ClearActiveEventsForConnectionId(connectionId);
					break;
				case E::TerminationReason::Cleanup:
					isRemovalSuccessful = singles.FailActiveEvents("cleanup");
					break;
				case E::TerminationReason::RemovedByFilter:
					singles.EraseEventsOnConnectionsByFilter(filterHash1);
					break;
				case E::TerminationReason::Failed:
					isRemovalSuccessful = singles.FailEventsOnConnectionsByFilter(filterHash1, "cleanup");
					break;
				default:
					isRemovalSuccessful = false;
					break;
				}

				isNotifiedBeforeSend = notifications != 0;
				ownedConnection.reset();
				out = "7";

				return true;
			}) };

		RETURN_IF_FALSE(
			t.Assert(result, E::SendResult::Success, "The batch owner still delivers after registry removal"));
		RETURN_IF_FALSE(
			t.Assert(isRemovalSuccessful, true, "Registry removal proceeds during production without deadlock"));
		RETURN_IF_FALSE(t.Assert(isNotifiedBeforeSend, false, "Other removal paths cannot notify a claimed event"));
		RETURN_IF_FALSE(t.Assert(notifications, uint64_t{ 1 }, "The delivery owner alone notifies its claimed event"));
		RETURN_IF_FALSE(
			t.Assert(isContextCorrect, true, "Claimed events retain their connection data through completion"));
		RETURN_IF_FALSE(t.Assert(ownedMessages == std::vector<std::string>{ R"({"uids":[1],"data":7})" }, true,
			"No competing failure frame is sent for an already claimed event"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "8" }), E::SendResult::Nothing,
			"Registry removal leaves no duplicate response work"));
	}

	// Claimed events stop counting against the pending-event limit before the producer registers another full batch.
	{
		uint64_t completed{};
		uint64_t purged{};
		bool areReasonsCorrect{ true };
		bool isRegistrationSuccessful{ true };
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&completed, &purged, &areReasonsCorrect](const E::Single&, const E::TerminationReason reason) {
				if (reason == E::TerminationReason::Completed) {
					++completed;
				}
				else if (reason == E::TerminationReason::LimitExceeded) {
					++purged;
				}
				else {
					areReasonsCorrect = false;
				}
			});

		const uint64_t batchSize{ 64 };
		RETURN_IF_FALSE(singles.SetEventsLimit(connection->GetConnectionId(), batchSize));
		for (uint64_t id{ 1 }; id <= batchSize; ++id) {
			RETURN_IF_FALSE(singles.Collect(/*id=*/id, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		}

		const auto result{ singles.CheckDelayed(
			filterHash1, [&singles, &connection, &isRegistrationSuccessful, batchSize](std::string& out) {
				for (uint64_t id{ batchSize + 1 }; id <= batchSize * 2; ++id) {
					isRegistrationSuccessful = singles.Collect(/*id=*/id, /*hash=*/1, connection, MSAPI::Json{ "{}" })
						&& isRegistrationSuccessful;
				}

				out = "7";
				return true;
			}) };

		RETURN_IF_FALSE(t.Assert(result, E::SendResult::Success, "The original full batch completes"));
		RETURN_IF_FALSE(
			t.Assert(isRegistrationSuccessful, true, "A second full batch can be registered during delivery"));
		RETURN_IF_FALSE(t.Assert(completed, batchSize, "Only the first claimed batch completes initially"));
		RETURN_IF_FALSE(
			t.Assert(purged, uint64_t{}, "In-flight events do not cause newly pending events to be purged"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "8" }), E::SendResult::Success,
			"The second full batch remains pending for later delivery"));

		// Completed: 64 events in the first batch + 64 in the second = 128, or batchSize * 2.
		RETURN_IF_FALSE(t.Assert(completed == batchSize * 2 && purged == 0 && areReasonsCorrect, true,
			"Claiming and completing batches keeps pending-event accounting consistent"));
	}

	// A throwing producer still finalizes its owned batch and reports a safe failure response.
	{
		messages.clear();
		uint64_t notifications{};
		E::TerminationReason reason{};
		E::SinglesDistributor<EventsAuthorization> singles{ authorization };
		singles.SetHandlerWithoutPermissions(
			1, [](std::string&, const E::Single&) { return E::HandleResult::Delay; },
			[&notifications, &reason](const E::Single&, const E::TerminationReason actual) {
				++notifications;
				reason = actual;
			});

		RETURN_IF_FALSE(singles.Collect(/*id=*/1, /*hash=*/1, connection, MSAPI::Json{ "{}" }));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1,
									 [](std::string&) -> bool { throw std::runtime_error{ "producer failure" }; }),
			E::SendResult::Fail, "Producer exceptions become delivery failures"));
		RETURN_IF_FALSE(t.Assert(
			messages == std::vector<std::string>{ R"({"uids":[1],"state":4,"error":"Event data production failed"})" },
			true, "Producer exceptions use a failure description that satisfies the error-text contract"));
		RETURN_IF_FALSE(t.Assert(notifications == 1 && reason == E::TerminationReason::Failed, true,
			"A throwing producer notifies its claimed event once with Failed"));
		RETURN_IF_FALSE(t.Assert(singles.CheckDelayed(filterHash1, std::string_view{ "8" }), E::SendResult::Nothing,
			"A throwing producer leaves no claimed event pending"));
	}

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_WEB_SOCKET_EVENTS_INL