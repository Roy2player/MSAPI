/**************************
 * @file        lock.inl
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
 * @attention Construction does not initialize the POSIX lock. Call MutexInit before using it and MutexDestroy
 * only after all owners and waiters finish.
 *
 * @tparam T Mutex or rwlock.
 *
 * @concurrency Yes, after initialization. Callers manage initialization and lifetime.
 */
template <MutexT T> struct NamedMutex {
	T mutex;
	const std::string name;

	/**************************
	 * @brief Construct a new Named Mutex object.
	 *
	 * @param name Mutex name.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
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
 * @pre The POSIX lock is not already initialized or in use.
 *
 * @locking Is not required.
 *
 * @return True if mutex initialized successfully, false if any errors occurred.
 *
 * @test Yes.
 *
 * @todo Cover custom attributes and initialization failures.
 */
template <typename T, typename S>
	requires MutexAndParams<T, S>
FORCE_INLINE [[nodiscard]] bool MutexInit(NamedMutex<T>& namedMutex, const S mutexattr);

/**************************
 * @brief Destroys an initialized POSIX mutex and reports errors without locking it.
 *
 * @tparam T Mutex or rwlock.
 *
 * @param namedMutex Named mutex.
 *
 * @pre The mutex is unlocked and has no remaining owners or waiters.
 *
 * @locking Is not required.
 *
 * @return True if mutex destroyed successfully, false if any errors occurred.
 *
 * @test Yes.
 *
 * @todo Cover valid, reproducible destruction failures.
 */
template <typename T> FORCE_INLINE [[nodiscard]] bool MutexDestroy(NamedMutex<T>& namedMutex);

/**************************
 * @brief Lock mutex and print error if any occurred.
 *
 * @param namedMutex Named mutex.
 *
 * @pre The mutex is initialized and recursion follows its POSIX attribute contract.
 *
 * @locking Locks namedMutex.mutex.
 *
 * @return True if mutex locked successfully, false if any errors occurred.
 *
 * @test Yes.
 *
 * @todo Cover recoverable lock errors.
 */
FORCE_INLINE [[nodiscard]] bool MutexLock(NamedMutex<pthread_mutex_t>& namedMutex);

constexpr bool WRITE{ true };
constexpr bool READ{ false };

constexpr bool TRY_LOCK{ true };
constexpr bool DO_LOCK{ false };

/**************************
 * @brief Lock read write mutex and print error if any occurred.
 *
 * @tparam Wr True for write lock, false for read lock.
 * @tparam Try True for try lock, false for lock.
 *
 * @param namedMutex Named mutex.
 *
 * @pre The lock is initialized. Blocking calls must not attempt to upgrade a held read lock.
 *
 * @locking Locks namedMutex.mutex in the requested mode.
 *
 * @return True if mutex locked successfully, false if any errors occurred or try lock and mutex busy.
 *
 * @test Yes.
 *
 * @todo Cover recoverable errors other than a busy try-lock.
 */
template <bool Wr, bool Try> FORCE_INLINE [[nodiscard]] bool MutexRWLock(NamedMutex<pthread_rwlock_t>& namedMutex);

/**************************
 * @brief Try unlock mutex and print error if any occurred.
 *
 * @tparam T Mutex or rwlock.
 *
 * @param namedMutex Named mutex.
 *
 * @pre The calling thread owns the initialized lock.
 *
 * @locking Unlocks namedMutex.mutex.
 *
 * @return True if mutex is unlocked successfully, false if any errors occurred.
 *
 * @test Yes.
 *
 * @todo Cover valid, reproducible unlock failures.
 */
template <typename T> FORCE_INLINE [[nodiscard]] bool MutexUnlock(NamedMutex<T>& namedMutex);

/**************************
 * @brief RAII guard that locks the mutex on construction and unlocks it on destruction.
 *
 * @attention The initialized named mutex outlives the guard. Guard lifetime defines the protected access scope.
 *
 * @concurrency Yes. Follows the underlying initialized POSIX mutex contract.
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
	 * @pre The named mutex is initialized and can be locked by the calling thread.
	 *
	 * @locking Locks namedMutex.mutex.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Guard(NamedMutex<pthread_mutex_t>& namedMutex) noexcept;

	Guard(const Guard&) = delete;
	Guard(Guard&&) = delete;
	Guard& operator=(const Guard&) = delete;
	Guard& operator=(Guard&&) = delete;

	/**************************
	 * @brief Destroy the Guard object, unlock mutex.
	 *
	 * @locking Unlocks the named mutex owned by this guard.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE ~Guard() noexcept;
};

class AtomicRW;

/**************************
 * @brief RAII guard that locks the read/write mutex on construction and unlocks it on destruction.
 *
 * @tparam Wr True for write lock, false for read lock.
 *
 * @concurrency Yes. Follows the initialized POSIX RW-lock contract; the named mutex outlives the guard.
 */
template <bool Wr> class GuardRW {
private:
	NamedMutex<pthread_rwlock_t>& m_namedMutex;

public:
	/**************************
	 * @brief Construct a new Guard RW object, lock mutex.
	 *
	 * @param namedMutex Initialized named RW mutex.
	 *
	 * @pre The calling thread can lock the requested mode without a read-to-write upgrade.
	 *
	 * @locking Locks namedMutex.mutex in the Wr mode.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE GuardRW(NamedMutex<pthread_rwlock_t>& namedMutex) noexcept;

	GuardRW(const GuardRW&) = delete;
	GuardRW(GuardRW&&) = delete;
	GuardRW& operator=(const GuardRW&) = delete;
	GuardRW& operator=(GuardRW&&) = delete;

	/**************************
	 * @brief Destroy the Guard RW object, unlock mutex.
	 *
	 * @locking Unlocks the named RW mutex owned by this guard.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE ~GuardRW() noexcept;
};

/**************************
 * @brief Atomic lock based on std::atomic_flag.
 *
 * @attention Locking is nonrecursive. Owners unlock before re-locking it; all users finish before
 * destruction. No fairness or starvation-freedom guarantee is provided.
 *
 * @concurrency Yes. Locking uses acquire memory ordering and unlocking uses release memory ordering to publish
 * protected writes to subsequent successful lock operations.
 */
class Atomic {
public:
	/**************************
	 * @brief RAII guard that locks the atomic lock on construction and unlocks it on destruction.
	 *
	 * @concurrency Yes. The owning lock outlives the guard; guard lifetime defines exclusive protected access.
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
		 * @locking Locks atomicLock without recursive ownership.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE Guard(Atomic& atomicLock) noexcept;

		Guard(const Guard&) = delete;
		Guard(Guard&&) = delete;
		Guard& operator=(const Guard&) = delete;
		Guard& operator=(Guard&&) = delete;

		/**************************
		 * @brief Destroy the Guard object, unlock atomic lock.
		 *
		 * @locking Unlocks the lock owned by this guard.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE ~Guard() noexcept;
	};

private:
	std::atomic_flag m_lock{};

public:
	FORCE_INLINE Atomic() noexcept = default;

	Atomic(const Atomic&) = delete;
	Atomic(Atomic&&) = delete;
	Atomic& operator=(const Atomic&) = delete;
	Atomic& operator=(Atomic&&) = delete;

	/**************************
	 * @brief Wait for lock is false and set it to true.
	 *
	 * @pre The calling thread does not already own the lock.
	 *
	 * @locking Locks exclusively using acquire memory ordering.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Lock() noexcept;

	/**************************
	 * @brief Try to set lock to true.
	 *
	 * @locking Attempts to lock exclusively without waiting, using acquire memory ordering. Failure leaves
	 * existing ownership unchanged.
	 *
	 * @return True if lock was false and now is true, false if lock was true.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE bool TryLock() noexcept;

	/**************************
	 * @brief Set lock to false and notify one thread.
	 *
	 * @pre The calling thread owns the lock.
	 *
	 * @locking Unlocks using release memory ordering. Notification does not select a guaranteed next owner.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Unlock() noexcept;

	// Allow AtomicRW to access private members
	friend class AtomicRW;
};

/**************************
 * @brief Atomic read/write lock based on std::atomic and Atomic for write operations.
 *
 * @attention Locking is nonrecursive and read-to-write upgrades are not supported. The lock outlives all
 * owners and waiters. No fairness or starvation-freedom guarantee is provided.
 *
 * @concurrency Yes. Locking uses acquire memory ordering and unlocking uses release memory ordering to publish
 * protected writes to subsequent successful lock operations.
 */
class AtomicRW {
public:
	/**************************
	 * @brief RAII guard that locks the atomic read/write lock on construction and unlocks it on destruction.
	 *
	 * @tparam Wr True for write lock, false for read lock.
	 *
	 * @concurrency Yes. Guard lifetime defines protected access; the owning lock outlives the guard.
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
		 * @pre The calling thread does not already hold this lock in either mode.
		 *
		 * @locking Locks atomicRWLock in the Wr mode.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE Guard(AtomicRW& atomicRWLock) noexcept;

		Guard(const Guard&) = delete;
		Guard(Guard&&) = delete;
		Guard& operator=(const Guard&) = delete;
		Guard& operator=(Guard&&) = delete;

		/**************************
		 * @brief Destroy the Guard object, unlock atomic read/write lock.
		 *
		 * @locking Unlocks the mode owned by this guard.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE ~Guard() noexcept;
	};

private:
	std::atomic<int32_t> m_lock{};
	Atomic m_writeLock;

public:
	FORCE_INLINE AtomicRW() noexcept = default;

	AtomicRW(const AtomicRW&) = delete;
	AtomicRW(AtomicRW&&) = delete;
	AtomicRW& operator=(const AtomicRW&) = delete;
	AtomicRW& operator=(AtomicRW&&) = delete;

	/**************************
	 * @brief Lock for read, wait if write lock is set.
	 *
	 * @pre The calling thread does not already hold this lock in either mode.
	 *
	 * @locking Briefly locks the writer gate to register a reader, then keeps the read lock held.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void ReadLock() noexcept;

	/**************************
	 * @brief Unlock for read and notify one thread.
	 *
	 * @pre The calling thread holds a read lock.
	 *
	 * @locking Unlocks one reader using release memory ordering and notifies a waiting writer.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void ReadUnlock() noexcept;

	/**************************
	 * @brief Locks for writing, waiting for another writer to unlock and for all readers to unlock.
	 *
	 * @pre The calling thread does not already hold this lock in either mode.
	 *
	 * @locking Locks the writer gate and waits for registered readers to unlock, then keeps the write lock held.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void WriteLock() noexcept;

	/**************************
	 * @brief Unlock for write and notify all threads.
	 *
	 * @pre The calling thread holds exclusive ownership.
	 *
	 * @locking Unlocks the writer gate using release memory ordering and notifies its waiters.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void WriteUnlock() noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
NamedMutex
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
	int32_t ret{ -1 };
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
	int32_t ret{ -1 };
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
			LOG_ERROR_NEW("Mutex name \"{}\": The mutex could not be locked, because the maximum number of recursive "
						  "locks for mutex has been exceeded, error EAGAIN",
				namedMutex.name);
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
	int32_t ret{ -1 };
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
			if constexpr (Wr) {
				LOG_DEBUG_NEW("Mutex name \"{}\": The write lock could not be locked because a reader or a writer "
							  "holds the lock, error EBUSY",
					namedMutex.name);
			}
			else {
				LOG_DEBUG_NEW("Mutex name \"{}\": The read lock could not be locked because a writer holds the lock, "
							  "error EBUSY",
					namedMutex.name);
			}
			return false;
		case EINVAL: // rdlock, tryrdlock, wrlock and trywrlock
			LOG_ERROR("Mutex name \"" + namedMutex.name + "\": The value specified by mutex is invalid, error EINVAL");
			return false;
		case EAGAIN: // rdlock and tryrdlock
			LOG_ERROR_NEW("Mutex name \"{}\": The mutex could not be locked, because the maximum number of recursive "
						  "locks for mutex has been exceeded, error EAGAIN",
				namedMutex.name);
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
	int32_t ret{ -1 };
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
			LOG_ERROR_NEW("Mutex name \"{}\": The mutex could not be unlocked, because the maximum number of recursive "
						  "locks for mutex has been exceeded, error EAGAIN",
				namedMutex.name);
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

/*---------------------------------------------------------------------------------
NamedMutex::Guard
---------------------------------------------------------------------------------*/

FORCE_INLINE Guard::Guard(NamedMutex<pthread_mutex_t>& namedMutex) noexcept
	: m_namedMutex{ namedMutex }
{
	(void)MutexLock(namedMutex);
}

FORCE_INLINE Guard::~Guard() noexcept { (void)MutexUnlock(m_namedMutex); }

/*---------------------------------------------------------------------------------
NamedMutex::GuardRW
---------------------------------------------------------------------------------*/

template <bool Wr>
FORCE_INLINE GuardRW<Wr>::GuardRW(NamedMutex<pthread_rwlock_t>& namedMutex) noexcept
	: m_namedMutex{ namedMutex }
{
	(void)MutexRWLock<Wr, DO_LOCK>(m_namedMutex);
}

template <bool Wr> FORCE_INLINE GuardRW<Wr>::~GuardRW() noexcept { (void)MutexUnlock(m_namedMutex); }

/*---------------------------------------------------------------------------------
Atomic::Guard
---------------------------------------------------------------------------------*/

FORCE_INLINE Atomic::Guard::Guard(Atomic& atomicLock) noexcept
	: m_atomicLock{ atomicLock }
{
	m_atomicLock.Lock();
}

FORCE_INLINE Atomic::Guard::~Guard() noexcept { m_atomicLock.Unlock(); }

/*---------------------------------------------------------------------------------
Atomic
---------------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------------
AtomicRW::Guard
---------------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------------
AtomicRW
---------------------------------------------------------------------------------*/

FORCE_INLINE void AtomicRW::ReadLock() noexcept
{
	m_writeLock.Lock();
	m_lock.fetch_add(1, std::memory_order_acquire);
	m_writeLock.Unlock();
}

FORCE_INLINE void AtomicRW::ReadUnlock() noexcept
{
	m_lock.fetch_sub(1, std::memory_order_release);
	m_lock.notify_one();
}

FORCE_INLINE void AtomicRW::WriteLock() noexcept
{
	m_writeLock.Lock();
	auto readers{ m_lock.load(std::memory_order_acquire) };

	while (readers != 0) {
		m_lock.wait(readers, std::memory_order_relaxed);
		readers = m_lock.load(std::memory_order_acquire);
	}
}

FORCE_INLINE void AtomicRW::WriteUnlock() noexcept
{
	m_writeLock.m_lock.clear(std::memory_order_release);
	m_writeLock.m_lock.notify_all();
}

} // namespace Lock

} // namespace MSAPI

#endif // MSAPI_LOCK_INL