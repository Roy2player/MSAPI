/**************************
 * @file        daemon.hpp
 * @version     6.0
 * @date        2023-09-17
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

#ifndef MSAPI_DAEMON_H
#define MSAPI_DAEMON_H

#include "../server/server.h"
#include <cstring>
#include <thread>

namespace MSAPI {

/**************************
 * @brief Function to start main Server process in new separated pthread.
 *
 * @tparam T Application class.
 *
 * @param dataOfPthreads Tuple with server, pthread, addr and port.
 *
 * @return nullptr.
 */
template <typename T> void* StartingRequest(void* dataOfPthreads)
{
	LOG_DEBUG("Pthread function is called, PID: " + _S(gettid()));
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
	std::tuple<T*, pthread_t, in_addr_t, in_port_t> serverParameters
		= *static_cast<std::tuple<T*, pthread_t, in_addr_t, in_port_t>*>(dataOfPthreads);
	T* server{ std::get<0>(serverParameters) };
	server->Start(std::get<2>(serverParameters), std::get<3>(serverParameters));
	return nullptr;
}

/**************************
 * @brief Base class for Daemon.
 */
class DaemonBase {
public:
	virtual ~DaemonBase() = default;

	/**************************
	 * @brief Start main Server process in new separated pthread, waiting until it is running or stopped. Port will be
	 * free only when Deamon is destroyed.
	 *
	 * @param addr Pthread addr struct.
	 * @param port Pthread port struct.
	 *
	 * @return True if server started, false in another way.
	 */
	virtual bool Start([[maybe_unused]] const in_addr_t addr, [[maybe_unused]] const in_port_t port)
	{
		LOG_ERROR("Called method from pure Daemon Base interface class, data is casted wrongly");
		return false;
	}

	/**************************
	 * @return Pointer to server.
	 */
	virtual void* GetApp()
	{
		LOG_ERROR("Called method from pure Daemon Base interface class, data is casted wrongly");
		return nullptr;
	}

	/**************************
	 * @return Port of server.
	 */
	virtual unsigned short GetPort() const
	{
		LOG_ERROR("Called method from pure Daemon Base interface class, data is casted wrongly");
		return 0;
	}

protected:
	/**************************
	 * @brief Set server state as Initialization.
	 */
	void SetInitializationState(Server* server) { server->m_state = Server::State::Initialization; }
};

/**************************
 * @brief Container to create APP which is based on Server and run main process in separate pthread with save
 * possibility to manage application directly.
 *
 * @tparam T Application class.
 */
template <typename T> class Daemon : public DaemonBase {
private:
	T m_application;
	pthread_t m_pthread;
	std::set<int> m_connections;
	//* { port, domain }
	std::map<int, std::pair<in_port_t, std::string>> m_connectionsDataToId;
	//* { App, ... }
	std::tuple<T*, pthread_t, in_addr_t, in_port_t> m_dataOfPthread;
	bool m_isRan{ false };

	static inline std::set<unsigned short> m_ports;

public:
	/**************************
	 * @brief Construct a new Daemon for T.
	 */
	template <typename... Args>
	Daemon(Args&&... args)
		: m_application(std::forward<Args>(args)...)
	{
	}

	/**************************
	 * @brief Destroy the Daemon for T and remove port from used ports.
	 */
	~Daemon()
	{
		if (m_isRan) {
			m_application.HandlePauseRequest();
			m_application.Server::Stop();
			m_ports.erase(std::get<3>(m_dataOfPthread));
		}
	}

	/**************************
	 * @brief Open a TCP socket connection.
	 *
	 * @param port Connection port.
	 * @param domain Domain to connect.
	 *
	 * @return Id of new connection or empty if something went wrong.
	 *
	 * @todo Check if domain is valid and if it is IP (mean ConnectionToDomainOrIp).
	 * @todo Store by port/ip 64-bit value
	 */
	FORCE_INLINE std::optional<int> ConnectToDomain(const in_port_t port, const std::string& domain)
	{
		int id;
		do {
			id = static_cast<int>(Identifier::mersenne());
		} while (m_connections.find(id) != m_connections.end());
		LOG_INFO("Daemon is connecting to domain: " + domain + ", id: " + _S(id));
		if (!m_application->ConnectOpen(id, inet_addr(Helper::DomainToIp(domain.c_str()).c_str()), port, false)) {
			return {};
		}
		m_connections.insert(id);
		m_connectionsDataToId.insert({ id, { port, domain } });
		return id;
	}

	/**************************
	 * @brief Start main Server process in new separated pthread, waiting until it is running or stopped. Port will be
	 * free only when Deamon is destroyed.
	 *
	 * @param addr Pthread addr struct.
	 * @param port Pthread port struct.
	 *
	 * @return True if server started, false in another way.
	 */
	FORCE_INLINE bool Start(const in_addr_t addr, const in_port_t port) final
	{
		if (static_cast<Server*>(&m_application)->GetState() == MSAPI::Server::State::Running) {
			LOG_ERROR("Application is in running state, port: " + _S(port));
			return false;
		}

		//* Because function can be used directly
		m_ports.insert(port);

		//* Because can be called on already stopped server
		SetInitializationState(static_cast<Server*>(&m_application));

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		// The minimum pthread stack is only POSIX requirement, which does not takes into additional requirements, like
		// guard page, bookkeeping/padding and god knows what else.
		// pthread_attr_setstacksize(&attr, UINT64(2 * PTHREAD_STACK_MIN));
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		m_dataOfPthread = { &m_application, m_pthread, addr, port };
		if (const auto result{
				pthread_create(&m_pthread, &attr, StartingRequest<T>, static_cast<void*>(&m_dataOfPthread)) };
			result != 0) {

			LOG_ERROR("Pthread for deamon is not created. Error №" + _S(result) + ": " + std::strerror(result));
			pthread_attr_destroy(&attr);
			return false;
		}

		pthread_attr_destroy(&attr);
		LOG_DEBUG("Pthread for deamon is created successfully");

		while (true) {
			if (static_cast<Server*>(&m_application)->IsRunning()) {
				m_isRan = true;
				return true;
			}

			if (static_cast<Server*>(&m_application)->GetState() == MSAPI::Server::State::Stopped) {
				LOG_ERROR("Application is in Stopped state, port: " + _S(port));
				break;
			}

			std::this_thread::sleep_for(std::chrono::microseconds(50));
		};

		m_ports.erase(port);
		return false;
	}

	/**************************
	 * @return Pointer to application.
	 */
	FORCE_INLINE void* GetApp() final { return &m_application; }

	/**************************
	 * @return Port of application.
	 */
	FORCE_INLINE unsigned short GetPort() const final { return std::get<3>(m_dataOfPthread); }

	/**************************
	 * @brief Create a new Daemon and start it with listened generated port on any address. Port will be free only when
	 * Deamon is destroyed.
	 *
	 * @tparam Args Arguments for Deamon type.
	 *
	 * @param name Name of application.
	 * @param args Arguments for Deamon type.
	 *
	 * @return Unique pointer to new Daemon or empty if something went wrong.
	 */
	template <typename... Args>
	static FORCE_INLINE std::unique_ptr<DaemonBase> Create(std::string&& name, Args&&... args)
	{
		auto daemon{ std::make_unique<MSAPI::Daemon<T>>(std::forward(args)...) };
		MSAPI::Server* server{ static_cast<MSAPI::Server*>(daemon->GetApp()) };
		server->SetName(name);
		unsigned short port{ static_cast<unsigned short>(Identifier::mersenne() % (65535 - 3000) + 3000) };

		size_t counter{ 0 };
		do {
			if (m_ports.insert(port).second) {
				break;
			}
			port = static_cast<unsigned short>(Identifier::mersenne() % (65535 - 3000) + 3000);

			if (++counter >= 50000) {
				LOG_ERROR("Cannot generate a unique port for app: " + name);
				return {};
			}
		} while (true);

		LOG_DEBUG("Creating application name: " + name + ", port: " + _S(port));
		if (!daemon->Start(INADDR_LOOPBACK, port)) {
			return {};
		}

		return daemon;
	}
};

}; //* namespace MSAPI

#endif //* MSAPI_DAEMON_H