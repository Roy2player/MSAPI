/**************************
 * @file        connection.inl
 * @date        2026-10-06
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
 * @brief Unit test for Connection.
 *
 * 1. Destructor closes descriptor and peer reads end of file;
 * 2. Only the first Close closes descriptor, destructor after Close does not close reused descriptor number.
 * 3. Send completes short writes and retries EINTR without losing progress;
 * 4. Send reports failure after partial progress and rejects further sends;
 * 5. Send rejects zero and oversized counts.
 */

#ifndef MSAPI_UNIT_TEST_CONNECTION_INL
#define MSAPI_UNIT_TEST_CONNECTION_INL

#include "../../../../library/source/server/connection.inl"
#include "../../../../library/source/test/test.inl"
#include <fcntl.h>
#include <sys/socket.h>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Connection.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool Connection();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool Connection()
{
	LOG_INFO("MSAPI UNIT TEST Connection");

	MSAPI::Test::Test t;

	// 1. Destructor closes descriptor and peer reads end of file
	{
		int32_t sockets[2]{};
		RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0, "Socket pair is created"));
		{
			const MSAPI::Connection connection{ sockets[0] };
		}
		RETURN_IF_FALSE(
			t.Assert(fcntl(sockets[0], F_GETFD) == -1 && errno == EBADF, true, "Destructor closes descriptor"));
		char byte{};
		RETURN_IF_FALSE(t.Assert(static_cast<int64_t>(read(sockets[1], &byte, 1)), 0, "Peer reads end of file"));
		(void)close(sockets[1]);
	}

	// 2. Only the first Close closes descriptor, destructor after Close does not close reused descriptor
	// number
	{
		int32_t sockets[2]{};
		RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0, "Socket pair is created"));
		auto connection{ std::make_unique<MSAPI::Connection>(sockets[0]) };
		connection->Close();
		RETURN_IF_FALSE(t.Assert(fcntl(sockets[0], F_GETFD) == -1 && errno == EBADF, true, "Close closes descriptor"));
		connection->Close();

		// The lowest free number is taken, which is the number of the closed descriptor
		const auto reused{ dup(sockets[1]) };
		RETURN_IF_FALSE(t.Assert(reused, sockets[0], "Number of closed descriptor is reused"));
		connection.reset();
		RETURN_IF_FALSE(
			t.Assert(fcntl(reused, F_GETFD) != -1, true, "Destructor after Close does not close reused descriptor"));
		(void)close(reused);
		(void)close(sockets[1]);
	}

	// 3. Short writes and EINTR preserve the byte offset, remaining size and flags.
	{
		int32_t sockets[2]{};
		RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0, "Socket pair is created"));
		MSAPI::Connection connection{ sockets[0] };
		const MSAPI::Connection peer{ sockets[1] };
		const std::string_view payload{ "abcdefgh" };
		std::string output;
		uint64_t attempts{};
		bool argumentsCorrect{ true };
		connection.SetSend([&output, &attempts, &argumentsCorrect, payload](
							   int32_t, const void* buffer, const uint64_t size, const int32_t flags) -> int64_t {
			++attempts;
			argumentsCorrect = argumentsCorrect && buffer == payload.data() + output.size()
				&& size == payload.size() - output.size() && flags == MSG_NOSIGNAL;
			if (attempts == 2) {
				errno = EINTR;
				return -1;
			}

			const uint64_t count{ size > 3 ? 3 : size };
			output.append(static_cast<const char*>(buffer), count);
			return static_cast<int64_t>(count);
		});
		RETURN_IF_FALSE(t.Assert(connection.Send(payload.data(), payload.size(), MSG_NOSIGNAL), true,
			"Send succeeds after sending all bytes"));
		RETURN_IF_FALSE(t.Assert(output, std::string{ payload }, "Short writes preserve complete payload"));
		RETURN_IF_FALSE(t.Assert(attempts, uint64_t{ 4 }, "EINTR retries remaining bytes"));
		RETURN_IF_FALSE(t.Assert(argumentsCorrect, true, "Every attempt receives correct buffer, size and flags"));
	}

	// 4. A failure after partial progress must not look like a successful send.
	{
		int32_t sockets[2]{};
		RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0, "Socket pair is created"));
		MSAPI::Connection connection{ sockets[0] };
		const MSAPI::Connection peer{ sockets[1] };
		uint64_t attempts{};
		connection.SetSend([&attempts](int32_t, const void*, uint64_t, int32_t) -> int64_t {
			if (++attempts == 1) {
				return 2;
			}

			errno = EPIPE;
			return -1;
		});
		RETURN_IF_FALSE(t.Assert(connection.Send("abcd", 4, MSG_NOSIGNAL), false, "Partial failure returns false"));
		RETURN_IF_FALSE(t.Assert(connection.Send("abcd", 4, MSG_NOSIGNAL), false, "Failed sender is inactive"));
		RETURN_IF_FALSE(t.Assert(attempts, uint64_t{ 2 }, "Inactive sender is not called again"));
	}

	// 5. Zero and counts exceeding the remaining buffer invalidate the sender.
	for (const int64_t result : { int64_t{}, int64_t{ 5 } }) {
		int32_t sockets[2]{};
		RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0, "Socket pair is created"));
		MSAPI::Connection connection{ sockets[0] };
		const MSAPI::Connection peer{ sockets[1] };
		uint64_t attempts{};
		connection.SetSend([&attempts, result](int32_t, const void*, uint64_t, int32_t) -> int64_t {
			++attempts;
			return result;
		});
		RETURN_IF_FALSE(t.Assert(connection.Send("abcd", 4, MSG_NOSIGNAL), false, "Invalid count returns false"));
		RETURN_IF_FALSE(t.Assert(connection.Send("abcd", 4, MSG_NOSIGNAL), false, "Invalid sender is inactive"));
		RETURN_IF_FALSE(t.Assert(attempts, uint64_t{ 1 }, "Invalid sender is not called again"));
	}

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_CONNECTION_INL
