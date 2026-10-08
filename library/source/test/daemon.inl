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
	/**************************
	 * @brief Data passed to the daemon pthread to start the application.
	 *
	 * @concurrency No.
	 */
	class AppData {
	private:
		T* m_app{};
		Lock::Atomic* m_lock{};
		uint32_t m_ip{};
		uint16_t m_port{};

	public:
		/**************************
		 * @brief Construct a new AppData object.
		 *
		 * @param app Application to start.
		 * @param lock Lock held by the pthread while the application is running.
		 * @param ip IP address to listen.
		 * @param port Port to listen.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE AppData(T* app, Lock::Atomic* lock, uint32_t ip, uint16_t port) noexcept;

		/**************************
		 * @test Yes.
		 */
		FORCE_INLINE AppData() noexcept = default;

		AppData(const AppData&) = delete;
		AppData(AppData&&) = delete;
		AppData& operator=(const AppData&) = delete;
		FORCE_INLINE AppData& operator=(AppData&&) noexcept = default;

		/**************************
		 * @return Application to start.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] T* GetApp() const noexcept;

		/**************************
		 * @return Lock held by the pthread while the application is running.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] Lock::Atomic* GetLock() const noexcept;

		/**************************
		 * @return IP address to listen.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] uint32_t GetIp() const noexcept;

		/**************************
		 * @return Port to listen.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] uint16_t GetPort() const noexcept;
	};

private:
	// Ports used by created daemons, to generate a unique port for each new daemon
	static inline std::set<uint16_t> PORTS;

private:
	T m_application;
	pthread_t m_pthread;
	mutable Lock::Atomic m_pthreadLock;
	// { port, domain }
	std::map<uint64_t, std::pair<uint16_t, std::string>> m_connectionsDataToId;
	AppData m_appData;
	Server::PthreadAttributes m_pthreadAttributes;
	std::atomic<int32_t> m_connectionIdGenerator{};
	bool m_isRan{};

public:
	/**************************
	 * @brief Construct a new Daemon for T.
	 *
	 * @test Yes.
	 */
	template <typename... Args> FORCE_INLINE explicit Daemon(Args&&... args);

	/**************************
	 * @brief Destroy the Daemon for T and remove port from used ports.
	 *
	 * @test Yes.
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
	 *
	 * @todo Add tests coverage.
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
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Start(uint32_t ip, uint16_t port);

	/**************************
	 * @return Application.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] T& GetApp();

	/**************************
	 * @return Port of application.
	 *
	 * @test Yes.
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
	 *
	 * @test Yes.
	 */
	template <typename... Args>
	static FORCE_INLINE [[nodiscard]] std::unique_ptr<Daemon<T>> Create(std::string&& name, Args&&... args);

	/**************************
	 * @brief Function to start main Server process in new separated pthread.
	 *
	 * @param appData Info to manage application.
	 *
	 * @return nullptr.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static void* StartingRequest(void* appData);
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
	: m_app{ app }
	, m_lock{ lock }
	, m_ip{ ip }
	, m_port{ port }
{
}

template <typename T> FORCE_INLINE [[nodiscard]] T* Daemon<T>::AppData::GetApp() const noexcept { return m_app; }

template <typename T> FORCE_INLINE [[nodiscard]] Lock::Atomic* Daemon<T>::AppData::GetLock() const noexcept
{
	return m_lock;
}

template <typename T> FORCE_INLINE [[nodiscard]] uint32_t Daemon<T>::AppData::GetIp() const noexcept { return m_ip; }

template <typename T> FORCE_INLINE [[nodiscard]] uint16_t Daemon<T>::AppData::GetPort() const noexcept
{
	return m_port;
}

/*---------------------------------------------------------------------------------
Daemon
---------------------------------------------------------------------------------*/

template <typename T>
template <typename... Args>
FORCE_INLINE Daemon<T>::Daemon(Args&&... args)
	: m_application{ std::forward<Args>(args)... }
{
}

template <typename T> FORCE_INLINE Daemon<T>::~Daemon()
{
	if (m_isRan) {
		m_application.HandlePauseRequest();
		m_application.Server::Stop();
		PORTS.erase(m_appData.GetPort());
	}
}

template <typename T>
FORCE_INLINE [[nodiscard]] std::optional<int> Daemon<T>::ConnectToDomain(
	[[maybe_unused]] const uint16_t port, [[maybe_unused]] const std::string& domain)
{
	// Legacy function is not used, but contain some context. Let's keep it.
	//
	// int id;
	// do {
	// 	id = m_connectionIdGenerator.fetch_add(1, std::memory_order_relaxed);
	// } while (m_connectionsDataToId.find(id) != m_connectionsDataToId.end());
	// LOG_INFO("Daemon is connecting to domain: " + domain + ", id: " + _S(id));
	// if (!m_application->ConnectOpen(id, inet_addr(Helper::DomainToIp(domain.c_str()).c_str()), port, false)) {
	// 	return {};
	// }
	// m_connectionsDataToId.insert({ id, { port, domain } });
	// return id;
	return {};
}

template <typename T> FORCE_INLINE [[nodiscard]] bool Daemon<T>::Start(const uint32_t ip, const uint16_t port)
{
	auto state{ m_application.Server::GetState() };
	const auto stoppedStateCount{ m_application.Server::GetStoppedStateCount() };

	if (state == Server::State::Running) {
		LOG_ERROR_NEW("Application is in running state, port: {}", port);
		return false;
	}

	if (m_isRan) {
		{
			Lock::Atomic::Guard _{ m_pthreadLock };
		}
		PORTS.erase(m_appData.GetPort());
		m_isRan = false;
	}

	// Because function can be used directly
	PORTS.insert(port);

	if (!m_pthreadAttributes.IsValid()) [[unlikely]] {
		PORTS.erase(port);
		LOG_ERROR_NEW("Pthread for daemon is not created, pthread attributes are not valid, port: {}", port);
		return false;
	}

	m_appData = AppData{ &m_application, &m_pthreadLock, ip, port };
	if (const auto result{
			pthread_create(&m_pthread, &m_pthreadAttributes.Get(), StartingRequest, static_cast<void*>(&m_appData)) };
		result != 0) {

		LOG_ERROR_NEW("Pthread for daemon is not created. Error №{}: {}", result, std::strerror(result));
		return false;
	}

	LOG_DEBUG("Pthread for daemon is created successfully");

	while (true) {
		state = m_application.Server::GetState();
		if (state == Server::State::Running) {
			m_isRan = true;
			return true;
		}

		if (stoppedStateCount != m_application.Server::GetStoppedStateCount()) {
			LOG_ERROR_NEW("Stopped state count is increased, port: {}", port);
			break;
		}

		std::this_thread::sleep_for(std::chrono::microseconds(50));
	};

	PORTS.erase(port);
	return false;
}

template <typename T> FORCE_INLINE [[nodiscard]] T& Daemon<T>::GetApp() { return m_application; }

template <typename T> FORCE_INLINE [[nodiscard]] uint16_t Daemon<T>::GetPort() const { return m_appData.GetPort(); }

template <typename T>
template <typename... Args>
FORCE_INLINE [[nodiscard]] std::unique_ptr<Daemon<T>> Daemon<T>::Create(std::string&& name, Args&&... args)
{
	auto daemon{ std::make_unique<Daemon<T>>(std::forward<Args>(args)...) };
	daemon->GetApp().SetName(name);
	std::mt19937 mersenne{ UINT64(Timer{}.GetNanoseconds()) };
	auto port{ static_cast<uint16_t>(mersenne() % (65535 - 3000) + 3000) };
	int32_t counter{};
	do {
		if (PORTS.insert(port).second) {
			break;
		}
		port = static_cast<uint16_t>(mersenne() % (65535 - 3000) + 3000);

		if (++counter >= 50000) {
			LOG_ERROR_NEW("Cannot generate a unique port for app: {}", name);
			return {};
		}
	} while (true);

	LOG_DEBUG_NEW("Creating application name: {}, port: {}", name, port);
	if (!daemon->Start(INADDR_LOOPBACK, port)) {
		return {};
	}

	return daemon;
}

template <typename T> FORCE_INLINE [[nodiscard]] void* Daemon<T>::StartingRequest(void* appData)
{
	// Pthread is not cancelable, it is finished cooperatively and cleans up its resources itself
	if (const auto result{ pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr) }; result != 0) [[unlikely]] {
		LOG_ERROR_NEW("Failed to disable pthread cancellation. Error №{}: {}", result, std::strerror(result));
	}

	const auto pid{ gettid() };
	LOG_DEBUG_NEW("Pthread function is called, PID: {}", pid);
	const auto* serverParameters{ static_cast<Daemon<T>::AppData*>(appData) };
	T* server{ serverParameters->GetApp() };
	Lock::Atomic::Guard _{ *serverParameters->GetLock() };
	server->Start(serverParameters->GetIp(), serverParameters->GetPort());
	LOG_DEBUG_NEW("Pthread function is finished, PID: {}", pid);
	return nullptr;
}

} // namespace MSAPI

#endif // MSAPI_DAEMON_INL