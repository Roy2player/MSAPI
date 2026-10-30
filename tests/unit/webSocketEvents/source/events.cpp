/**************************
 * @file        events.cpp
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
 * @brief Runs every WebSocket events test group with shared setup and one assertion report.
 */

#include "events.h"

namespace MSAPI {

namespace Test {

namespace Unit {

[[nodiscard]] bool WebSocketEvents()
{
	namespace E = MSAPI::Protocol::WebSocket::Events;

	static_assert(U(E::Type::Undefined) == 0);
	static_assert(E::EnumToString(E::Type::Undefined) == "Undefined");
	static_assert(U(E::Type::Single) == 1);
	static_assert(E::EnumToString(E::Type::Single) == "Single");
	static_assert(U(E::Type::Stream) == 2);
	static_assert(E::EnumToString(E::Type::Stream) == "Stream");
	static_assert(U(E::Type::Max) == 3);
	static_assert(E::EnumToString(E::Type::Max) == "Max");

	static_assert(U(E::HandleResult::Undefined) == 0);
	static_assert(E::EnumToString(E::HandleResult::Undefined) == "Undefined");
	static_assert(U(E::HandleResult::Success) == 1);
	static_assert(E::EnumToString(E::HandleResult::Success) == "Success");
	static_assert(U(E::HandleResult::Fail) == 2);
	static_assert(E::EnumToString(E::HandleResult::Fail) == "Fail");
	static_assert(U(E::HandleResult::Delay) == 3);
	static_assert(E::EnumToString(E::HandleResult::Delay) == "Delay");
	static_assert(U(E::HandleResult::SuccessChunked) == 4);
	static_assert(E::EnumToString(E::HandleResult::SuccessChunked) == "SuccessChunked");
	static_assert(U(E::HandleResult::IOError) == 5);
	static_assert(E::EnumToString(E::HandleResult::IOError) == "IOError");
	static_assert(U(E::HandleResult::Max) == 6);
	static_assert(E::EnumToString(E::HandleResult::Max) == "Max");

	static_assert(U(E::TerminationReason::Undefined) == 0);
	static_assert(E::EnumToString(E::TerminationReason::Undefined) == "Undefined");
	static_assert(U(E::TerminationReason::ClientInterrupted) == 1);
	static_assert(E::EnumToString(E::TerminationReason::ClientInterrupted) == "ClientInterrupted");
	static_assert(U(E::TerminationReason::ConnectionClosed) == 2);
	static_assert(E::EnumToString(E::TerminationReason::ConnectionClosed) == "ConnectionClosed");
	static_assert(U(E::TerminationReason::Cleanup) == 3);
	static_assert(E::EnumToString(E::TerminationReason::Cleanup) == "Cleanup");
	static_assert(U(E::TerminationReason::LimitExceeded) == 4);
	static_assert(E::EnumToString(E::TerminationReason::LimitExceeded) == "LimitExceeded");
	static_assert(U(E::TerminationReason::RemovedByFilter) == 5);
	static_assert(E::EnumToString(E::TerminationReason::RemovedByFilter) == "RemovedByFilter");
	static_assert(U(E::TerminationReason::Failed) == 6);
	static_assert(E::EnumToString(E::TerminationReason::Failed) == "Failed");
	static_assert(U(E::TerminationReason::Completed) == 7);
	static_assert(E::EnumToString(E::TerminationReason::Completed) == "Completed");
	static_assert(U(E::TerminationReason::Max) == 8);
	static_assert(E::EnumToString(E::TerminationReason::Max) == "Max");

	static_assert(U(E::SendResult::Undefined) == 0);
	static_assert(E::EnumToString(E::SendResult::Undefined) == "Undefined");
	static_assert(U(E::SendResult::Success) == 1);
	static_assert(E::EnumToString(E::SendResult::Success) == "Success");
	static_assert(U(E::SendResult::Fail) == 2);
	static_assert(E::EnumToString(E::SendResult::Fail) == "Fail");
	static_assert(U(E::SendResult::Nothing) == 3);
	static_assert(E::EnumToString(E::SendResult::Nothing) == "Nothing");
	static_assert(U(E::SendResult::IOError) == 4);
	static_assert(E::EnumToString(E::SendResult::IOError) == "IOError");
	static_assert(U(E::SendResult::Max) == 5);
	static_assert(E::EnumToString(E::SendResult::Max) == "Max");

	static_assert(U(E::Stream::State::Undefined) == 0);
	static_assert(E::Stream::EnumToString(E::Stream::State::Undefined) == "Undefined");
	static_assert(U(E::Stream::State::Pending) == 1);
	static_assert(E::Stream::EnumToString(E::Stream::State::Pending) == "Pending");
	static_assert(U(E::Stream::State::Opened) == 2);
	static_assert(E::Stream::EnumToString(E::Stream::State::Opened) == "Opened");
	static_assert(U(E::Stream::State::Done) == 3);
	static_assert(E::Stream::EnumToString(E::Stream::State::Done) == "Done");
	static_assert(U(E::Stream::State::Failed) == 4);
	static_assert(E::Stream::EnumToString(E::Stream::State::Failed) == "Failed");
	static_assert(U(E::Stream::State::Closed) == 5);
	static_assert(E::Stream::EnumToString(E::Stream::State::Closed) == "Closed");
	static_assert(U(E::Stream::State::Removed) == 6);
	static_assert(E::Stream::EnumToString(E::Stream::State::Removed) == "Removed");
	static_assert(U(E::Stream::State::Max) == 7);
	static_assert(E::Stream::EnumToString(E::Stream::State::Max) == "Max");

	LOG_INFO("MSAPI UNIT TEST WebSocket events");
	MSAPI::Test::Test t;

	// Unknown values below Undefined and above Max exercise the logging fallback at runtime.
	for (const int8_t unknown : { int8_t{ -1 }, int8_t{ 127 } }) {
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::Type>(unknown)), std::string_view{ "Unknown" },
			"Unknown event type has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::HandleResult>(unknown)), std::string_view{ "Unknown" },
			"Unknown handle result has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::TerminationReason>(unknown)),
			std::string_view{ "Unknown" }, "Unknown termination reason has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::EnumToString(static_cast<E::SendResult>(unknown)), std::string_view{ "Unknown" },
			"Unknown send result has a fallback string"));
		RETURN_IF_FALSE(t.Assert(E::Stream::EnumToString(static_cast<E::Stream::State>(unknown)),
			std::string_view{ "Unknown" }, "Unknown stream state has a fallback string"));
	}

	EventsAuthorization authorization;
	std::vector<std::string> messages;
	std::vector<std::string> otherMessages;
	const auto connection{ MakeEventsConnection(messages) };
	const auto otherConnection{ MakeEventsConnection(otherMessages) };

	RETURN_IF_FALSE(t.Assert(connection != nullptr && otherConnection != nullptr, true, "Connections are created"));

	RETURN_IF_FALSE(
		WebSocketEventsInitialization(t, authorization, messages, otherMessages, connection, otherConnection));
	RETURN_IF_FALSE(WebSocketEventsRemovalAndLimits(t, authorization, messages, connection));
	RETURN_IF_FALSE(WebSocketEventsDeferredPurging(t, authorization, messages, connection));
	RETURN_IF_FALSE(WebSocketEventsTermination(t, authorization, messages, connection));
	RETURN_IF_FALSE(WebSocketEventsTransport(t, authorization));
	RETURN_IF_FALSE(WebSocketEventsDelayedTransport(t, authorization));
	RETURN_IF_FALSE(
		WebSocketEventsBuffersAndRegistration(t, authorization, messages, otherMessages, connection, otherConnection));
	RETURN_IF_FALSE(WebSocketEventsDelayedDelivery(t, authorization, messages, connection));
	RETURN_IF_FALSE(WebSocketEventsDelayedOwnership(t, authorization, messages, connection));

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI