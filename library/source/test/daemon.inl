/**************************
 * @file        daemon.inl
 * @date        2023-09-17
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

#ifndef MSAPI_DAEMON_INL
#define MSAPI_DAEMON_INL

#include "../server/server.inl"
#include <cstring>
#include <random>
#include <thread>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Container to create APP which is based on Server and run main process in separate pthread with save
 * possibility to manage application directly.
 *
 * @tparam T Application class.
 *
 * @todo Exclude port generator to separate utility class.
 */
template <typename T> class Daemon {
private:
	struct AppData {
		T* app{};
		Lock::Atomic* lock{};
		uint32_t ip{};
		uint16_t port{};

		FORCE_INLINE AppData(T* app, Lock::Atomic* lock, uint32_t ip, uint16_t port) noexcept;

		FORCE_INLINE AppData() noexcept = default;
		AppData(const AppData&) = delete;
		AppData(AppData&&) = delete;
		AppData& operator=(const AppData&) = delete;
		FORCE_INLINE AppData& operator=(AppData&&) noexcept = default;
	};

private:
	T m_application;
	pthread_t m_pthread;
	Lock::Atomic m_pthreadLock;
	// { port, domain }
	std::map<uint64_t, std::pair<uint16_t, std::string>> m_connectionsDataToId;
	AppData m_appData;
	std::atomic<int32_t> m_connectionIdGenerator{};
	bool m_isRan{};

	static inline std::set<uint16_t> m_ports;

public:
	/**************************
	 * @brief Construct a new Daemon for T.
	 */
	template <typename... Args> FORCE_INLINE Daemon(Args&&... args);

	/**************************
	 * @brief Destroy the Daemon for T and remove port from used ports.
	 */
	FORCE_INLINE ~Daemon();

	Daemon(const Daemon&) = delete;
	Daemon(Daemon&&) = delete;
	Daemon& operator=(const Daemon&) = delete;
	Daemon& operator=(Daemon&&) = delete;

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
	FORCE_INLINE [[nodiscard]] std::optional<int> ConnectToDomain(uint16_t port, const std::string& domain);

	/**************************
	 * @brief Start main Server process in new separated pthread, waiting until it is running or stopped. Port will be
	 * free only when Daemon is destroyed.
	 *
	 * @param ip Address to listen.
	 * @param port Port to listen.
	 *
	 * @return True if server started, false in another way.
	 */
	FORCE_INLINE [[nodiscard]] bool Start(uint32_t ip, uint16_t port);

	/**************************
	 * @return Application.
	 */
	FORCE_INLINE [[nodiscard]] T& GetApp();

	/**************************
	 * @return Port of application.
	 */
	FORCE_INLINE [[nodiscard]] uint16_t GetPort() const;

	/**************************
	 * @brief Create a new Daemon and start it with listened generated port on any address. Port will be free only when
	 * Daemon is destroyed.
	 *
	 * @tparam Args Arguments for Daemon type.
	 *
	 * @param name Name of application.
	 * @param args Arguments for Daemon type.
	 *
	 * @return Unique pointer to new Daemon or empty if something went wrong.
	 */
	template <typename... Args>
	static FORCE_INLINE [[nodiscard]] std::unique_ptr<Daemon<T>> Create(std::string&& name, Args&&... args);

	/**************************
	 * @brief Function to start main Server process in new separated pthread.
	 *
	 * @param appData Info to manage application.
	 *
	 * @return nullptr.
	 */
	static void* StartingRequest(void* appData);
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
AppData
---------------------------------------------------------------------------------*/

template <typename T>
FORCE_INLINE Daemon<T>::AppData::AppData(
	T* const app, Lock::Atomic* const lock, const uint32_t ip, const uint16_t port) noexcept
	: app{ app }
	, lock{ lock }
	, ip{ ip }
	, port{ port }
{
}

/*---------------------------------------------------------------------------------
Daemon
---------------------------------------------------------------------------------*/

template <typename T>
template <typename... Args>
FORCE_INLINE Daemon<T>::Daemon(Args&&... args)
	: m_application(std::forward<Args>(args)...)
{
}

template <typename T> FORCE_INLINE Daemon<T>::~Daemon()
{
	if (m_isRan) {
		m_application.HandlePauseRequest();
		m_application.Server::Stop();
		m_ports.erase(m_appData.port);
	}
}

template <typename T>
FORCE_INLINE [[nodiscard]] std::optional<int> Daemon<T>::ConnectToDomain(const uint16_t port, const std::string& domain)
{
	int id;
	do {
		id = m_connectionIdGenerator.fetch_add(1, std::memory_order_relaxed);
	} while (m_connectionsDataToId.find(id) != m_connectionsDataToId.end());
	LOG_INFO("Daemon is connecting to domain: " + domain + ", id: " + _S(id));
	if (!m_application->ConnectOpen(id, inet_addr(Helper::DomainToIp(domain.c_str()).c_str()), port, false)) {
		return {};
	}
	m_connectionsDataToId.insert({ id, { port, domain } });
	return id;
}

template <typename T> FORCE_INLINE [[nodiscard]] bool Daemon<T>::Start(const uint32_t ip, const uint16_t port)
{
	auto state{ static_cast<Server*>(&m_application)->GetState() };
	if (state == Server::State::Running) {
		LOG_ERROR("Application is in running state, port: " + _S(port));
		return false;
	}

	if (m_isRan) {
		{
			Lock::Atomic::Guard _{ m_pthreadLock };
		}
		m_ports.erase(m_appData.port);
		m_isRan = false;
	}

	// Because function can be used directly
	m_ports.insert(port);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	Server::AddPthreadAttributes(attr);

	m_appData = { &m_application, &m_pthreadLock, ip, port };
	if (const auto result{ pthread_create(&m_pthread, &attr, StartingRequest, static_cast<void*>(&m_appData)) };
		result != 0) {

		LOG_ERROR("Pthread for daemon is not created. Error №" + _S(result) + ": " + std::strerror(result));
		pthread_attr_destroy(&attr);
		return false;
	}

	pthread_attr_destroy(&attr);
	LOG_DEBUG("Pthread for daemon is created successfully");

	while (true) {
		state = static_cast<Server*>(&m_application)->GetState();
		if (state == Server::State::Running) {
			m_isRan = true;
			return true;
		}

		if (state == Server::State::Stopped) {
			LOG_ERROR("Application is in Stopped state, port: " + _S(port));
			break;
		}

		std::this_thread::sleep_for(std::chrono::microseconds(50));
	};

	m_ports.erase(port);
	return false;
}

template <typename T> FORCE_INLINE [[nodiscard]] T& Daemon<T>::GetApp() { return m_application; }

template <typename T> FORCE_INLINE [[nodiscard]] uint16_t Daemon<T>::GetPort() const { return m_appData.port; }

template <typename T>
template <typename... Args>
FORCE_INLINE [[nodiscard]] [[nodiscard]] std::unique_ptr<Daemon<T>> Daemon<T>::Create(
	std::string&& name, Args&&... args)
{
	auto daemon{ std::make_unique<Daemon<T>>(std::forward<Args>(args)...) };
	daemon->GetApp().SetName(name);
	std::mt19937 mersenne{ UINT64(Timer{}.GetNanoseconds()) };
	auto port{ static_cast<uint16_t>(mersenne() % (65535 - 3000) + 3000) };
	int32_t counter{ 0 };
	do {
		if (m_ports.insert(port).second) {
			break;
		}
		port = static_cast<uint16_t>(mersenne() % (65535 - 3000) + 3000);

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

template <typename T> FORCE_INLINE [[nodiscard]] void* Daemon<T>::StartingRequest(void* appData)
{
	const auto pid{ gettid() };
	LOG_DEBUG_NEW("Pthread function is called, PID: {}", pid);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
	const auto* serverParameters{ static_cast<Daemon<T>::AppData*>(appData) };
	T* server{ serverParameters->app };
	Lock::Atomic::Guard _{ *serverParameters->lock };
	server->Start(serverParameters->ip, serverParameters->port);
	LOG_DEBUG_NEW("Pthread function is finished, PID: {}", pid);
	return nullptr;
}

} // namespace MSAPI

#endif // MSAPI_DAEMON_INL