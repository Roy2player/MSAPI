/**************************
 * @file        server.cpp
 * @version     6.0
 * @date        2023-12-11
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#include "server.h"
#include "../help/autoClearPtr.hpp"
#include "../help/diagnostic.h"
#include "../help/identifier.h"
#include <climits>
#include <fcntl.h>
#include <iomanip>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Server
---------------------------------------------------------------------------------*/

Server::Server()
{
	static_assert(CHAR_BIT == 8, "CHAR_BIT is not 8");

	RegisterParameter(1000001, { "Seconds between try to connect", &m_secondsBetweenTryToConnect, 1 });
	RegisterParameter(1000002, { "Limit of attempts to connection", &m_limitConnectAttempts, 1 });
	RegisterParameter(1000003, { "Limit of connections from one IP", &m_maxConnectionsOneIp, 1 });
	RegisterParameter(1000004, { "Recv buffer size", &recvBufferSize, 3 });
	RegisterParameter(1000005, { "Recv buffer size limit", &recvBufferSizeLimit, 1024 });
	RegisterConstParameter(1000006, { "Server state", &m_state, &EnumToString });
	RegisterConstParameter(1000007, { "Max connections", &m_somaxconn });
	RegisterConstParameter(1000008, { "Listening IP", &m_listeningIp });
	RegisterConstParameter(1000009, { "Listening port", &m_listeningPort });
}

Server::~Server()
{
	Stop();
	m_alivePthreadsRWLock.WriteLock();
	m_closingConnectionLocks.Lock();
	m_serverDestroyLock.Lock();
}

void Server::HandleRunRequest() { MSAPI_HANDLE_RUN_REQUEST_PRESET; }

void Server::HandlePauseRequest() { MSAPI_HANDLE_PAUSE_REQUEST_PRESET; }

void Server::HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	MSAPI_HANDLE_MODIFY_REQUEST_PRESET
}

void Server::HandleDeleteRequest() { Stop(); }

std::string Server::GetIp(const int connection) const
{
	const auto it{ m_IpToConnection.find(connection) };
	if (it == m_IpToConnection.end()) [[unlikely]] {
		return "";
	}
	return it->second;
}

void Server::Start(const in_addr_t ip, const in_port_t port)
{
	if (m_state != State::Initialization) [[unlikely]] {
		LOG_DEBUG_NEW("Server is not in initialization state and cannot be started, current state is {}", EnumToString(m_state));
		return;
	}

	m_addr.sin_addr.s_addr = htonl(ip);
	m_listeningPort = port;
	m_listeningIp = Helper::GetStringIp(m_addr);
	m_addr.sin_port = htons(port);
	m_addr.sin_family = AF_INET;

	LOG_INFO("Starting server, IP: " + Helper::GetStringIp(m_addr) + ", port: " + _S(port));

	bool socketListenCheck;
	AutoFreeSocket socketListen{ Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, socketListenCheck), &socketListenCheck };
	if (!socketListenCheck) [[unlikely]] {
		LOG_ERROR("Force stop. Socket constructor error");
		Stop();
		return;
	}
	m_socketListen = &socketListen;

	if (!Bind(socketListen.socket, &m_addr)) [[unlikely]] {
		LOG_ERROR("Force stop. Bind constructor throw");
		Stop();
		return;
	}
	if (!Listen(socketListen.socket)) [[unlikely]] {
		LOG_ERROR("Force stop. Listen constructor throw");
		Stop();
		return;
	}
	LOG_INFO("Successfully server start");
	m_state = State::Running;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	// The minimum pthread stack is only POSIX requirement, which does not takes into additional requirements, like
	// guard page, bookkeeping/padding and god knows what else.
	// As a side effect, not only pthread_create can return EAGAIN, but also pthread can crashes due to wrong mangling!
	// pthread_attr_setstacksize(&attr, UINT64(2 * PTHREAD_STACK_MIN));
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	MSAPI::Pthread::AtomicLock::ExitGuard exitGuard{ m_serverDestroyLock };

	for (const auto& [id, info] : m_infoToConnection) {
		StandardProtocol::SendActionHello(info.connection);
	}

	sockaddr_in clientAddr{ 0, 0, 0, 0 };
	do {
		std::optional<int> newConnection;
		while (m_connectionsCounter < m_somaxconn && m_state != State::Stopped) {
			newConnection = Accept(socketListen.socket, &clientAddr);
			if (m_state == State::Stopped) [[unlikely]] {
				pthread_attr_destroy(&attr);
				return;
			}
			if (m_state != State::Running) [[unlikely]] {
				LOG_DEBUG_NEW("Server state is {}, continue to accept new connections", EnumToString(m_state));
				continue;
			}
			if (!newConnection.has_value()) [[unlikely]] {
				continue;
			}

			MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_closingConnectionLocks };

			std::string ip{ Helper::GetStringIp(clientAddr) };
			const auto& connection = newConnection.value();

			int id;
			do {
				id = static_cast<int>(Identifier::mersenne());
			} while (m_connectionToId.find(id) != m_connectionToId.end()
				|| m_infoToConnection.find(id) != m_infoToConnection.end());

			m_connectionToId.insert({ id, connection });
			m_IpToConnection.insert({ connection, ip });
			if (!IsConnectionAllowed(id, ip)) {
				m_connectionToId.erase(id);
				m_IpToConnection.erase(connection);
				if (shutdown(connection, SHUT_RDWR) == -1) [[unlikely]] {
					LOG_ERROR("Connection " + _S(connection) + " shutdown is failed, id: " + _S(id) + ". Error №"
						+ _S(errno) + ": " + std::strerror(errno));
				}
				if (close(connection) == -1) [[unlikely]] {
					LOG_ERROR("Connection " + _S(connection) + " close is failed, id: " + _S(id) + ". Error №"
						+ _S(errno) + ": " + std::strerror(errno));
				}
				continue;
			}

			++m_connectionsCounter;
			const auto [pair, status] = m_pthreadToId.emplace(id, pthread_t{});
			LOG_INFO("Connect successfully, id: " + _S(id));

			auto it = m_dataToPthreads.insert({ id, { this, &m_connectionToId.find(id)->first } });
		pthreadCreate:
			if (const auto result{ pthread_create(&pair->second, &attr, PthreadRunner<RecvProcessingType::Income>,
					static_cast<void*>(&(it.first->second))) };
				result != 0) [[unlikely]] {

				LOG_ERROR(
					"Pthread is not created, id: " + _S(id) + ". Error №" + _S(result) + ": " + std::strerror(result));

				if (result == EAGAIN) {
					goto pthreadCreate;
				}

				m_pthreadToId.erase(id);
				Close(id, connection);
			}
			else {
				LOG_DEBUG("Pthread is created successfully, id: " + _S(id));
			}
		}

		if (m_state == State::Stopped) {
			pthread_attr_destroy(&attr);
			LOG_DEBUG("Server state is Stopped. Return from the main accepting loop");
			return;
		}

		LOG_INFO("Server can't accept new connection, limit: " + _S(m_somaxconn) + " reached. Sleep for 10 seconds");
		std::this_thread::sleep_for(std::chrono::seconds(10));
	} while (m_somaxconn >= m_connectionsCounter);

	pthread_attr_destroy(&attr);
	LOG_ERROR_NEW("Unexpected exit from the main accepting loop, server state is {}, connections counter is {}",
		EnumToString(m_state), m_connectionsCounter);
}

void Server::Stop()
{
	if (m_state == State::Stopped) [[unlikely]] {
		LOG_DEBUG("Server is already stopped");
		return;
	}

	LOG_INFO("Server is stopping");
	m_state = State::Stopped;

	MSAPI::Pthread::AtomicLock::ExitGuard exitGuard{ m_closingConnectionLocks };

	if (m_socketListen != nullptr && m_socketListen->socketCheck != nullptr && *m_socketListen->socketCheck) {
		m_socketListen->socketCheck = nullptr;
		if (shutdown(m_socketListen->socket, SHUT_RDWR) == -1) [[unlikely]] {
			LOG_ERROR("Listen socket shutdown is failed. Error №" + _S(errno) + ": " + std::strerror(errno));
		}
		if (close(m_socketListen->socket) == -1) [[unlikely]] {
			LOG_ERROR("Listen socket close is failed. Error №" + _S(errno) + ": " + std::strerror(errno));
		}
	}

	//* Close all income connections
	{
		std::map<int, int>::iterator copyIt;
		for (auto it{ m_connectionToId.begin() }; it != m_connectionToId.end();) {
			copyIt = it;
			++it;
			Close(copyIt->first, copyIt->second);
		}
	}

	//* Close all outcome connections
	{
		std::map<int, Server::ConnectionInfo>::iterator copyIt;
		for (auto it{ m_infoToConnection.begin() }; it != m_infoToConnection.end();) {
			copyIt = it;
			++it;
			CloseConnect(copyIt->second);
		}
	}

	LOG_INFO("Server stopped");
}

int Server::Socket(const int domain, const int type, const int protocol, bool& status)
{
	const int socketListen{ socket(domain, type, protocol) };
	if (socketListen == -1) [[unlikely]] {
		LOG_ERROR("Socket is not opened");
		status = false;
		return -1;
	}
	LOG_DEBUG("Socket is opened successfully");
	status = true;

	{
		int enable{ 1 };
		/*
			This socket option tells the kernel to reuse a local socket in TIME_WAIT state, without waiting for its
			natural timeout to expire. If you're developing a server, setting this option can be useful, because it
			allows the server to restart without waiting for the timeout to expire when it has been shut down.
		*/
		if (setsockopt(socketListen, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) [[unlikely]] {
			LOG_ERROR("Failed to set SO_REUSEADDR option to socket");
		}
	}
#ifdef SO_REUSEPORT
	{
		int enable{ 0 };
		/*
			This is a more recent addition that allows multiple sockets on the same host to bind to the same port. This
			can be useful for programs that want to do multicast or need to have multiple processes listening on the
			same port. Note that this option is not available on all systems, which is why it's wrapped in an #ifdef in
			your code.
		*/
		if (setsockopt(socketListen, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) [[unlikely]] {
			LOG_ERROR("Failed to set SO_REUSEPORT option to socket");
		}
	}
#endif
	{
		int enable{ 1 };
		/*
			This option is used to control the Nagle's algorithm for a socket. When enabled (set to 1), the algorithm is
			disabled and the TCP stack will send out small packets without waiting to see if more data is coming that
			could be included in the packets. This can reduce latency but may increase bandwidth usage.
		*/
		if (setsockopt(socketListen, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) [[unlikely]] {
			LOG_ERROR("Failed to set TCP_NODELAY option to socket");
		}
	}

	return socketListen;
}

bool Server::Bind(const int socket, const sockaddr_in* addr)
{
	if (bind(socket, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in)) == -1) [[unlikely]] {
		LOG_ERROR("Socket is not bound. Error №" + _S(errno) + ": " + std::strerror(errno));
		return false;
	}
	LOG_DEBUG("Socket is bound successfully, connection limit: " + _S(m_somaxconn));
	return true;
}

bool Server::Listen(const int socket)
{
	if (listen(socket, m_somaxconn) == -1) [[unlikely]] {
		LOG_ERROR("Socket is not listened. Error №" + _S(errno) + ": " + std::strerror(errno));
		return false;
	}
	LOG_DEBUG("Socket is listened successfully");
	return true;
}

std::optional<int> Server::Accept(const int socket, sockaddr_in* addr)
{
	const int res{ accept(socket, reinterpret_cast<sockaddr*>(addr), &m_sizeAddr) };
	if (res == -1) [[unlikely]] {
		if (m_state == State::Stopped) {
			LOG_DEBUG("Socket accepting is interrupted, server state is Stopped");
			return {};
		}
		LOG_ERROR("Socket accepting is interrupted. Error №" + _S(errno) + ": " + std::strerror(errno));
		return {};
	}
	LOG_DEBUG("Socket is accepted successfully");
	return res;
}

bool Server::Connect(const int socket, const sockaddr_in* addr)
{
	if (m_connectionsCounter >= m_somaxconn) [[unlikely]] {
		LOG_WARNING("Maximum queue length of listening is full: " + _S(m_connectionsCounter) + "/" + _S(m_somaxconn));
		return false;
	}
	if (connect(socket, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in)) != 0) [[unlikely]] {
		LOG_ERROR("Socket is not connected. Error №" + _S(errno) + ": " + std::strerror(errno));
		return false;
	}
	LOG_INFO("Socket is connected successfully");
	return true;
}

void Server::Close(const int id, const int connection)
{
	LOG_INFO("Closing connection id: " + _S(id));

	if (shutdown(connection, SHUT_RDWR) == -1) [[unlikely]] {
		if (errno == ENOTCONN) {
			LOG_DEBUG("Connection " + _S(connection) + " is already closed, id: " + _S(id));
		}
		else {
			LOG_ERROR("Connection " + _S(connection) + " shutdown is failed, id: " + _S(id) + ". Error №" + _S(errno)
				+ ": " + std::strerror(errno));
		}
	}
	else if (close(connection) == -1) [[unlikely]] {
		LOG_ERROR("Connection " + _S(connection) + " close is failed, id: " + _S(id) + ". Error №" + _S(errno) + ": "
			+ std::strerror(errno));
	}

	m_connectionToId.erase(id);

	if (const auto IpIt{ m_IpToConnection.find(connection) }; IpIt != m_IpToConnection.end()) {
		const std::string& ip{ IpIt->second };
		--m_connectionsCounter;
		LOG_INFO("Successfully closed connection id: " + _S(id) + ", IP: " + ip + ". Active connections counter is "
			+ _S(m_connectionsCounter));

		//* Filter flow, control IP
		if (const auto idsIt{ m_connectionsToIp.find(ip) }; idsIt != m_connectionsToIp.end()) [[likely]] {
			for (auto it{ idsIt->second.begin() }; it != idsIt->second.end(); ++it) {
				if (*it == id) {
					LOG_INFO("IP Filter module: Erase an IP: " + ip + ", id: " + _S(id));
					idsIt->second.erase(it);
					break;
				}
			}
		}
		else {
			LOG_WARNING("Don't find connections related to IP: " + ip);
		}
		m_IpToConnection.erase(connection);

		return;
	}

	bool needReconnection{ false };
	std::pair<in_addr_t, in_port_t> connectionData;
	if (const auto it{ m_infoToConnection.find(id) }; it != m_infoToConnection.end()) [[likely]] {
		const std::string& ip{ it->second.textIp };

		if (needReconnection = it->second.needReconnection; m_state != State::Stopped && needReconnection) {
			LOG_INFO("Reconnecting is required to outcome connection id: " + _S(id) + ", IP: " + ip);
			connectionData = { it->second.ip, it->second.port };
		}
		//* If server stops during reconnection, connecting process will be interrupted
		--m_connectionsCounter;
		LOG_INFO("Successfully close outcome connection id: " + _S(id) + ", IP: " + ip
			+ ". Active connections counter is " + _S(m_connectionsCounter));
		m_infoToConnection.erase(id);
	}
	else {
		LOG_WARNING("Don't find an IP to connection id: " + _S(id));
	}

	if (needReconnection) {
		//* It is possible only when connection is disconnected by other side unexpectedly and will block only
		//* RecvProcessing pthread, which is not a problem
		std::this_thread::sleep_for(std::chrono::seconds(m_secondsBetweenTryToConnect));
		if (OpenConnect(id, connectionData.first, connectionData.second)) {
			HandleReconnect(id);
		}
	}
}

bool Server::IsConnectionAllowed(const int id, const std::string& ip)
{
	//* Filter flow, control IP
	const auto connectionsIt{ m_connectionsToIp.find(ip) };
	if (connectionsIt == m_connectionsToIp.end() || connectionsIt->second.size() == 0) {
		LOG_INFO("IP Filter module: This is unique connection, IP: " + ip + ", id: " + _S(id));
		m_connectionsToIp[ip] = { id };
		return true;
	}

	size_t size{ connectionsIt->second.size() };
	LOG_INFO("IP Filter module: This is not unique connection, now: " + _S(size) + " connection(s), IP: " + ip
		+ ", current id: " + _S(id));

	if (size >= m_maxConnectionsOneIp) {
		LOG_INFO("IP Filter module: Close the connection due to limit, IP: " + ip + ", id: " + _S(id));
		return false;
	}

	connectionsIt->second.push_back(id);
	return true;
}

bool Server::OpenConnect(const int id, const in_addr_t ip, const in_port_t port, const bool needReconnection)
{
	if (m_state == State::Stopped) [[unlikely]] {
		LOG_INFO("Connecting process is interrupted, because of server is stopped, id: " + _S(id)
			+ ", port: " + _S(static_cast<uint>(port)));
		return false;
	}

	LOG_INFO("Connecting to id: " + _S(id) + ", port: " + _S(static_cast<uint>(port))
		+ ", reconnection: " + _S(needReconnection));

	sockaddr_in addr{ 0, 0, 0, 0 };
	addr.sin_addr.s_addr = htonl(ip);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	bool newConnectionCheck;
	const int newConnection{ Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, newConnectionCheck) };
	if (!newConnectionCheck) [[unlikely]] {
		LOG_ERROR("Connect to id: " + _S(id) + ", port: " + _S(static_cast<uint>(port)) + " is failed");
		return false;
	}

	size_t attempt{ 1 };
	while (!Connect(newConnection, &addr)) {
		std::this_thread::sleep_for(std::chrono::seconds(m_secondsBetweenTryToConnect));
		if (attempt++ >= m_limitConnectAttempts) {
			LOG_WARNING("Limit of connect attempts (" + _S(m_limitConnectAttempts) + ") is reached, id: " + _S(id)
				+ ", port: " + _S(static_cast<uint>(port)));
			return false;
		}
		if (m_state == State::Stopped) {
			LOG_INFO("Connecting process is interrupted, because of server is stopped, id: " + _S(id)
				+ ", port: " + _S(static_cast<uint>(port)));
			return false;
		}
	}

	const std::string textIp{ Helper::GetStringIp(addr) };

	const auto [pair, status]
		= m_infoToConnection.emplace(id, ConnectionInfo{ id, ip, port, newConnection, textIp, needReconnection });
	if (!status) [[unlikely]] {
		LOG_ERROR("Abort connecting. Duplicated id: " + _S(id) + ", IP: " + textIp);
		if (shutdown(newConnection, SHUT_RDWR) == -1) [[unlikely]] {
			LOG_ERROR("Connection " + _S(newConnection) + " shutdown is failed, id: " + _S(id) + ". Error №" + _S(errno)
				+ ": " + std::strerror(errno));
		}
		if (close(newConnection) == -1) [[unlikely]] {
			LOG_ERROR("Connection " + _S(newConnection) + " close is failed, id: " + _S(id) + ". Error №" + _S(errno)
				+ ": " + std::strerror(errno));
		}
		return false;
	}
	const int* saveId = &pair->first;
	LOG_INFO("Successfully open new connection id: " + _S(*saveId) + ", IP: " + textIp
		+ ", port: " + _S(static_cast<uint>(port)));

	const auto [idAndThread, insertStatus] = m_pthreadToId.emplace(*saveId, pthread_t{});

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	// The minimum pthread stack is only POSIX requirement, which does not takes into additional requirements, like
	// guard page, bookkeeping/padding and god knows what else.
	// pthread_attr_setstacksize(&attr, UINT64(2 * PTHREAD_STACK_MIN));
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	//? pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); Cant use it, because of SCHED_RR, but why?

	++m_connectionsCounter;
	auto it = m_dataToPthreads.insert({ *saveId, { this, saveId } });
pthreadCreate:
	if (const auto result{ pthread_create(&idAndThread->second, &attr,
			*saveId == 0 ? PthreadRunner<RecvProcessingType::Manager> : PthreadRunner<RecvProcessingType::Outcome>,
			static_cast<void*>(&(it.first->second))) };
		result != 0) [[unlikely]] {

		LOG_ERROR(
			"Pthread is not created, id: " + _S(*saveId) + ". Error №" + _S(result) + ": " + std::strerror(result));

		if (result == EAGAIN) {
			goto pthreadCreate;
		}

		pair->second.needReconnection = false;
		Close(*saveId, newConnection);
		m_pthreadToId.erase(idAndThread);
		pthread_attr_destroy(&attr);
		return false;
	}

	pthread_attr_destroy(&attr);
	LOG_DEBUG("Pthread is created successfully, id: " + _S(*saveId));

	if (m_state == State::Running) {
		StandardProtocol::SendActionHello(newConnection);
	}

	return true;
}

bool Server::ConnectIsOpen(const int id) { return m_infoToConnection.find(id) != m_infoToConnection.end(); }

void Server::CloseConnect(const int id)
{
	if (const auto connectionIt = m_infoToConnection.find(id); connectionIt != m_infoToConnection.end()) [[likely]] {
		CloseConnect(connectionIt->second);
		return;
	}

	LOG_WARNING("Connection is not found, id: " + _S(id));
}

void Server::CloseConnect(ConnectionInfo& info)
{
	info.needReconnection = false;
	LOG_INFO("Closing connection to id: " + _S(info.id) + ". Reconnection is disabled");
	Close(info.id, info.connection);
}

#define TMP_MSAPI_SERVER_DO_RECV(flags)                                                                                \
	const auto result{ recv(                                                                                           \
		recvBufferInfo->connection, &(static_cast<char*>(*recvBufferInfo->buffer))[offset], readData, flags) };        \
	if (result == 0) [[unlikely]] {                                                                                    \
		/* Not sure if it is required pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr); */                      \
		LOG_INFO("Connection will be closed, id: " + _S(recvBufferInfo->id));                                          \
		return false;                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	if (result == -1) [[unlikely]] {                                                                                   \
		if (errno == EAGAIN || errno == EWOULDBLOCK) {                                                                 \
			LOG_PROTOCOL(                                                                                              \
				"Non-blocking operation returned EAGAIN or EWOULDBLOC, connection id: " + _S(recvBufferInfo->id));     \
			return false;                                                                                              \
		}                                                                                                              \
                                                                                                                       \
		if (errno == 104) {                                                                                            \
			LOG_PROTOCOL("Recv returned unrecoverable error №104: Connection reset by peer, connection id: "         \
				+ _S(recvBufferInfo->id));                                                                             \
			return false;                                                                                              \
		}                                                                                                              \
                                                                                                                       \
		LOG_ERROR("Recv returned unrecoverable error №" + _S(errno) + ": " + std::strerror(errno)                      \
			+ ", connection id: " + _S(recvBufferInfo->id));                                                           \
		return false;                                                                                                  \
	}                                                                                                                  \
	/* Not sure if it is required pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr); */                           \
	LOG_PROTOCOL("Get data from connection: " + _S(recvBufferInfo->connection) + ", id: " + _S(recvBufferInfo->id)     \
		+ ", size: " + _S(readData) + ", read size: " + _S(result) + ", flags: " + _S(flags)                           \
		+ ", offset: " + _S(offset));

#define TMP_MSAPI_SERVER_DO_DROP                                                                                       \
	int devNull{ open("/dev/null", O_WRONLY) };                                                                        \
	if (devNull == -1) [[unlikely]] {                                                                                  \
		LOG_ERROR("Failed to open /dev/null");                                                                         \
		return false;                                                                                                  \
	}                                                                                                                  \
	const auto bytesSpliced{ splice(                                                                                   \
		recvBufferInfo->connection, nullptr, devNull, nullptr, bufferSize - sizeof(size_t) * 2, SPLICE_F_MOVE) };      \
	if (bytesSpliced == -1) [[unlikely]] {                                                                             \
		LOG_ERROR("Failed to splice data to /dev/null, id: " + _S(recvBufferInfo->id) + ". Error №" + _S(errno) + ": " \
			+ std::strerror(errno));                                                                                   \
		close(devNull);                                                                                                \
		return false;                                                                                                  \
	}                                                                                                                  \
	LOG_PROTOCOL("Spliced " + _S(bytesSpliced) + " out of " + _S(bufferSize - sizeof(size_t) * 2)                      \
		+ " bytes to /dev/null, id: " + _S(recvBufferInfo->id));                                                       \
	close(devNull);

bool Server::ReadAdditionalData(RecvBufferInfo* recvBufferInfo, const size_t bufferSize)
{
	const auto action{ recvBufferInfo->ManageBuffer(bufferSize) };
	switch (action) {
	case RecvBufferInfo::Action::Return:
		return false;
	case RecvBufferInfo::Action::Read: {
		int bytesAvailable{ 0 };
		size_t offset{ sizeof(size_t) * 2 };
		ioctl(recvBufferInfo->connection, FIONREAD, &bytesAvailable);
		if (bytesAvailable > 0) [[likely]] {
			size_t readData{ bufferSize - offset };
			if (UINT64(bytesAvailable) < readData) {
				LOG_PROTOCOL("Available number of bytes is less than need to be read, id: " + _S(recvBufferInfo->id)
					+ ", available: " + _S(bytesAvailable));
				do {
					TMP_MSAPI_SERVER_DO_RECV(0);
					offset += UINT64(result);
					readData -= UINT64(result);
				} while (readData > 0);
			}
			else {
				TMP_MSAPI_SERVER_DO_RECV(0);
			}

			// Diagnostic::PrintBinaryDescriptor(*buffer, bufferSize, "Additional read memory");
			return true;
		}

		if (bytesAvailable == 0) [[likely]] {
			LOG_WARNING("No data available, id: " + _S(recvBufferInfo->id));
			return false;
		}

		LOG_ERROR("Fail to get available number of bytes, id: " + _S(recvBufferInfo->id) + ". Error №" + _S(errno)
			+ ": " + std::strerror(errno));
		return false;
	}
	case RecvBufferInfo::Action::Drop: {
		TMP_MSAPI_SERVER_DO_DROP;
		return false;
	}
	default:
		LOG_ERROR("Unknown action " + _S(static_cast<short>(action)) + ", id: " + _S(recvBufferInfo->id));
		return false;
	}
}

bool Server::LookForAdditionalData(RecvBufferInfo* recvBufferInfo, size_t& bufferSize)
{
	const auto action{ recvBufferInfo->ManageBuffer(bufferSize) };
	switch (action) {
	case RecvBufferInfo::Action::Return:
		return false;
	case RecvBufferInfo::Action::Read: {
		int bytesAvailable{ 0 };
		size_t offset{ sizeof(size_t) * 2 };
		ioctl(recvBufferInfo->connection, FIONREAD, &bytesAvailable);
		if (bytesAvailable > 0) [[likely]] {
			const auto readData{ bufferSize - offset };
			if (UINT64(bytesAvailable) < readData) {
				LOG_PROTOCOL("Available number of bytes is less than need to be read, id: " + _S(recvBufferInfo->id)
					+ ", available: " + _S(bytesAvailable));
			}

			TMP_MSAPI_SERVER_DO_RECV(MSG_PEEK);
			bufferSize = UINT64(result);
			return true;
		}

		if (bytesAvailable == 0) [[likely]] {
			LOG_WARNING("No data available, id: " + _S(recvBufferInfo->id));
			return false;
		}

		LOG_ERROR("Fail to get available number of bytes, id: " + _S(recvBufferInfo->id) + ". Error №" + _S(errno)
			+ ": " + std::strerror(errno));
		return false;
	}
	case RecvBufferInfo::Action::Drop: {
		TMP_MSAPI_SERVER_DO_DROP;
		return false;
	}
	default:
		LOG_ERROR("Unknown action " + _S(static_cast<short>(action)) + ", id: " + _S(recvBufferInfo->id));
		return false;
	}
}

#undef TMP_MSAPI_SERVER_DO_DROP
#undef TMP_MSAPI_SERVER_DO_RECV

std::optional<int> Server::GetConnect(const int id) const
{
	const auto connectionIt = m_infoToConnection.find(id);
	if (connectionIt == m_infoToConnection.end()) {
		LOG_DEBUG("Connection is not found, id: " + _S(id));
		return {};
	}
	return connectionIt->second.connection;
}

in_port_t Server::GetListenedPort() const { return m_listeningPort; }

bool Server::IsRunning() const noexcept { return m_state == State::Running; }

Server::State Server::GetState() const noexcept { return m_state; }

unsigned int Server::GetSecondsBetweenTryToConnect() const noexcept { return m_secondsBetweenTryToConnect; }

size_t Server::GetLimitConnectAttempts() const noexcept { return m_limitConnectAttempts; }

std::string_view Server::EnumToString(const State state)
{
	static_assert(U(State::Max) == 4, "Missed description of server state enum");

	switch (state) {
	case State::Undefined:
		return "Undefined";
	case State::Running:
		return "Running";
	case State::Initialization:
		return "Initialization";
	case State::Stopped:
		return "Stopped";
	case State::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown server state enum: " + _S(U(state)));
		return "Unknown";
	}
}

/*---------------------------------------------------------------------------------
AutoFreeSocket
---------------------------------------------------------------------------------*/

Server::AutoFreeSocket::AutoFreeSocket(const int socket, bool* socketCheck)
	: socket(socket)
	, socketCheck(socketCheck)
{
}

Server::AutoFreeSocket::~AutoFreeSocket()
{
	if (socketCheck != nullptr && *socketCheck) {
		socketCheck = nullptr;
		// TODO: Can throw error if socket was created but not binded
		if (shutdown(socket, SHUT_RDWR) == -1) {
			LOG_ERROR(
				"Fail to shutdown connection " + _S(socket) + ". Error №" + _S(errno) + ": " + std::strerror(errno));
		}
		if (close(socket) == -1) {
			LOG_ERROR("Fail to close connection " + _S(socket) + ". Error №" + _S(errno) + ": " + std::strerror(errno));
		}
	}
}

/*---------------------------------------------------------------------------------
ConnectionInfo
---------------------------------------------------------------------------------*/

Server::ConnectionInfo::ConnectionInfo(const int id, const in_addr_t& ip, const in_port_t& port, const int connection,
	const std::string& textIp, const bool needReconnection)
	: id(id)
	, ip(ip)
	, port(port)
	, connection(connection)
	, textIp(textIp)
	, needReconnection(needReconnection)
{
}

/*---------------------------------------------------------------------------------
RecvBufferInfo
---------------------------------------------------------------------------------*/

RecvBufferInfo::RecvBufferInfo(void** buffer, const int connection, const int id, const size_t currentRecvBufferSize,
	const size_t* recvBufferSizeLimit, Server* server)
	: buffer{ buffer }
	, connection{ connection }
	, id{ id }
	, m_currentRecvBufferSize{ currentRecvBufferSize }
	, m_recvBufferSizeLimit{ recvBufferSizeLimit }
	, m_server{ server }
{
}

RecvBufferInfo::Action RecvBufferInfo::ManageBuffer(const size_t bufferSize)
{
	if (bufferSize <= m_currentRecvBufferSize) [[likely]] {
		return Action::Read;
	}

	if (bufferSize > *m_recvBufferSizeLimit) [[unlikely]] {
		LOG_ERROR("Needed buffer size (" + _S(bufferSize) + ") is greater than limit (" + _S(*m_recvBufferSizeLimit)
			+ "), connection id: " + _S(id));
		return Action::Drop;
	}

	void* newBuffer{ realloc(*buffer, bufferSize) };
	if (newBuffer == nullptr) [[unlikely]] {
		LOG_ERROR("Failed to reallocate " + _S(bufferSize) + " bytes of memory, connection id: " + _S(id));
		return Action::Drop;
	}

	m_currentRecvBufferSize = bufferSize;
	*buffer = newBuffer;
	LOG_PROTOCOL(
		"Reallocate buffer size: " + _S(m_currentRecvBufferSize) + " bytes successfully, connection id: " + _S(id));
	return Action::Read;
}

}; //* namespace MSAPI