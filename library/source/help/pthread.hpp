/**************************
 * @file        pthread.hpp
 * @version     6.0
 * @date        2024-01-28
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

#ifndef MSAPI_PTHREAD_H
#define MSAPI_PTHREAD_H

#include "log.h"
#include <sys/socket.h>

namespace MSAPI {

namespace Pthread {

/**************************
 * @brief Struct to contain mutex with name.
 *
 * @tparam T Pthread mutex or pthread rwlock.
 */
template <typename T>
	requires(std::is_same_v<T, pthread_mutex_t> || std::is_same_v<T, pthread_rwlock_t>)
struct NamedMutex {
	T mutex;
	const std::string name;

	/**************************
	 * @brief Construct a new Named Mutex object.
	 *
	 * @param name Mutex name.
	 */
	FORCE_INLINE NamedMutex(std::string&& name) noexcept
		: name{ std::move(name) }
	{
	}
};

/**************************
 * @brief Initialize mutex and print error if any occurred.
 *
 * @tparam T Pthread mutex or pthread rwlock.
 * @tparam S Pointer to pthread mutex attributes, pointer to pthread rwlock attributes or nullptr.
 *
 * @param namedMutex Named mutex.
 * @param mutexattr Pointer to attributes or nullptr.
 *
 * @return True if mutex initialized successfully, false if any errors occurred.
 */
template <typename T, typename S>
	requires(
		(std::is_same_v<T, pthread_mutex_t>
			&& (std::is_same_v<std::remove_pointer_t<S>, pthread_mutexattr_t> || std::is_same_v<S, std::nullptr_t>))
		|| (std::is_same_v<T, pthread_rwlock_t>
			&& (std::is_same_v<std::remove_pointer_t<S>, pthread_rwlockattr_t> || std::is_same_v<S, std::nullptr_t>)))
FORCE_INLINE bool PthreadMutexInit(NamedMutex<T>& namedMutex, const S mutexattr) noexcept
{
	int ret{ -1 };
	if constexpr (std::is_same_v<T, pthread_mutex_t>) {
		ret = pthread_mutex_init(&namedMutex.mutex, mutexattr);
	}
	else if constexpr (std::is_same_v<T, pthread_rwlock_t>) {
		ret = pthread_rwlock_init(&namedMutex.mutex, mutexattr);
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Unknown mutex type");
	}

	if (ret != 0) {
		switch (ret) {
		case EAGAIN:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The system lacked the necessary resources (other than memory) to initialize another mutex, "
				  "error EAGAIN");
			return false;
		case ENOMEM:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": Insufficient memory exists to initialize the mutex, error ENOMEM");
			return false;
		case EPERM:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The caller does not have the privilege to perform the operation, error EPERM");
			return false;
		case EBUSY:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The implementation has detected an attempt to reinitialize the object referenced by mutex, a "
				  "previously initialized, but not yet destroyed, mutex, error EBUSY");
			return false;
		case EINVAL:
			LOG_ERROR(
				"Pthread mutex name \"" + namedMutex.name + "\": The value specified by attr is invalid, error EINVAL");
			return false;
		default:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

/**************************
 * @brief Destroy mutex and print error if any occurred. Calls inside PthreadMutexLock() and PthreadMutexUnlock()
 * before.
 *
 * @tparam T Pthread mutex or pthread rwlock.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex destroyed successfully, false if any errors occurred.
 */
template <typename T> FORCE_INLINE bool PthreadMutexDestroy(NamedMutex<T>& namedMutex) noexcept
{
	int ret{ -1 };

	if constexpr (std::is_same_v<T, pthread_mutex_t>) {
		ret = pthread_mutex_destroy(&namedMutex.mutex);
	}
	else if constexpr (std::is_same_v<T, pthread_rwlock_t>) {
		ret = pthread_rwlock_destroy(&namedMutex.mutex);
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Unknown mutex type");
	}

	if (ret != 0) {
		switch (ret) {
		case EBUSY:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The implementation has detected an attempt to destroy the object referenced by mutex while it "
				  "is locked or referenced (for example, while being used in a pthread_cond_timedwait() or "
				  "pthread_cond_wait()) by another thread, error EBUSY");
			return false;
		case EINVAL:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		default:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

/**************************
 * @brief Lock mutex and print error if any occurred.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex locked successfully, false if any errors occurred.
 */
FORCE_INLINE bool PthreadMutexLock(NamedMutex<pthread_mutex_t>& namedMutex) noexcept
{
	if (const auto ret{ pthread_mutex_lock(&namedMutex.mutex) }; ret != 0) {
		switch (ret) {
		case EINVAL:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the "
				  "calling thread's priority is higher than the mutex's current priority ceiling, error EINVAL");
			return false;
		case EAGAIN:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The mutex could not be acquired, because the maximum number of recursive locks for mutex has "
				  "been exceeded, error EAGAIN");
			return false;
		case EDEADLK:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": A deadlock condition was detected or the value of mutex is invalid, error EDEADLK");
			return false;
		default:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

constexpr bool write{ true };
constexpr bool read{ false };

static_assert(write, "Pthread write must be true");
static_assert(!read, "Pthread read must be false");

constexpr bool tryLock{ true };
constexpr bool doLock{ false };

static_assert(tryLock, "Pthread tryLock must be true");
static_assert(!doLock, "Pthread doLock must be false");

/**************************
 * @brief Lock read write mutex and print error if any occurred.
 *
 * @tparam Wr True for write lock, false for read lock.
 * @tparam Try True for try lock, false for lock.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex locked successfully, false if any errors occurred or try lock and mutex busy.
 */
template <bool Wr, bool Try> FORCE_INLINE bool PthreadMutexRWLock(NamedMutex<pthread_rwlock_t>& namedMutex) noexcept
{
	int ret{ -1 };

	if constexpr (Try) {
		if constexpr (Wr) {
			ret = pthread_rwlock_trywrlock(&namedMutex.mutex);
		}
		else {
			ret = pthread_rwlock_tryrdlock(&namedMutex.mutex);
		}
	}
	else {
		if constexpr (Wr) {
			ret = pthread_rwlock_wrlock(&namedMutex.mutex);
		}
		else {
			ret = pthread_rwlock_rdlock(&namedMutex.mutex);
		}
	}

	if (ret != 0) {
		switch (ret) {
		case EBUSY: //* trywrlock and tryrdlock
			LOG_DEBUG("Pthread rwlock name \"" + namedMutex.name
				+ "\": The read lock could not be acquired because a writer holds the lock, error EBUSY");
			return false;
		case EINVAL: //* rdlock, tryrdlock, wrlock and trywrlock
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		case EAGAIN: //* rdlock and tryrdlock
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The mutex could not be acquired, because the maximum number of recursive locks for mutex has "
				  "been exceeded, error EAGAIN");
			return false;
		case EDEADLK: //* rdlock, wrlock and trywrlock
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": A deadlock condition was detected or the value of mutex is invalid, error EDEADLK");
			return false;
		default:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

/**************************
 * @brief Try unlock mutex and print error if any occurred.
 *
 * @tparam T Pthread mutex or pthread rwlock.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex is unlocked successfully, false if any errors occurred.
 */
template <typename T> FORCE_INLINE bool PthreadMutexUnlock(NamedMutex<T>& namedMutex) noexcept
{
	int ret{ -1 };
	if constexpr (std::is_same_v<T, pthread_mutex_t>) {
		ret = pthread_mutex_unlock(&namedMutex.mutex);
	}
	else if constexpr (std::is_same_v<T, pthread_rwlock_t>) {
		ret = pthread_rwlock_unlock(&namedMutex.mutex);
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Unknown mutex type");
	}

	if (ret != 0) {
		switch (ret) {
		case EPERM:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The current thread does not own the mutex, error EPERM");
			return false;
		case EAGAIN: //* Only for pthread_mutex_t
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The mutex could not be acquired, because the maximum number of recursive locks for mutex has "
				  "been exceeded, error EAGAIN");
			return false;
		case EINVAL:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name
				+ "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		default:
			LOG_ERROR("Pthread mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

/**************************
 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking mutex.
 */
class ExitGuard {
private:
	NamedMutex<pthread_mutex_t>& m_namedMutex;

public:
	/**************************
	 * @brief Construct a new Exit Guard object, lock mutex.
	 *
	 * @param namedMutex Named mutex.
	 */
	FORCE_INLINE ExitGuard(NamedMutex<pthread_mutex_t>& namedMutex) noexcept
		: m_namedMutex{ namedMutex }
	{
		PthreadMutexLock(namedMutex);
	}

	/**************************
	 * @brief Destroy the Exit Guard object, unlock mutex.
	 */
	FORCE_INLINE ~ExitGuard() noexcept { PthreadMutexUnlock(m_namedMutex); }
};

class AtomicRWLock;

/**************************
 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking read/write mutex.
 *
 * @tparam Wr True for write lock, false for read lock.
 */
template <bool Wr> class ExitGuardRW {
private:
	NamedMutex<pthread_rwlock_t>& m_namedMutex;

public:
	/**************************
	 * @brief Construct a new Exit Guard RW object, lock mutex.
	 *
	 * @param mutex Pointer to mutex.
	 * @param name Mutex name for logging.
	 */
	FORCE_INLINE ExitGuardRW(NamedMutex<pthread_rwlock_t>& namedMutex) noexcept
		: m_namedMutex{ namedMutex }
	{
		PthreadMutexRWLock<Wr, doLock>(m_namedMutex);
	}

	/**************************
	 * @brief Destroy the Exit Guard RW object, unlock mutex.
	 */
	FORCE_INLINE ~ExitGuardRW() noexcept { PthreadMutexUnlock(m_namedMutex); }
};

/**************************
 * @brief Atomic lock based on std::atomic_flag.
 */
class AtomicLock {
public:
	/**************************
	 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking atomic lock.
	 */
	class ExitGuard {
	private:
		AtomicLock& m_atomicLock;

	public:
		/**************************
		 * @brief Construct a new Exit Guard object, lock atomic lock.
		 *
		 * @param atomicLock Atomic lock.
		 */
		FORCE_INLINE ExitGuard(AtomicLock& atomicLock) noexcept
			: m_atomicLock{ atomicLock }
		{
			m_atomicLock.Lock();
		}

		/**************************
		 * @brief Destroy the Exit Guard object, unlock atomic lock.
		 */
		FORCE_INLINE ~ExitGuard() noexcept { m_atomicLock.Unlock(); }
	};

private:
	std::atomic_flag m_lock{};

public:
	/**************************
	 * @brief Wait for lock is false and set it to true.
	 */
	FORCE_INLINE void Lock() noexcept
	{
		while (m_lock.test_and_set(std::memory_order_acquire)) {
			m_lock.wait(true, std::memory_order_relaxed);
		}
	}

	/**************************
	 * @brief Try to set lock to true.
	 *
	 * @return True if lock was false and now is true, false if lock was true.
	 */
	FORCE_INLINE bool TryLock() noexcept { return !m_lock.test_and_set(std::memory_order_acquire); }

	/**************************
	 * @brief Set lock to false and notify one thread.
	 */
	FORCE_INLINE void Unlock() noexcept
	{
		m_lock.clear(std::memory_order_release);
		m_lock.notify_one();
	}

	/**************************
	 * @brief Allow AtomicRWLock to access private members.
	 */
	friend class AtomicRWLock;
};

/**************************
 * @brief Atomic read/write lock based on std::atomic and AtomicLock for write operations.
 */
class AtomicRWLock {
public:
	/**************************
	 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking atomic read/write lock.
	 *
	 * @tparam Wr True for write lock, false for read lock.
	 */
	template <bool Wr> class ExitGuard {
	private:
		AtomicRWLock& m_atomicRWLock;

	public:
		/**************************
		 * @brief Construct a new Exit Guard object, lock atomic read/write lock.
		 *
		 * @param atomicRWLock Atomic read/write lock.
		 */
		FORCE_INLINE ExitGuard(AtomicRWLock& atomicRWLock) noexcept
			: m_atomicRWLock{ atomicRWLock }
		{
			if constexpr (Wr) {
				m_atomicRWLock.WriteLock();
			}
			else {
				m_atomicRWLock.ReadLock();
			}
		}

		/**************************
		 * @brief Destroy the Exit Guard object, unlock atomic read/write lock.
		 */
		FORCE_INLINE ~ExitGuard() noexcept
		{
			if constexpr (Wr) {
				m_atomicRWLock.WriteUnlock();
			}
			else {
				m_atomicRWLock.ReadUnlock();
			}
		}
	};

private:
	std::atomic<int> m_lock{};
	AtomicLock m_writeLock;

public:
	/**************************
	 * @brief Lock for read, wait if write lock is not set.
	 */
	FORCE_INLINE void ReadLock() noexcept
	{
		if (!m_writeLock.m_lock.test()) {
			m_writeLock.m_lock.wait(true, std::memory_order_relaxed);
		}

		m_lock.fetch_add(1, std::memory_order_acquire);
	}

	/**************************
	 * @brief Unlock for read.
	 */
	FORCE_INLINE void ReadUnlock() noexcept
	{
		m_lock.fetch_sub(1, std::memory_order_release);
		m_lock.notify_all();
	}

	/**************************
	 * @brief Lock for write, wait if write lock is not set and then wait for all read locks to be released.
	 */
	FORCE_INLINE void WriteLock() noexcept
	{
		m_writeLock.Lock();
		while (m_lock.load(std::memory_order_acquire) != 0) {
			m_lock.wait(1, std::memory_order_relaxed);
		}
	}

	/**************************
	 * @brief Unlock for write.
	 */
	FORCE_INLINE void WriteUnlock() noexcept
	{
		m_writeLock.Unlock();
		m_lock.notify_all();
	}
};

}; //* namespace Pthread

}; //* namespace MSAPI

#endif //* MSAPI_PTHREAD_H