/**************************
 * @file        removalAndLimits.cpp
 * @date        2026-10-30
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
 * @brief Tests removal callbacks, stale registration handles and event limits.
 */

#include "events.h"

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsRemovalAndLimits(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;
	using DistributorT = E::StreamsDistributor<EventsAuthorization>;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI