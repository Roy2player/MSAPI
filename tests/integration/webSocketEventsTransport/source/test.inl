/**************************
 * @file        test.inl
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

#ifndef MSAPI_INTEGRATION_TEST_WEB_SOCKET_EVENTS_TRANSPORT_INL
#define MSAPI_INTEGRATION_TEST_WEB_SOCKET_EVENTS_TRANSPORT_INL

#include "../../../../library/source/protocol/webSocketEvents.inl"
#include "../../../../library/source/test/test.inl"

namespace MSAPI {

namespace Test {

namespace Integration {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Tests regular, chunked and partial-failure event sequences through real sockets, WebSocket serialization,
 * receive buffers and the existing WebSocket parser. Also tests connection cleanup after transport closure.
 *
 * @return True if every assertion passes.
 */
FORCE_INLINE [[nodiscard]] bool WebSocketEventsTransport();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool WebSocketEventsTransport()
{
	LOG_INFO("MSAPI INTEGRATION TEST: WebSocket events transport");

	MSAPI::Test::Test t;

	int32_t sockets[2]{};
	RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, /*protocol=*/0, sockets), 0, "Transport socket pair"));

	const auto server{ std::make_shared<MSAPI::Connection::Data>(std::make_unique<MSAPI::Connection>(sockets[0]),
		MSAPI::SString<16>{}, /*ip=*/0, /*port=*/0, MSAPI::Connection::Type::Income, /*doReconnection=*/false) };
	const auto client{ std::make_shared<MSAPI::Connection::Data>(std::make_unique<MSAPI::Connection>(sockets[1]),
		MSAPI::SString<16>{}, /*ip=*/0, /*port=*/0, MSAPI::Connection::Type::Outcome, /*doReconnection=*/false) };

	uint64_t terminated{};
	MSAPI::Protocol::WebSocket::Events::TerminationReason terminationReason{};
	MSAPI::Authorization::Base::Module<> authorization;
	const std::string largeChunk{ "\"" + std::string(/*count=*/200, 'x') + "\"" };

	MSAPI::Protocol::WebSocket::Events::StreamsDistributor<decltype(authorization)> distributor{ authorization };
	distributor.SetHandlerWithoutPermissions(
		1,
		[&largeChunk](std::string& out, const MSAPI::Protocol::WebSocket::Events::Stream& stream) {
			if (stream.GetId() == 1) {
				out += "[1,2]";
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Success;
			}

			if (stream.GetId() != 4) {
				if (!stream.SendData(largeChunk) || !stream.SendData("[3,4]")) {
					return MSAPI::Protocol::WebSocket::Events::HandleResult::IOError;
				}
			}

			if (stream.GetId() >= 3) {
				out = "snapshot failed";
				return MSAPI::Protocol::WebSocket::Events::HandleResult::Fail;
			}

			out = "UNUSED";
			return MSAPI::Protocol::WebSocket::Events::HandleResult::SuccessChunked;
		},
		[&terminated, &terminationReason](const MSAPI::Protocol::WebSocket::Events::Stream&,
			const MSAPI::Protocol::WebSocket::Events::TerminationReason reason) {
			++terminated;
			terminationReason = reason;
		});

	for (uint64_t id{ 1 }; id <= 4; ++id) {
		RETURN_IF_FALSE(distributor.Collect(id, 1, server, MSAPI::Json{ "{}" }));

		std::vector<std::string> expected{ std::format("{{\"uids\":[{}],\"state\":2}}", id) };

		if (id == 1) {
			expected.emplace_back(R"({"uids":[1],"data":[1,2]})");
		}
		else if (id != 4) {
			expected.emplace_back(std::format("{{\"uids\":[{}],\"data\":{}}}", id, largeChunk));
			expected.emplace_back(std::format("{{\"uids\":[{}],\"data\":[3,4]}}", id));
		}
		if (id >= 3) {
			expected.emplace_back(std::format("{{\"uids\":[{}],\"state\":4,\"error\":\"snapshot failed\"}}", id));
		}
		else {
			expected.emplace_back(std::format("{{\"uids\":[{}],\"state\":3}}", id));
		}

		MSAPI::RecvBuffer buffer{ client, /*capacityLimit=*/4096, /*toProcessSize=*/2 };
		for (const auto& payload : expected) {
			RETURN_IF_FALSE(t.Assert(buffer.Recv().GetBufferSize(), uint64_t{ 2 }, "Receive WebSocket header"));
			const MSAPI::Protocol::WebSocket::Data frame{ buffer };
			RETURN_IF_FALSE(t.Assert(frame.IsValid(), true, "Existing WebSocket parser accepts event frame"));
			const auto data{ frame.GetPayload() };
			RETURN_IF_FALSE(t.Assert(std::string_view{ reinterpret_cast<const char*>(data.data()), data.size() },
				std::string_view{ payload }, "Real transport preserves initialization order and wire format"));
		}
	}

	RETURN_IF_FALSE(t.Assert(terminated, uint64_t{}, "Initialization failures are not terminations"));
	RETURN_IF_FALSE(t.Assert(
		distributor.SendData({ 1, MSAPI::Protocol::WebSocket::Events::IdentityFilter{} }, std::string_view{ "[5]" }),
		MSAPI::Protocol::WebSocket::Events::SendResult::Success, "Broadcast succeeds over the real connection"));

	{
		MSAPI::RecvBuffer buffer{ client, /*capacityLimit=*/4096, /*toProcessSize=*/2 };
		RETURN_IF_FALSE(t.Assert(buffer.Recv().GetBufferSize(), uint64_t{ 2 }, "Receive broadcast frame header"));
		const MSAPI::Protocol::WebSocket::Data frame{ buffer };
		RETURN_IF_FALSE(t.Assert(frame.IsValid(), true, "Existing WebSocket parser accepts a batched broadcast"));
		const auto payload{ frame.GetPayload() };
		RETURN_IF_FALSE(t.Assert(std::string_view{ reinterpret_cast<const char*>(payload.data()), payload.size() },
			std::string_view{ R"({"uids":[1,2],"data":[5]})" }, "One frame addresses both registered streams"));
	}

	RETURN_IF_FALSE(t.Assert(terminated, uint64_t{}, "Broadcast preserves registered streams"));
	server->GetConnection().Close();
	distributor.ClearActiveEventsForConnectionId(server->GetConnectionId());
	RETURN_IF_FALSE(t.Assert(terminated, uint64_t{ 2 }, "Connection cleanup terminates only registered streams"));
	RETURN_IF_FALSE(t.Assert(terminationReason, MSAPI::Protocol::WebSocket::Events::TerminationReason::ConnectionClosed,
		"Connection cleanup reason"));

	return t.Passed<bool>();
}

} // namespace Integration

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_INTEGRATION_TEST_WEB_SOCKET_EVENTS_TRANSPORT_INL