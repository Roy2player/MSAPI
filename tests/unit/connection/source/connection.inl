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

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_CONNECTION_INL
