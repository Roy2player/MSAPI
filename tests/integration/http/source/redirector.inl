/**************************
 * @file        redirector.inl
 * @date        2026-10-10
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

#ifndef MSAPI_INTEGRATION_TEST_HTTP_REDIRECTOR_INL
#define MSAPI_INTEGRATION_TEST_HTTP_REDIRECTOR_INL

#include "../../../../library/source/protocol/http.h"
#include "../../../../library/source/server/server.inl"
#include "../../../../library/source/test/test.inl"
#include <chrono>
#include <format>
#include <thread>

namespace MSAPI {

namespace Test {

namespace Integration {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Receives exact response bytes through the Server receive loop and observes disconnection of its single
 * outgoing connection through the Server connection registry.
 *
 * @concurrency Yes.
 */
class RedirectClient final : public MSAPI::Server {
private:
	std::shared_ptr<MSAPI::Connection::Data> m_connection;
	std::string m_response;
	mutable MSAPI::Lock::Atomic m_responseLock;

public:
	/**************************
	 * @brief Connects to the HTTP listener on the loopback address.
	 *
	 * @param port Listener port.
	 *
	 * @locking Is not required.
	 */
	FORCE_INLINE explicit RedirectClient(uint16_t port);

	/**************************
	 * @brief Closes the connection and waits for receive callbacks to finish before destroying their state.
	 *
	 * @locking Is not required.
	 */
	FORCE_INLINE ~RedirectClient() noexcept;

	RedirectClient(const RedirectClient&) = delete;
	RedirectClient(RedirectClient&&) = delete;
	RedirectClient& operator=(const RedirectClient&) = delete;
	RedirectClient& operator=(RedirectClient&&) = delete;

	/**************************
	 * @brief Sends request bytes through the outgoing connection.
	 *
	 * @param text Request bytes.
	 *
	 * @locking Is not required.
	 *
	 * @return True if all bytes are sent.
	 */
	FORCE_INLINE [[nodiscard]] bool Send(std::string_view text) const;

	/**************************
	 * @brief Waits up to two seconds for remote disconnection and returns received bytes.
	 *
	 * @locking Lock m_responseLock inside.
	 *
	 * @return Received response.
	 */
	FORCE_INLINE [[nodiscard]] std::string Read();

	/**************************
	 * @locking Is not required.
	 *
	 * @return True if no connection was established or the receive loop removed the connection from the registry.
	 */
	FORCE_INLINE [[nodiscard]] bool IsClosed() const noexcept;

	/**************************
	 * @brief Collects raw response bytes through the standard Server receive loop.
	 *
	 * @param recvBuffer Buffer containing received response bytes.
	 *
	 * @locking Lock m_responseLock inside.
	 */
	// MSAPI::Server
	FORCE_INLINE void HandleBuffer(MSAPI::RecvBuffer& recvBuffer) final;
};

/**************************
 * @brief Selects three distinct loopback ports using temporary bound sockets and releases them before returning.
 *
 * @note Another process may claim a selected port before a redirector starts; startup detects that conflict.
 *
 * @locking Is not required.
 *
 * @return Selected ports, or zeros if socket setup fails.
 */
FORCE_INLINE [[nodiscard]] std::array<uint16_t, 3> FindRedirectPorts() noexcept;

/**************************
 * @brief Verifies exact redirect response bytes and socket closure.
 *
 * @param t Test assertions.
 * @param port Listener port.
 * @param target Request path and query.
 * @param destination Expected destination origin.
 * @param code Expected redirect status.
 *
 * @locking Locks RedirectClient::m_responseLock in RedirectClient::Read call.
 *
 * @return True if the response matches and the socket closes.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool ExpectRedirect(MSAPI::Test::Test& t, uint16_t port, std::string_view target,
	std::string_view destination, MSAPI::Protocol::HTTP::StatusCode code);

/**************************
 * @brief Tests independent redirect listeners, endpoint configuration, HTTP validation, deadlines and connection caps.
 *
 * @locking Locks RedirectClient::m_responseLock in ExpectRedirect and RedirectClient::Read calls.
 *
 * @return True if all redirector tests pass.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool RedirectorProtocol();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
RedirectClient
---------------------------------------------------------------------------------*/

FORCE_INLINE RedirectClient::RedirectClient(const uint16_t port)
{
	m_connection = MSAPI::Server::OpenConnection(INADDR_LOOPBACK, port, /*doReconnection=*/false);
}

FORCE_INLINE RedirectClient::~RedirectClient() noexcept
{
	if (m_connection == nullptr) [[unlikely]] {
		return;
	}

	m_connection->GetConnection().Close();

	// Keep callback state alive until the receive loop finishes callbacks and removes its connection.
	while (!IsClosed()) {
		std::this_thread::yield();
	}
}

FORCE_INLINE [[nodiscard]] bool RedirectClient::Send(const std::string_view text) const
{
	if (m_connection == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("{}", "HTTP test client attempted to send with no connection");
		return false;
	}

	return m_connection->GetConnection().Send(text.data(), text.size(), MSG_NOSIGNAL);
}

FORCE_INLINE [[nodiscard]] std::string RedirectClient::Read()
{
	if (m_connection == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("{}", "HTTP test client attempted to read with no connection");
		return {};
	}

	const auto deadline{ std::chrono::steady_clock::now() + std::chrono::seconds{ 2 } };
	while (!IsClosed() && std::chrono::steady_clock::now() < deadline) {
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
	}

	if (!IsClosed()) {
		LOG_ERROR_NEW("{}", "HTTP test client timed out waiting for remote disconnection");
	}

	const MSAPI::Lock::Atomic::Guard _{ m_responseLock };
	return m_response;
}

FORCE_INLINE [[nodiscard]] bool RedirectClient::IsClosed() const noexcept
{
	if (m_connection == nullptr) [[unlikely]] {
		return true;
	}

	// Read lock inside; this client owns exactly one outgoing connection and never reconnects.
	return MSAPI::Server::GetConnectionsCount() == 0;
}

FORCE_INLINE void RedirectClient::HandleBuffer(MSAPI::RecvBuffer& recvBuffer)
{
	const auto buffer{ recvBuffer.GetBuffer() };
	{
		const MSAPI::Lock::Atomic::Guard _{ m_responseLock };
		m_response.append(reinterpret_cast<const char*>(buffer.data()), buffer.size());
	}

	// To handle all the data
	recvBuffer.SetToProcessSize(1);
}

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] std::array<uint16_t, 3> FindRedirectPorts() noexcept
{
	std::array<uint16_t, 3> ports{};
	std::array<int32_t, 3> sockets{ -1, -1, -1 };
	bool isValid{ true };
	for (uint64_t index{}; index < ports.size(); ++index) {
		sockets[index] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if (sockets[index] == -1) [[unlikely]] {
			const auto error{ errno };
			LOG_ERROR_NEW("Port fixture socket failed, errno: {}, error: \"{}\"", error, std::strerror(error));
			isValid = false;
			break;
		}

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		socklen_t length{ sizeof(address) };
		if (bind(sockets[index], reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0
			|| getsockname(sockets[index], reinterpret_cast<sockaddr*>(&address), &length) != 0) [[unlikely]] {

			const auto error{ errno };
			LOG_ERROR_NEW("Port fixture bind failed, errno: {}, error: \"{}\"", error, std::strerror(error));
			isValid = false;
			break;
		}

		ports[index] = ntohs(address.sin_port);
	}

	// Keep all sockets bound until selection finishes so the operating system cannot reuse a selected port.
	for (const auto socket : sockets) {
		if (socket != -1) {
			(void)close(socket);
		}
	}

	return isValid ? ports : std::array<uint16_t, 3>{};
}

FORCE_INLINE [[nodiscard]] bool ExpectRedirect(MSAPI::Test::Test& t, const uint16_t port, const std::string_view target,
	const std::string_view destination, const MSAPI::Protocol::HTTP::StatusCode code)
{
	RedirectClient peer{ port };
	RETURN_IF_FALSE(t.Assert(
		peer.Send(std::format("GET {} HTTP/1.1\r\nHost: www.site.com\r\n\r\n", target)), true, "HTTP request sent"));

	const std::string expected{ std::format(
		"HTTP/1.1 {} {}\r\nLocation: {}{}\r\nConnection: close\r\nContent-Length: 0\r\n\r\n",
		static_cast<uint16_t>(code), MSAPI::Protocol::HTTP::EnumToString(code), destination, target) };
	RETURN_IF_FALSE(t.Assert(peer.Read(), expected, "Complete redirect bytes correct"));
	RETURN_IF_FALSE(t.Assert(peer.IsClosed(), true, "Redirect socket closes"));

	return true;
}

FORCE_INLINE [[nodiscard]] bool RedirectorProtocol()
{
	LOG_INFO_NEW("MSAPI INTEGRATION TEST: {}", "Independent HTTP redirector");
	MSAPI::Test::Test t;
	const auto ports{ FindRedirectPorts() };
	RETURN_IF_FALSE(ports[0] != 0 && ports[1] != 0 && ports[2] != 0);
	const auto first{ ports[0] };
	const auto second{ ports[1] };
	const auto rollback{ ports[2] };

	MSAPI::Protocol::HTTP::Redirector redirector;
	RETURN_IF_FALSE(t.Assert(redirector.Start(MSAPI::Protocol::HTTP::StatusCode::MovedPermanently,
								 /*receiveTimeoutMs=*/2000, /*maxConnections=*/32),
		false, "No endpoints fails"));
	RETURN_IF_FALSE(
		t.Assert(redirector.AddEndpoint("127.0.0.1", 0, "http://site.com"), false, "Zero listener port rejected"));
	RETURN_IF_FALSE(t.Assert(redirector.AddEndpoint("bad-ip", first, "http://site.com"), false, "Invalid IP fails"));
	RETURN_IF_FALSE(
		t.Assert(redirector.AddEndpoint("127.0.0.1", first, "http://site.com/path"), false, "Destination path fails"));
	RETURN_IF_FALSE(t.Assert(
		redirector.AddEndpoint("127.0.0.1", first, "https://site.com\r\nX: y"), false, "Destination injection fails"));

	RETURN_IF_FALSE(
		t.Assert(redirector.AddEndpoint("127.0.0.1", first, "http://site.com"), true, "First endpoint added"));
	RETURN_IF_FALSE(t.Assert(
		redirector.AddEndpoint("127.0.0.1", second, "https://other.site:8443"), true, "Second endpoint added"));
	RETURN_IF_FALSE(t.Assert(
		redirector.Start(MSAPI::Protocol::HTTP::StatusCode::Found, /*receiveTimeoutMs=*/2000, /*maxConnections=*/32),
		false, "Only 301 and 308 allowed"));
	RETURN_IF_FALSE(
		t.Assert(redirector.Start(MSAPI::Protocol::HTTP::StatusCode::MovedPermanently, 0, /*maxConnections=*/32), false,
			"Zero timeout fails"));
	RETURN_IF_FALSE(t.Assert(
		redirector.Start(MSAPI::Protocol::HTTP::StatusCode::MovedPermanently, 250, 33), false, "Oversized cap fails"));
	RETURN_IF_FALSE(t.Assert(
		redirector.Start(MSAPI::Protocol::HTTP::StatusCode::MovedPermanently, 250, 1), true, "Both endpoints start"));
	RETURN_IF_FALSE(
		t.Assert(first != 0 && second != 0 && first != second, true, "Distinct endpoints bind available ports"));
	RETURN_IF_FALSE(t.Assert(redirector.AddEndpoint("127.0.0.1", first, "http://late.site"), false,
		"Configuration immutable while running"));

	{
		MSAPI::Protocol::HTTP::Redirector failure;
		RETURN_IF_FALSE(
			t.Assert(failure.AddEndpoint("127.0.0.1", rollback, "http://site.com"), true, "Rollback listener added"));
		RETURN_IF_FALSE(
			t.Assert(failure.AddEndpoint("127.0.0.1", first, "http://site.com"), true, "Occupied listener added"));
		RETURN_IF_FALSE(t.Assert(failure.Start(MSAPI::Protocol::HTTP::StatusCode::MovedPermanently,
									 /*receiveTimeoutMs=*/2000, /*maxConnections=*/32),
			false, "Occupied port fails startup"));

		MSAPI::Protocol::HTTP::Redirector recovered;
		RETURN_IF_FALSE(t.Assert(
			recovered.AddEndpoint("127.0.0.1", rollback, "http://site.com"), true, "Rollback port registered again"));
		RETURN_IF_FALSE(t.Assert(recovered.Start(MSAPI::Protocol::HTTP::StatusCode::MovedPermanently,
									 /*receiveTimeoutMs=*/2000, /*maxConnections=*/32),
			true, "Earlier listener released on startup failure"));
		RETURN_IF_FALSE(ExpectRedirect(
			t, rollback, "/recovered", "http://site.com", MSAPI::Protocol::HTTP::StatusCode::MovedPermanently));
	}

	{
		MSAPI::Protocol::HTTP::Redirector full;
		for (uint64_t index{}; index < 16; ++index) {
			RETURN_IF_FALSE(
				t.Assert(full.AddEndpoint("127.0.0.1", first, "http://site.com"), true, "All endpoint slots usable"));
		}
		RETURN_IF_FALSE(
			t.Assert(full.AddEndpoint("127.0.0.1", first, "http://site.com"), false, "Endpoint cap enforced"));
	}

	RETURN_IF_FALSE(
		ExpectRedirect(t, first, "/a?b=1&c=2", "http://site.com", MSAPI::Protocol::HTTP::StatusCode::MovedPermanently));
	RETURN_IF_FALSE(ExpectRedirect(
		t, second, "/other?x", "https://other.site:8443", MSAPI::Protocol::HTTP::StatusCode::MovedPermanently));

	{
		RedirectClient idle{ first };
		RETURN_IF_FALSE(t.Assert(idle.IsClosed(), false, "Idle peer connects"));
		RETURN_IF_FALSE(t.Assert(idle.Read().empty(), true, "Peer sending no bytes gets no response"));
		RETURN_IF_FALSE(t.Assert(idle.IsClosed(), true, "Receive timeout closes peer sending no bytes"));
	}

	for (const std::string_view invalid :
		std::array<std::string_view, 6>{ std::string_view{ "\x16\x03\0\x01", 4 }, std::string_view{ "\x16\x03", 2 },
			"not HTTP data", "garbage\r\n\r\n", "GET / HTTP/9.9\r\n\r\n", "BOGUS / HTTP/1.1\r\n\r\n" }) {
		RedirectClient peer{ first };
		RETURN_IF_FALSE(t.Assert(peer.Send(invalid), true, "Invalid input sent"));
		RETURN_IF_FALSE(t.Assert(peer.Read().empty(), true, "Invalid input gets no response"));
		RETURN_IF_FALSE(t.Assert(peer.IsClosed(), true, "Invalid input closes"));
	}

	{
		RedirectClient idle{ first };
		RETURN_IF_FALSE(t.Assert(idle.Send("GET / HTTP/1.1\r\n"), true, "Incomplete header sent"));

		RedirectClient excess{ second };
		// At least one idle peer must close before either 250 ms read deadline, irrespective of accept pthread order.
		const auto deadline{ std::chrono::steady_clock::now() + std::chrono::milliseconds{ 100 } };
		while (!idle.IsClosed() && !excess.IsClosed() && std::chrono::steady_clock::now() < deadline) {
			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		}
		RETURN_IF_FALSE(
			t.Assert(idle.IsClosed() || excess.IsClosed(), true, "Global cap rejects a peer before its read deadline"));
		RETURN_IF_FALSE(t.Assert(excess.Read().empty(), true, "Excess peer gets no response"));
		RETURN_IF_FALSE(t.Assert(excess.IsClosed(), true, "Total concurrent cap enforced"));

		RETURN_IF_FALSE(t.Assert(idle.Read().empty(), true, "Incomplete header times out without response"));
		RETURN_IF_FALSE(t.Assert(idle.IsClosed(), true, "Timed out peer closes"));
	}

	{
		RedirectClient peer{ first };
		const std::string oversized{ "GET / HTTP/1.1\r\nX: " + std::string(2048, 'a') };
		RETURN_IF_FALSE(t.Assert(peer.Send(oversized), true, "Oversized header sent"));
		RETURN_IF_FALSE(t.Assert(peer.Read().empty(), true, "Oversized header gets no response"));
		RETURN_IF_FALSE(t.Assert(peer.IsClosed(), true, "2048-byte read bound enforced"));
	}

	{
		RedirectClient fragmented{ first };
		RETURN_IF_FALSE(t.Assert(
			fragmented.Send("GET /split?query HTTP/1.1\r\nHost: site.com\r\n"), true, "First header portion sent"));
		// Give the recv pthread time to consume the incomplete header before sending its terminator.
		std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });
		RETURN_IF_FALSE(t.Assert(fragmented.IsClosed(), false, "Incomplete request waits for its terminator"));
		RETURN_IF_FALSE(t.Assert(fragmented.Send("\r\n"), true, "Header terminator sent separately"));
		RETURN_IF_FALSE(t.Assert(fragmented.Read(),
			std::string{ "HTTP/1.1 301 Moved Permanently\r\nLocation: http://site.com/split?query\r\nConnection: "
						 "close\r\nContent-Length: 0\r\n\r\n" },
			"Fragmented headers redirect correctly"));
		RETURN_IF_FALSE(t.Assert(fragmented.IsClosed(), true, "Fragmented request closes"));
	}

	redirector.Stop();
	RETURN_IF_FALSE(t.Assert(redirector.Start(MSAPI::Protocol::HTTP::StatusCode::PermanentRedirect, 5000, 2), true,
		"Endpoints restart with 308"));
	RETURN_IF_FALSE(
		ExpectRedirect(t, first, "/again?q", "http://site.com", MSAPI::Protocol::HTTP::StatusCode::PermanentRedirect));
	RETURN_IF_FALSE(ExpectRedirect(
		t, second, "/again?q", "https://other.site:8443", MSAPI::Protocol::HTTP::StatusCode::PermanentRedirect));

	{
		RedirectClient idle{ first };
		RETURN_IF_FALSE(t.Assert(idle.Send("GET / HTTP/1.1\r\n"), true, "Pending peer before shutdown"));

		const auto start{ std::chrono::steady_clock::now() };
		redirector.Stop();
		RETURN_IF_FALSE(t.Assert(std::chrono::steady_clock::now() - start < std::chrono::seconds{ 1 }, true,
			"Stop interrupts long read deadline"));
		RETURN_IF_FALSE(t.Assert(idle.Read().empty(), true, "Stopped pending peer receives no response"));
		RETURN_IF_FALSE(t.Assert(idle.IsClosed(), true, "Stop closes pending peer"));
	}

	redirector.Stop();
	return t.Passed<bool>();
}

} // namespace Integration

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_INTEGRATION_TEST_HTTP_REDIRECTOR_INL
