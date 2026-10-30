/**************************
 * @file        delayedOwnership.cpp
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
 * @brief Tests claimed batch lifetime, pending-event accounting and producer exceptions.
 */

#include "events.h"
#include <stdexcept>

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsDelayedOwnership(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI