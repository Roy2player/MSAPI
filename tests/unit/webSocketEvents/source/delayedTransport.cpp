/**************************
 * @file        delayedTransport.cpp
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
 * @brief Tests delayed delivery failures, partial delivery and callback registrations.
 */

#include "events.h"

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsDelayedTransport(MSAPI::Test::Test& t, EventsAuthorization& authorization)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI