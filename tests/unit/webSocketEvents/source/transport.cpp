/**************************
 * @file        transport.cpp
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
 * @brief Tests initialization, cleanup and broadcast transport failures.
 */

#include "events.h"

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEventsTransport(MSAPI::Test::Test& t, EventsAuthorization& authorization)
{
	namespace E = MSAPI::Protocol::WebSocket::Events;
	using DistributorT = E::StreamsDistributor<EventsAuthorization>;

	// Matches event hash 1 and the default identity across connections.
	const E::StreamsDistributor<EventsAuthorization>::FilterT filterHash1{ /*hash=*/1, E::IdentityFilter{} };

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

	return true;
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI