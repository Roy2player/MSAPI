/**************************
 * @file        lock.inl
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

#ifndef MSAPI_LOCK_INL
#define MSAPI_LOCK_INL

#include "log.h"
#include <sys/socket.h>

namespace MSAPI {

namespace Lock {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

template <typename T>
concept MutexT = std::is_same_v<T, pthread_mutex_t> || std::is_same_v<T, pthread_rwlock_t>;

/**************************
 * @brief Struct to contain mutex with name.
 *
 * @tparam T Mutex or rwlock.
 */
template <MutexT T> struct NamedMutex {
	T mutex;
	const std::string name;

	/**************************
	 * @brief Construct a new Named Mutex object.
	 *
	 * @param name Mutex name.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE NamedMutex(std::string&& name) noexcept;
};

template <typename T, typename S>
concept MutexAndParams
	= (std::is_same_v<T, pthread_mutex_t>
		  && (std::is_same_v<std::remove_pointer_t<S>, pthread_mutexattr_t> || std::is_same_v<S, std::nullptr_t>))
	|| (std::is_same_v<T, pthread_rwlock_t>
		&& (std::is_same_v<std::remove_pointer_t<S>, pthread_rwlockattr_t> || std::is_same_v<S, std::nullptr_t>));

/**************************
 * @brief Initialize mutex and print error if any occurred.
 *
 * @tparam T Mutex or rwlock.
 * @tparam S Pointer to mutex attributes, pointer to rwlock attributes or nullptr.
 *
 * @param namedMutex Named mutex.
 * @param mutexattr Pointer to attributes or nullptr.
 *
 * @return True if mutex initialized successfully, false if any errors occurred.
 *
 * @todo Add unit test.
 */
template <typename T, typename S>
	requires MutexAndParams<T, S>
FORCE_INLINE [[nodiscard]] bool MutexInit(NamedMutex<T>& namedMutex, const S mutexattr);

/**************************
 * @brief Destroy mutex and print error if any occurred. Calls inside MutexLock() and MutexUnlock()
 * before.
 *
 * @tparam T Mutex or rwlock.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex destroyed successfully, false if any errors occurred.
 *
 * @todo Add unit test.
 */
template <typename T> FORCE_INLINE [[nodiscard]] bool MutexDestroy(NamedMutex<T>& namedMutex);

/**************************
 * @brief Lock mutex and print error if any occurred.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex locked successfully, false if any errors occurred.
 *
 * @todo Add unit test.
 */
FORCE_INLINE [[nodiscard]] bool MutexLock(NamedMutex<pthread_mutex_t>& namedMutex);

constexpr bool write{ true };
constexpr bool read{ false };

static_assert(write, "Lock \"write\" must be true");
static_assert(!read, "Lock \"read\" must be false");

constexpr bool tryLock{ true };
constexpr bool doLock{ false };

static_assert(tryLock, "Lock \"tryLock\" must be true");
static_assert(!doLock, "Lock \"doLock\" must be false");

/**************************
 * @brief Lock read write mutex and print error if any occurred.
 *
 * @tparam Wr True for write lock, false for read lock.
 * @tparam Try True for try lock, false for lock.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex locked successfully, false if any errors occurred or try lock and mutex busy.
 *
 * @todo Add unit test.
 */
template <bool Wr, bool Try> FORCE_INLINE [[nodiscard]] bool MutexRWLock(NamedMutex<pthread_rwlock_t>& namedMutex);

/**************************
 * @brief Try unlock mutex and print error if any occurred.
 *
 * @tparam T Mutex or rwlock.
 *
 * @param namedMutex Named mutex.
 *
 * @return True if mutex is unlocked successfully, false if any errors occurred.
 *
 * @todo Add unit test.
 */
template <typename T> FORCE_INLINE [[nodiscard]] bool MutexUnlock(NamedMutex<T>& namedMutex);

/**************************
 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking mutex.
 */
class Guard {
private:
	NamedMutex<pthread_mutex_t>& m_namedMutex;

public:
	/**************************
	 * @brief Construct a new Guard object, lock mutex.
	 *
	 * @param namedMutex Named mutex.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE Guard(NamedMutex<pthread_mutex_t>& namedMutex) noexcept;

	Guard(const Guard&) = delete;
	const Guard& operator=(const Guard&) = delete;
	Guard(Guard&&) = delete;
	const Guard& operator=(Guard&&) = delete;

	/**************************
	 * @brief Destroy the Guard object, unlock mutex.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE ~Guard() noexcept;
};

class AtomicRW;

/**************************
 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking read/write mutex.
 *
 * @tparam Wr True for write lock, false for read lock.
 */
template <bool Wr> class GuardRW {
private:
	NamedMutex<pthread_rwlock_t>& m_namedMutex;

public:
	/**************************
	 * @brief Construct a new Guard RW object, lock mutex.
	 *
	 * @param mutex Pointer to mutex.
	 * @param name Mutex name for logging.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE GuardRW(NamedMutex<pthread_rwlock_t>& namedMutex) noexcept;

	GuardRW(const GuardRW&) = delete;
	const GuardRW& operator=(const GuardRW&) = delete;
	GuardRW(GuardRW&&) = delete;
	const GuardRW& operator=(GuardRW&&) = delete;

	/**************************
	 * @brief Destroy the Guard RW object, unlock mutex.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE ~GuardRW() noexcept;
};

/**************************
 * @brief Atomic lock based on std::atomic_flag.
 */
class Atomic {
public:
	/**************************
	 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking atomic lock.
	 */
	class Guard {
	private:
		Atomic& m_atomicLock;

	public:
		/**************************
		 * @brief Construct a new Guard object, lock atomic lock.
		 *
		 * @param atomicLock Atomic lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE Guard(Atomic& atomicLock) noexcept;

		Guard(const Guard&) = delete;
		const Guard& operator=(const Guard&) = delete;
		Guard(Guard&&) = delete;
		const Guard& operator=(Guard&&) = delete;

		/**************************
		 * @brief Destroy the Guard object, unlock atomic lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE ~Guard() noexcept;
	};

private:
	std::atomic_flag m_lock{};

public:
	FORCE_INLINE Atomic() noexcept = default;

	Atomic(const Atomic&) = delete;
	const Atomic& operator=(const Atomic&) = delete;
	Atomic(Atomic&&) = delete;
	const Atomic& operator=(Atomic&&) = delete;

	/**************************
	 * @brief Wait for lock is false and set it to true.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Lock() noexcept;

	/**************************
	 * @brief Try to set lock to true.
	 *
	 * @return True if lock was false and now is true, false if lock was true.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE bool TryLock() noexcept;

	/**************************
	 * @brief Set lock to false and notify one thread.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void Unlock() noexcept;

	// Allow AtomicRW to access private members.
	friend class AtomicRW;
};

/**************************
 * @brief Atomic read/write lock based on std::atomic and Atomic for write operations.
 */
class AtomicRW {
public:
	/**************************
	 * @brief Resource acquisition is initialization (RAII) Guard for locking and unlocking atomic read/write lock.
	 *
	 * @tparam Wr True for write lock, false for read lock.
	 */
	template <bool Wr> class Guard {
	private:
		AtomicRW& m_atomicRWLock;

	public:
		/**************************
		 * @brief Construct a new Guard object, lock atomic read/write lock.
		 *
		 * @param atomicRWLock Atomic read/write lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE Guard(AtomicRW& atomicRWLock) noexcept;

		Guard(const Guard&) = delete;
		const Guard& operator=(const Guard&) = delete;
		Guard(Guard&&) = delete;
		const Guard& operator=(Guard&&) = delete;

		/**************************
		 * @brief Destroy the Guard object, unlock atomic read/write lock.
		 *
		 * @todo Add unit test.
		 */
		FORCE_INLINE ~Guard() noexcept;
	};

private:
	std::atomic<int> m_lock{};
	Atomic m_writeLock;

public:
	FORCE_INLINE AtomicRW() noexcept = default;

	AtomicRW(const AtomicRW&) = delete;
	const AtomicRW& operator=(const AtomicRW&) = delete;
	AtomicRW(AtomicRW&&) = delete;
	const AtomicRW& operator=(AtomicRW&&) = delete;

	/**************************
	 * @brief Lock for read, wait if write lock is not set.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void ReadLock() noexcept;

	/**************************
	 * @brief Unlock for read.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void ReadUnlock() noexcept;

	/**************************
	 * @brief Lock for write, wait if write lock is not set and then wait for all read locks to be released.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void WriteLock() noexcept;

	/**************************
	 * @brief Unlock for write.
	 *
	 * @todo Add unit test.
	 */
	FORCE_INLINE void WriteUnlock() noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

template <MutexT T>
FORCE_INLINE NamedMutex<T>::NamedMutex(std::string&& name) noexcept
	: name{ std::move(name) }
{
}

template <typename T, typename S>
	requires MutexAndParams<T, S>
FORCE_INLINE [[nodiscard]] bool MutexInit(NamedMutex<T>& namedMutex, const S mutexattr)
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
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The system lacked the necessary resources (other than memory) to initialize another mutex, "
				  "error EAGAIN");
			return false;
		case ENOMEM:
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": Insufficient memory exists to initialize the mutex, error ENOMEM");
			return false;
		case EPERM:
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The caller does not have the privilege to perform the operation, error EPERM");
			return false;
		case EBUSY:
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The implementation has detected an attempt to reinitialize the object referenced by mutex, a "
				  "previously initialized, but not yet destroyed, mutex, error EBUSY");
			return false;
		case EINVAL:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": The value specified by attr is invalid, error EINVAL");
			return false;
		default:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

template <typename T> FORCE_INLINE [[nodiscard]] bool MutexDestroy(NamedMutex<T>& namedMutex)
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
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The implementation has detected an attempt to destroy the object referenced by mutex while it "
				  "is locked or referenced (for example, while being used in a pthread_cond_timedwait() or "
				  "pthread_cond_wait()) by another thread, error EBUSY");
			return false;
		case EINVAL:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		default:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

FORCE_INLINE [[nodiscard]] bool MutexLock(NamedMutex<pthread_mutex_t>& namedMutex)
{
	if (const auto ret{ pthread_mutex_lock(&namedMutex.mutex) }; ret != 0) {
		switch (ret) {
		case EINVAL:
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the "
				  "calling thread's priority is higher than the mutex's current priority ceiling, error EINVAL");
			return false;
		case EAGAIN:
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The mutex could not be acquired, because the maximum number of recursive locks for mutex has "
				  "been exceeded, error EAGAIN");
			return false;
		case EDEADLK:
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": A deadlock condition was detected or the value of mutex is invalid, error EDEADLK");
			return false;
		default:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

template <bool Wr, bool Try> FORCE_INLINE [[nodiscard]] bool MutexRWLock(NamedMutex<pthread_rwlock_t>& namedMutex)
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
		case EBUSY: // trywrlock and tryrdlock
			LOG_DEBUG("Mutex name \"" + namedMutex.name
				+ "\": The read lock could not be acquired because a writer holds the lock, error EBUSY");
			return false;
		case EINVAL: // rdlock, tryrdlock, wrlock and trywrlock
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		case EAGAIN: // rdlock and tryrdlock
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The mutex could not be acquired, because the maximum number of recursive locks for mutex has "
				  "been exceeded, error EAGAIN");
			return false;
		case EDEADLK: // rdlock, wrlock and trywrlock
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": A deadlock condition was detected or the value of mutex is invalid, error EDEADLK");
			return false;
		default:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

template <typename T> FORCE_INLINE [[nodiscard]] bool MutexUnlock(NamedMutex<T>& namedMutex)
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
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": The current thread does not own the mutex, error EPERM");
			return false;
		case EAGAIN: // Only for pthread_mutex_t
			LOG_ERROR("Mutex name \"" + namedMutex.name
				+ "\": The mutex could not be acquired, because the maximum number of recursive locks for mutex has "
				  "been exceeded, error EAGAIN");
			return false;
		case EINVAL:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		default:
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": Unknown error №" + _S(ret));
			return false;
		}
	}

	return true;
}

FORCE_INLINE Guard::Guard(NamedMutex<pthread_mutex_t>& namedMutex) noexcept
	: m_namedMutex{ namedMutex }
{
	(void)MutexLock(namedMutex);
}

FORCE_INLINE Guard::~Guard() noexcept { (void)MutexUnlock(m_namedMutex); }

template <bool Wr>
FORCE_INLINE GuardRW<Wr>::GuardRW(NamedMutex<pthread_rwlock_t>& namedMutex) noexcept
	: m_namedMutex{ namedMutex }
{
	(void)MutexRWLock<Wr, doLock>(m_namedMutex);
}

template <bool Wr> FORCE_INLINE GuardRW<Wr>::~GuardRW() noexcept { (void)MutexUnlock(m_namedMutex); }

FORCE_INLINE Atomic::Guard::Guard(Atomic& atomicLock) noexcept
	: m_atomicLock{ atomicLock }
{
	m_atomicLock.Lock();
}

FORCE_INLINE Atomic::Guard::~Guard() noexcept { m_atomicLock.Unlock(); }

FORCE_INLINE void Atomic::Lock() noexcept
{
	while (m_lock.test_and_set(std::memory_order_acquire)) {
		m_lock.wait(true, std::memory_order_relaxed);
	}
}

FORCE_INLINE bool Atomic::TryLock() noexcept { return !m_lock.test_and_set(std::memory_order_acquire); }

FORCE_INLINE void Atomic::Unlock() noexcept
{
	m_lock.clear(std::memory_order_release);
	m_lock.notify_one();
}

template <bool Wr>
FORCE_INLINE AtomicRW::Guard<Wr>::Guard(AtomicRW& atomicRWLock) noexcept
	: m_atomicRWLock{ atomicRWLock }
{
	if constexpr (Wr) {
		m_atomicRWLock.WriteLock();
	}
	else {
		m_atomicRWLock.ReadLock();
	}
}

template <bool Wr> FORCE_INLINE AtomicRW::Guard<Wr>::~Guard() noexcept
{
	if constexpr (Wr) {
		m_atomicRWLock.WriteUnlock();
	}
	else {
		m_atomicRWLock.ReadUnlock();
	}
}

FORCE_INLINE void AtomicRW::ReadLock() noexcept
{
	if (!m_writeLock.m_lock.test()) {
		m_writeLock.m_lock.wait(true, std::memory_order_relaxed);
	}

	m_lock.fetch_add(1, std::memory_order_acquire);
}

FORCE_INLINE void AtomicRW::ReadUnlock() noexcept
{
	m_lock.fetch_sub(1, std::memory_order_release);
	m_lock.notify_all();
}

FORCE_INLINE void AtomicRW::WriteLock() noexcept
{
	m_writeLock.Lock();
	while (m_lock.load(std::memory_order_acquire) != 0) {
		m_lock.wait(1, std::memory_order_relaxed);
	}
}

FORCE_INLINE void AtomicRW::WriteUnlock() noexcept
{
	m_writeLock.Unlock();
	m_lock.notify_all();
}

} // namespace Lock

} // namespace MSAPI

#endif // MSAPI_LOCK_INL