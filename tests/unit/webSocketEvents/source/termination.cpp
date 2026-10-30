/**************************
 * @file        termination.cpp
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
 * @brief Tests selected recipients, termination callbacks, derived events and racing removers.
 */

#include "events.h"
#include <barrier>
#include <stdexcept>

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsTermination(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;
	using DistributorT = E::StreamsDistributor<EventsAuthorization>;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI