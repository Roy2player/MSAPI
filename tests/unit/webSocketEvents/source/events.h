/**************************
 * @file        events.h
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
 *
 * @brief Declares the shared WebSocket events unit suite and its separately compiled test groups.
 */

#ifndef MSAPI_UNIT_TEST_WEB_SOCKET_EVENTS_H
#define MSAPI_UNIT_TEST_WEB_SOCKET_EVENTS_H

#include "../../../../library/source/protocol/webSocketEvents.inl"
#include "../../../../library/source/test/test.inl"

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Authorization stub for public and permission-checked handler registration.
 *
 * @concurrency No.
 */
class EventsAuthorization {
public:
	/**************************
	 * @brief Minimum grade used by the authorization stub.
	 */
	enum class Grade : int16_t { Reader };
	using grade_t = Grade;

public:
	FORCE_INLINE [[nodiscard]] bool IsAccessGranted(uint64_t id, Grade grade) const noexcept;
};

/**************************
 * @brief Creates a connection whose sender records decoded text payloads from real WebSocket frames.
 *
 * @param messages Destination for payloads; outlives the connection and all send operations.
 *
 * @return Connection data or nullptr if socket creation fails.
 */
FORCE_INLINE [[nodiscard]] std::shared_ptr<MSAPI::Connection::Data> MakeEventsConnection(
	std::vector<std::string>& messages);

/**************************
 * @brief Tests synchronous regular, chunked, partial and failed initialization, recipient isolation, broadcasts,
 * owned delayed batches, every termination reason, reentrant callbacks, deferred purges, exception containment and
 * racing removals, enum numeric values and string conversions.
 *
 * @note Compiles the suite and its groups out of line to isolate optimization in separate translation units.
 *
 * @return True if all assertions pass.
 */
[[nodiscard]] bool WebSocketEvents();

/**************************
 * @brief Tests regular, chunked and failed initialization and recipient isolation.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param otherMessages Recorded payloads for the second connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 * @param otherConnection Second connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsInitialization(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, std::vector<std::string>& otherMessages,
	const std::shared_ptr<MSAPI::Connection::Data>& connection,
	const std::shared_ptr<MSAPI::Connection::Data>& otherConnection);

/**************************
 * @brief Tests removal callbacks, stale registration handles and event limits.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsRemovalAndLimits(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection);

/**************************
 * @brief Tests reentrant and concurrent changes during deferred purging.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsDeferredPurging(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection);

/**************************
 * @brief Tests selected recipients, termination callbacks, derived events and racing removers.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsTermination(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection);

/**************************
 * @brief Tests initialization, cleanup and broadcast transport failures.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsTransport(MSAPI::Test::Test& t, EventsAuthorization& authorization);

/**************************
 * @brief Tests delayed delivery failures, partial delivery and callback registrations.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsDelayedTransport(MSAPI::Test::Test& t, EventsAuthorization& authorization);

/**************************
 * @brief Tests response buffer reuse and duplicate handler registration.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param otherMessages Recorded payloads for the second connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 * @param otherConnection Second connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsBuffersAndRegistration(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, std::vector<std::string>& otherMessages,
	const std::shared_ptr<MSAPI::Connection::Data>& connection,
	const std::shared_ptr<MSAPI::Connection::Data>& otherConnection);

/**************************
 * @brief Tests reentrant registrations and concurrent owners of delayed batches.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsDelayedDelivery(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection);

/**************************
 * @brief Tests claimed batch lifetime, pending-event accounting and producer exceptions.
 *
 * @param t Shared assertion counter for the complete suite.
 * @param authorization Authorization stub shared by the suite.
 * @param messages Recorded payloads for the primary connection; outlives every callback.
 * @param connection Primary connection retained by the suite through all test groups.
 *
 * @return True if every check in this group succeeds.
 */
[[nodiscard]] bool WebSocketEventsDelayedOwnership(MSAPI::Test::Test& t, EventsAuthorization& authorization,
	std::vector<std::string>& messages, const std::shared_ptr<MSAPI::Connection::Data>& connection);

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
EventsAuthorization
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool EventsAuthorization::IsAccessGranted(
	[[maybe_unused]] const uint64_t id, [[maybe_unused]] const Grade grade) const noexcept
{
	return true;
}

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] std::shared_ptr<MSAPI::Connection::Data> MakeEventsConnection(
	std::vector<std::string>& messages)
{
	// Real descriptors keep Connection cleanup valid.
	int32_t sockets[2]{};
	if (socketpair(AF_UNIX, SOCK_STREAM, /*protocol=*/0, sockets) != 0) {
		return {};
	}

	// All sends are intercepted, so the peer descriptor is unused.
	(void)close(sockets[1]);
	auto connection{ std::make_unique<MSAPI::Connection>(sockets[0]) };
	connection->SetSend([&messages](int32_t, const void* buffer, const uint64_t size, int32_t) -> int64_t {
		const auto* bytes{ static_cast<const uint8_t*>(buffer) };
		if (size < 2 || bytes[0] != 0x81 || (bytes[1] & 0x80) != 0) {
			return -1;
		}

		uint64_t length{ static_cast<uint64_t>(bytes[1] & 0x7f) };
		uint64_t offset{ 2 };
		if (length >= 126) {
			const uint64_t extra{ length == 126 ? uint64_t{ 2 } : uint64_t{ 8 } };
			if (size < offset + extra) {
				return -1;
			}

			length = 0;
			for (uint64_t i{}; i < extra; ++i) {
				length = (length << 8) | bytes[offset++];
			}
		}

		if (size != offset + length) {
			return -1;
		}

		messages.emplace_back(reinterpret_cast<const char*>(bytes + offset), length);
		return static_cast<int64_t>(size);
	});

	return std::make_shared<MSAPI::Connection::Data>(std::move(connection), MSAPI::SString<16>{},
		/*ip=*/0, /*port=*/0, MSAPI::Connection::Type::Income, /*doReconnection=*/false);
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_WEB_SOCKET_EVENTS_H