/**************************
 * @file        buffersAndRegistration.cpp
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
 * @brief Tests response buffer reuse and duplicate handler registration.
 */

#include "events.h"

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsBuffersAndRegistration(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, std::vector<std::string>& otherMessages,
	const std::shared_ptr<MSAPI::Connection::Data>& connection,
	const std::shared_ptr<MSAPI::Connection::Data>& otherConnection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;
	using DistributorT = E::StreamsDistributor<EventsAuthorization>;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI