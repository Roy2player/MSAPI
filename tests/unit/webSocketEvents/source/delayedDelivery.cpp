/**************************
 * @file        delayedDelivery.cpp
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
 * @brief Tests reentrant registrations and concurrent owners of delayed batches.
 */

#include "events.h"
#include <barrier>

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsDelayedDelivery(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI