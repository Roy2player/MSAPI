/**************************
 * @file        lock.inl
 * @date        2026-10-03
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

#ifndef MSAPI_UNIT_TEST_LOCK_INL
#define MSAPI_UNIT_TEST_LOCK_INL

#include "../../../../library/source/help/lock.inl"
#include "../../../../library/source/test/test.inl"
#include <array>
#include <atomic>
#include <barrier>
#include <latch>
#include <thread>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Lock.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool Lock();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool Lock()
{
	static_assert(MSAPI::Lock::WRITE, "Lock \"WRITE\" must be true");
	static_assert(!MSAPI::Lock::READ, "Lock \"READ\" must be false");

	static_assert(MSAPI::Lock::TRY_LOCK, "Lock \"TRY_LOCK\" must be true");
	static_assert(!MSAPI::Lock::DO_LOCK, "Lock \"DO_LOCK\" must be false");

	LOG_INFO("MSAPI UNIT TEST Lock");

	MSAPI::Test::Test t;

	{
		// TryLock distinguishes ownership, and guards unlock on both scope exit and early return
		MSAPI::Lock::Atomic lock;
		RETURN_IF_FALSE(t.Assert(lock.TryLock(), true, "Atomic initially available"));
		RETURN_IF_FALSE(t.Assert(lock.TryLock(), false, "Atomic held try-lock fails"));
		lock.Unlock();
		{
			const MSAPI::Lock::Atomic::Guard _{ lock };
			RETURN_IF_FALSE(t.Assert(lock.TryLock(), false, "Atomic guard owns lock"));
		}
		RETURN_IF_FALSE(t.Assert(lock.TryLock(), true, "Atomic guard unlocks lock"));
		lock.Unlock();
		const auto earlyReturn{ [&lock] {
			const MSAPI::Lock::Atomic::Guard _{ lock };
			return true;
		} };
		RETURN_IF_FALSE(t.Assert(earlyReturn(), true, "Atomic guarded early return"));
		RETURN_IF_FALSE(t.Assert(lock.TryLock(), true, "Atomic early return unlocks lock"));
		lock.Unlock();
	}

	{
		// Serialized writers protect a shared non-atomic counter
		MSAPI::Lock::Atomic lock;
		std::barrier start{ 4 };
		int64_t counter{};
		std::array<std::jthread, 4> workers;

		for (auto& worker : workers) {
			worker = std::jthread{ [&] {
				start.arrive_and_wait();
				for (int32_t iteration{}; iteration < 256; ++iteration) {
					const MSAPI::Lock::Atomic::Guard _{ lock };
					++counter;
				}
			} };
		}

		for (auto& worker : workers) {
			worker.join();
		}

		RETURN_IF_FALSE(t.Assert(counter, 1024, "Atomic serializes writers"));
	}

	{
		// A reader excludes writers until it unlocks; a writer likewise excludes readers
		MSAPI::Lock::AtomicRW lock;
		std::latch writerStarted{ 1 };
		std::atomic<bool> writerEntered{};
		std::jthread writer;
		bool writerBlocked{};
		int32_t published{};

		{
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ lock };
			writer = std::jthread{ [&] {
				writerStarted.count_down();
				const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ lock };
				writerEntered.store(true);
				published = 42;
			} };
			writerStarted.wait();
			writerBlocked = !writerEntered.load();
		}

		writer.join();
		RETURN_IF_FALSE(t.Assert(writerBlocked, true, "AtomicRW reader excludes writer"));
		RETURN_IF_FALSE(t.Assert(writerEntered.load(), true, "AtomicRW writer proceeds after reader"));

		{
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ lock };
			RETURN_IF_FALSE(t.Assert(published, 42, "AtomicRW subsequent reader observes data"));
		}

		std::latch readerStarted{ 1 };
		std::atomic<bool> readerEntered{};
		std::jthread reader;
		bool readerBlocked{};

		{
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ lock };
			reader = std::jthread{ [&] {
				readerStarted.count_down();
				const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ lock };
				readerEntered.store(true);
			} };
			readerStarted.wait();
			readerBlocked = !readerEntered.load();
		}

		reader.join();
		RETURN_IF_FALSE(t.Assert(readerBlocked, true, "AtomicRW writer excludes reader"));
		RETURN_IF_FALSE(t.Assert(readerEntered.load(), true, "AtomicRW reader proceeds after writer"));
	}

	{
		// POSIX wrappers must be initialized before use and destroyed only after all guards finish
		MSAPI::Lock::NamedMutex<pthread_mutex_t> mutex{ "unitMutex" };
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexInit(mutex, nullptr), true, "Initialize named mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexLock(mutex), true, "Lock named mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexUnlock(mutex), true, "Unlock named mutex"));
		{
			const MSAPI::Lock::Guard _{ mutex };
			RETURN_IF_FALSE(t.Assert(pthread_mutex_trylock(&mutex.mutex), EBUSY, "Named mutex guard owns lock"));
		}
		RETURN_IF_FALSE(t.Assert(pthread_mutex_trylock(&mutex.mutex), 0, "Named mutex guard unlocks lock"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexUnlock(mutex), true, "Unlock re-locked named mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexDestroy(mutex), true, "Destroy named mutex"));
	}

	{
		MSAPI::Lock::NamedMutex<pthread_rwlock_t> mutex{ "unitRWMutex" };
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexInit(mutex, nullptr), true, "Initialize named RW mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::READ, MSAPI::Lock::DO_LOCK>(mutex), true,
			"Read-lock named RW mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::WRITE, MSAPI::Lock::TRY_LOCK>(mutex), false,
			"Read lock prevents write try-lock"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexUnlock(mutex), true, "Unlock named RW reader"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::WRITE, MSAPI::Lock::DO_LOCK>(mutex), true,
			"Write-lock named RW mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::READ, MSAPI::Lock::TRY_LOCK>(mutex), false,
			"Write lock prevents read try-lock"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexUnlock(mutex), true, "Unlock named RW writer"));

		{
			const MSAPI::Lock::GuardRW<MSAPI::Lock::READ> _{ mutex };
			RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::WRITE, MSAPI::Lock::TRY_LOCK>(mutex), false,
				"Read guard prevents write try-lock"));
		}

		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::WRITE, MSAPI::Lock::TRY_LOCK>(mutex), true,
			"Read guard unlocks named RW mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexUnlock(mutex), true, "Unlock named RW try-writer"));

		{
			const MSAPI::Lock::GuardRW<MSAPI::Lock::WRITE> _{ mutex };
			RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::READ, MSAPI::Lock::TRY_LOCK>(mutex), false,
				"Write guard prevents read try-lock"));
		}

		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexRWLock<MSAPI::Lock::READ, MSAPI::Lock::TRY_LOCK>(mutex), true,
			"Write guard unlocks named RW mutex"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexUnlock(mutex), true, "Unlock named RW try-reader"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Lock::MutexDestroy(mutex), true, "Destroy named RW mutex"));
	}

	{
		// Allow simultaneous readers while retaining writer exclusion
		MSAPI::Lock::AtomicRW lock;
		std::barrier readersReady{ 2 };
		std::atomic<int32_t> activeReaders{};
		std::atomic<bool> overlappingReaders{};

		const auto reader{ [&] {
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ lock };
			activeReaders.fetch_add(1);
			readersReady.arrive_and_wait();
			if (activeReaders.load() == 2) {
				overlappingReaders.store(true);
			}
			readersReady.arrive_and_wait();
			activeReaders.fetch_sub(1);
		} };

		std::jthread firstReader{ reader };
		std::jthread secondReader{ reader };
		firstReader.join();
		secondReader.join();

		RETURN_IF_FALSE(t.Assert(overlappingReaders.load(), true, "AtomicRW permits concurrent readers"));
		RETURN_IF_FALSE(t.Assert(activeReaders.load(), 0, "AtomicRW readers finished"));
	}

	{
		// Stress reader/writer exclusion and verify that competing writers finish
		MSAPI::Lock::AtomicRW lock;
		std::barrier start{ 4 };
		std::atomic<int32_t> activeReaders{};
		std::atomic<int32_t> activeWriters{};
		std::atomic<int32_t> completed{};
		std::atomic<bool> violation{};
		std::array<std::jthread, 4> workers;

		for (int32_t worker{}; worker < 4; ++worker) {
			workers[UINT64(worker)] = std::jthread{ [&, worker] {
				start.arrive_and_wait();

				for (int32_t iteration{}; iteration < 4096; ++iteration) {
					if (worker < 2) {
						const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ lock };
						activeReaders.fetch_add(1);
						if (activeWriters.load() != 0) {
							violation.store(true);
						}
						activeReaders.fetch_sub(1);
						continue;
					}

					const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ lock };
					if (activeWriters.fetch_add(1) != 0 || activeReaders.load() != 0) {
						violation.store(true);
					}
					completed.fetch_add(1);
					activeWriters.fetch_sub(1);
				}
			} };
		}

		for (auto& worker : workers) {
			worker.join();
		}

		RETURN_IF_FALSE(t.Assert(violation.load(), false, "AtomicRW excludes readers and competing writers"));
		RETURN_IF_FALSE(t.Assert(completed.load(), 8192, "AtomicRW blocked writers make progress"));
		RETURN_IF_FALSE(t.Assert(activeReaders.load(), 0, "AtomicRW stress readers unlocked"));
		RETURN_IF_FALSE(t.Assert(activeWriters.load(), 0, "AtomicRW stress writers unlocked"));
	}

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_LOCK_INL