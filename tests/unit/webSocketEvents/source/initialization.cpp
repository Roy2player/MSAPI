/**************************
 * @file        initialization.cpp
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
 * @brief Tests regular, chunked and failed initialization and recipient isolation.
 */

#include "events.h"

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsInitialization(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, std::vector<std::string>& otherMessages,
	const std::shared_ptr<MSAPI::Connection::Data>& connection,
	const std::shared_ptr<MSAPI::Connection::Data>& otherConnection)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;
	using DistributorT = E::StreamsDistributor<EventsAuthorization>;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI