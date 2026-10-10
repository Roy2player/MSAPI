/**************************
 * @file        server.inl
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
 * @brief Unit test for Server IP limits and connection data lifetime.
 *
 * 1.1. IP limits: connection ids are added up to the maximum, the next one is rejected;
 * 1.2. IP limits: duplicate connection id is rejected even if there is free space;
 * 1.3. IP limits: removing unknown connection id does not change the count;
 * 1.4. IP limits: invalid and unchanged maximum is rejected;
 * 1.5. IP limits: maximum less than current count keeps existing connections and rejects new ones until count is
 * below the maximum;
 * 1.6. IP limits: invalid maximum on construction keeps default maximum 1;
 * 2.1. Server: connections from one IP are registered up to default limit 5, the next one is denied;
 * 2.2. Server: connections from another IP are limited independently;
 * 2.3. Server: unregistered connection frees space for a new one, duplicate connection id is denied;
 * 2.4. Server: unregistering from unknown IP and unknown connection id does not change limits;
 * 2.5. Server: changed limit of connections from one IP is applied to a new IP;
 * 2.6. Server: IP limits are kept when the last connection is unregistered;
 * 3.1. Server: income connection data is released after peer closes connection and pthread recv loop is finished,
 * connection is removed from server and IP limits;
 * 4.1. Pthread attributes: attributes are valid after construction and have detached state.
 */

#ifndef MSAPI_UNIT_TEST_SERVER_INL
#define MSAPI_UNIT_TEST_SERVER_INL

#include "../../../../library/source/server/server.inl"
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
 * @brief Provides direct access to Server IP limits for unit testing.
 *
 * @concurrency No.
 */
class ServerObserver {
public:
	using IpLimits = MSAPI::Server::IpLimits;

private:
	MSAPI::Server& m_server;

public:
	/**************************
	 * @param server Observed server, outlives the observer.
	 */
	FORCE_INLINE explicit ServerObserver(MSAPI::Server& server) noexcept;

	ServerObserver(const ServerObserver&) = delete;
	ServerObserver(ServerObserver&&) = delete;
	ServerObserver& operator=(const ServerObserver&) = delete;
	ServerObserver& operator=(ServerObserver&&) = delete;

	/**************************
	 * @brief Register connection from IP via Server::RegisterConnectionFromIp.
	 *
	 * @param id Connection id.
	 * @param ip IP address.
	 *
	 * @return True if connection is registered, false otherwise or if IP does not fit into static string.
	 */
	FORCE_INLINE [[nodiscard]] bool RegisterConnectionFromIp(uint64_t id, std::string_view ip) noexcept;

	/**************************
	 * @brief Unregister connection from IP via Server::UnregisterConnectionFromIp.
	 *
	 * @param id Connection id.
	 * @param ip IP address.
	 */
	FORCE_INLINE void UnregisterConnectionFromIp(uint64_t id, std::string_view ip) noexcept;

	/**************************
	 * @param ip IP address.
	 *
	 * @return Count of connections registered from IP, empty if there are no limits for IP.
	 */
	FORCE_INLINE [[nodiscard]] std::optional<uint64_t> GetConnectionsCountFromIp(std::string_view ip) const noexcept;

	/**************************
	 * @return Count of IPs with limits.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetIpLimitsCount() const noexcept;

	/**************************
	 * @brief Set limit of connections from one IP, as parameter 1000003 modification does.
	 *
	 * @param value New limit.
	 */
	FORCE_INLINE void SetMaxConnectionsOneIp(uint64_t value) noexcept;

	/**************************
	 * @brief Create income connection with pthread recv loop via Server::CreatePthread.
	 *
	 * @param socket Connected socket, ownership is transferred to the connection.
	 * @param ip IP address.
	 *
	 * @return Connection data on success, nullptr otherwise.
	 */
	FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> CreateIncomeConnection(
		int32_t socket, std::string_view ip) noexcept;

	/**************************
	 * @return Count of server connections.
	 */
	FORCE_INLINE [[nodiscard]] uint64_t GetConnectionsCount() const noexcept;
};

/**************************
 * @brief Unit test for Server IP limits and connection data lifetime.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool Server();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE ServerObserver::ServerObserver(MSAPI::Server& server) noexcept
	: m_server{ server }
{
}

FORCE_INLINE [[nodiscard]] bool ServerObserver::RegisterConnectionFromIp(
	const uint64_t id, const std::string_view ip) noexcept
{
	SString<16> ipStr;
	if (!ipStr.Copy(ip)) [[unlikely]] {
		return false;
	}

	// Friend access
	return m_server.RegisterConnectionFromIp(id, ipStr);
}

FORCE_INLINE void ServerObserver::UnregisterConnectionFromIp(const uint64_t id, const std::string_view ip) noexcept
{
	// Friend access
	m_server.UnregisterConnectionFromIp(id, ip);
}

FORCE_INLINE [[nodiscard]] std::optional<uint64_t> ServerObserver::GetConnectionsCountFromIp(
	const std::string_view ip) const noexcept
{
	std::shared_ptr<IpLimits> ipLimits;
	{
		// Friend access
		const Lock::AtomicRW::Guard<Lock::READ> _{ m_server.m_ipToLimitsRWLock };
		const auto it{ m_server.m_ipToLimits.find(std::hash<std::string_view>{}(ip)) };
		if (it == m_server.m_ipToLimits.end()) {
			return {};
		}

		ipLimits = it->second;
	}

	const Lock::AtomicRW::Guard<Lock::READ> _{ ipLimits->GetLock() };
	return ipLimits->GetConnectionsCount();
}

FORCE_INLINE [[nodiscard]] uint64_t ServerObserver::GetIpLimitsCount() const noexcept
{
	// Friend access
	const Lock::AtomicRW::Guard<Lock::READ> _{ m_server.m_ipToLimitsRWLock };
	return m_server.m_ipToLimits.size();
}

FORCE_INLINE void ServerObserver::SetMaxConnectionsOneIp(const uint64_t value) noexcept
{
	// Friend access
	m_server.m_maxConnectionsOneIp = value;
}

FORCE_INLINE [[nodiscard]] std::shared_ptr<Connection::Data> ServerObserver::CreateIncomeConnection(
	const int32_t socket, const std::string_view ip) noexcept
{
	SString<16> ipStr;
	if (!ipStr.Copy(ip)) [[unlikely]] {
		return {};
	}

	// Friend access
	return m_server.CreatePthread<Connection::Type::Income>(
		std::make_unique<Connection>(socket), std::move(ipStr), /*ip=*/0, /*port=*/0, /*doReconnection=*/false);
}

FORCE_INLINE [[nodiscard]] uint64_t ServerObserver::GetConnectionsCount() const noexcept
{
	// Friend access
	return m_server.GetConnectionsCount();
}

FORCE_INLINE [[nodiscard]] bool Server()
{
	LOG_INFO("MSAPI UNIT TEST Server");

	MSAPI::Test::Test t;
	using IpLimits = ServerObserver::IpLimits;

	{
		IpLimits ipLimits{ 3 };
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ ipLimits.GetLock() };

		// 1.1. IP limits: connection ids are added up to the maximum, the next one is rejected
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 0, "IP limits are empty after construction"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(1), true, "First connection id is added"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(2), true, "Second connection id is added"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(3), true, "Third connection id is added"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(4), false, "Connection id over maximum is rejected"));
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 3, "Count is equal to maximum"));

		// 1.2. IP limits: duplicate connection id is rejected even if there is free space
		ipLimits.RemoveConnectionId(3);
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 2, "Count after removal is expected"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(2), false, "Duplicate connection id is rejected"));
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 2, "Count after duplicate is not changed"));

		// 1.3. IP limits: removing unknown connection id does not change the count
		ipLimits.RemoveConnectionId(100);
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 2, "Count after unknown removal is not changed"));

		// 1.4. IP limits: invalid and unchanged maximum is rejected
		RETURN_IF_FALSE(t.Assert(ipLimits.SetMaxConnections(0), false, "Zero maximum is rejected"));
		RETURN_IF_FALSE(t.Assert(ipLimits.SetMaxConnections(3), false, "Unchanged maximum is rejected"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(3), true, "Maximum is kept after rejected changes"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(4), false, "Connection id over kept maximum is rejected"));

		// 1.5. IP limits: maximum less than current count keeps existing connections and rejects new ones
		RETURN_IF_FALSE(t.Assert(ipLimits.SetMaxConnections(1), true, "Minimum maximum is accepted"));
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 3, "Existing connections are kept"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(4), false, "Connection id over lowered maximum is rejected"));
		ipLimits.RemoveConnectionId(1);
		ipLimits.RemoveConnectionId(2);
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(4), false, "Connection id at lowered maximum is rejected"));
		ipLimits.RemoveConnectionId(3);
		RETURN_IF_FALSE(t.Assert(ipLimits.GetConnectionsCount(), 0, "All connections are removed"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(4), true, "Connection id below lowered maximum is added"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(5), false, "Second connection id is rejected"));
	}

	// 1.6. IP limits: invalid maximum on construction keeps default maximum 1
	{
		IpLimits ipLimits{ 0 };
		const Lock::AtomicRW::Guard<Lock::WRITE> _{ ipLimits.GetLock() };
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(1), true, "Connection id within default maximum is added"));
		RETURN_IF_FALSE(t.Assert(ipLimits.AddConnectionId(2), false, "Connection id over default maximum is rejected"));
	}

	MSAPI::Server server;
	ServerObserver observer{ server };
	constexpr std::string_view FIRST_IP{ "127.0.0.1" };
	constexpr std::string_view SECOND_IP{ "10.0.0.1" };
	constexpr std::string_view THIRD_IP{ "192.168.100.100" };

	// 2.1. Server: connections from one IP are registered up to default limit 5, the next one is denied
	RETURN_IF_FALSE(t.Assert(observer.GetIpLimitsCount(), 0, "No IP limits before registration"));
	for (uint64_t id{ 1 }; id <= 5; ++id) {
		RETURN_IF_FALSE(t.Assert(observer.RegisterConnectionFromIp(id, FIRST_IP), true,
			std::format("Connection id: {} from first IP is registered", id)));
	}
	RETURN_IF_FALSE(t.Assert(
		observer.RegisterConnectionFromIp(6, FIRST_IP), false, "Connection over limit from first IP is denied"));
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(FIRST_IP), std::optional<uint64_t>{ 5 },
		"Count of connections from first IP is limit"));
	RETURN_IF_FALSE(t.Assert(observer.GetIpLimitsCount(), 1, "IP limits are created for first IP"));

	// 2.2. Server: connections from another IP are limited independently
	RETURN_IF_FALSE(
		t.Assert(observer.RegisterConnectionFromIp(6, SECOND_IP), true, "Connection from second IP is registered"));
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(SECOND_IP), std::optional<uint64_t>{ 1 },
		"Count of connections from second IP is expected"));
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(FIRST_IP), std::optional<uint64_t>{ 5 },
		"Count of connections from first IP is not changed"));
	RETURN_IF_FALSE(t.Assert(observer.GetIpLimitsCount(), 2, "IP limits are created for second IP"));

	// 2.3. Server: unregistered connection frees space for a new one, duplicate connection id is denied
	observer.UnregisterConnectionFromIp(5, FIRST_IP);
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(FIRST_IP), std::optional<uint64_t>{ 4 },
		"Count of connections from first IP after unregistration is expected"));
	RETURN_IF_FALSE(t.Assert(
		observer.RegisterConnectionFromIp(1, FIRST_IP), false, "Duplicate connection id from first IP is denied"));
	RETURN_IF_FALSE(t.Assert(
		observer.RegisterConnectionFromIp(7, FIRST_IP), true, "Connection from first IP is registered in freed space"));
	RETURN_IF_FALSE(t.Assert(
		observer.RegisterConnectionFromIp(8, FIRST_IP), false, "Connection over limit from first IP is denied"));

	// 2.4. Server: unregistering from unknown IP and unknown connection id does not change limits
	observer.UnregisterConnectionFromIp(1, THIRD_IP);
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(THIRD_IP), std::optional<uint64_t>{},
		"IP limits are not created on unregistration from unknown IP"));
	RETURN_IF_FALSE(t.Assert(observer.GetIpLimitsCount(), 2, "Count of IP limits is not changed"));
	observer.UnregisterConnectionFromIp(100, FIRST_IP);
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(FIRST_IP), std::optional<uint64_t>{ 5 },
		"Count of connections from first IP after unknown id unregistration is not changed"));

	// 2.5. Server: changed limit of connections from one IP is applied to a new IP
	observer.SetMaxConnectionsOneIp(2);
	RETURN_IF_FALSE(
		t.Assert(observer.RegisterConnectionFromIp(9, THIRD_IP), true, "First connection from third IP is registered"));
	RETURN_IF_FALSE(t.Assert(
		observer.RegisterConnectionFromIp(10, THIRD_IP), true, "Second connection from third IP is registered"));
	RETURN_IF_FALSE(t.Assert(observer.RegisterConnectionFromIp(11, THIRD_IP), false,
		"Connection over changed limit from third IP is denied"));
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(THIRD_IP), std::optional<uint64_t>{ 2 },
		"Count of connections from third IP is changed limit"));

	// 2.6. Server: IP limits are kept when the last connection is unregistered
	observer.UnregisterConnectionFromIp(6, SECOND_IP);
	RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(SECOND_IP), std::optional<uint64_t>{ 0 },
		"IP limits are kept on unregistration of the last connection"));
	RETURN_IF_FALSE(t.Assert(observer.GetIpLimitsCount(), 3, "Count of IP limits is not changed"));

	// 3.1. Server: income connection data is released after peer closes connection and pthread recv loop is finished
	{
		constexpr std::string_view FOURTH_IP{ "172.16.0.1" };
		int32_t sockets[2]{};
		RETURN_IF_FALSE(t.Assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0, "Socket pair is created"));

		std::weak_ptr<Connection::Data> weakConnectionData;
		{
			const auto connectionData{ observer.CreateIncomeConnection(sockets[0], FOURTH_IP) };
			RETURN_IF_FALSE(t.Assert(connectionData != nullptr, true, "Income connection is created"));
			weakConnectionData = connectionData;
		}
		RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCount(), 1, "Connection is saved in server"));
		RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(FOURTH_IP), std::optional<uint64_t>{ 1 },
			"Connection is registered in IP limits"));
		RETURN_IF_FALSE(t.Assert(
			weakConnectionData.expired(), false, "Connection data is alive while pthread recv loop is running"));

		// Peer closing finishes the pthread recv loop
		(void)close(sockets[1]);
		RETURN_IF_FALSE(t.Wait(
			1000000, [&weakConnectionData]() { return weakConnectionData.expired(); }, true,
			"Connection data is released after pthread recv loop is finished"));
		RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCount(), 0, "Connection is removed from server"));
		RETURN_IF_FALSE(t.Assert(observer.GetConnectionsCountFromIp(FOURTH_IP), std::optional<uint64_t>{ 0 },
			"Connection is unregistered from IP limits"));
		RETURN_IF_FALSE(t.Assert(fcntl(sockets[0], F_GETFD) == -1 && errno == EBADF, true, "Descriptor is closed"));
	}

	// 4.1. Pthread attributes: attributes are valid after construction and have detached state
	{
		const MSAPI::Server::PthreadAttributes attributes;
		RETURN_IF_FALSE(t.Assert(attributes.IsValid(), true, "Pthread attributes are valid after construction"));
		int32_t detachState{};
		RETURN_IF_FALSE(t.Assert(
			pthread_attr_getdetachstate(&attributes.Get(), &detachState), 0, "Detach state of attributes is read"));
		RETURN_IF_FALSE(t.Assert(detachState, PTHREAD_CREATE_DETACHED, "Pthread attributes have detached state"));
	}

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_SERVER_INL
