/**************************
 * @file        deferredPurging.cpp
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
 * @brief Tests reentrant and concurrent changes during deferred purging.
 */

#include "events.h"
#include <barrier>

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsDeferredPurging(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI