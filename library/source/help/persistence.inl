/**************************
 * @file        persistence.inl
 * @date        2026-09-21
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
 * @brief Functional abstraction to keep a single-type container persisted in binary format on disk.
 */

#ifndef MSAPI_PERSISTENCE_INL
#define MSAPI_PERSISTENCE_INL

#include "basicSString.inl"
#include "io.inl"
#include "lock.inl"
#include "log.h"
#include <algorithm>

namespace MSAPI {

namespace Persistence {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Stores payload wrappers in memory and incrementally persists uncached wrappers alongside a manual timestamp.
 *
 * @attention Data file and timestamp file paths are derived once from constructor arguments and never change.
 * Separate instances sharing the same files require external synchronization.
 *
 * @note Each data file record is a raw copy of the Object wrapper: payload, cached status, and padding, so the record
 * size is sizeof(Object). The stored cached status is not meaningful and is overwritten on Load. Any change of the
 * Object layout makes existing data files unreadable. The timestamp file holds a raw copy of MSAPI::Timer.
 *
 * @tparam Container Sequence container with emplace_back support.
 * @tparam Type Type of the objects stored in the container.
 *
 * @concurrency Yes. Internally locking methods must not be called while holding GetLock. Destruction requires
 * all callers and guards to have finished.
 *
 * @purging Objects accumulate in memory through EmplaceBack and Load and are never released automatically. Clear
 * empties the container and makes the next Save rewrite the data file with the objects stored by then, so objects
 * cleared from memory are also removed from the data file.
 */
template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
class Single {
public:
	/**************************
	 * @brief Holds a payload and its in-memory persistence status. Ordering and equality ignore the status.
	 *
	 * @concurrency No.
	 */
	class Object {
	private:
		Type m_object;
		bool m_isCached{};

	public:
		/**************************
		 * @brief Constructs an Object with the given payload.
		 *
		 * @param object The payload to store.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit Object(const Type& object) noexcept
			: m_object{ object }
		{
		}

		/**************************
		 * @brief Constructs an Object with the given payload using move semantics.
		 *
		 * @param object The payload to store.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit Object(Type&& object) noexcept
			: m_object{ std::move(object) }
		{
		}

		FORCE_INLINE Object(const Object&) noexcept = default;
		FORCE_INLINE Object(Object&&) noexcept = default;
		FORCE_INLINE Object& operator=(const Object&) noexcept = default;
		FORCE_INLINE Object& operator=(Object&&) noexcept = default;

		/**************************
		 * @return Const reference to the payload.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const Type& Get() const noexcept { return m_object; }

		/**************************
		 * @return True if the payload has already been persisted.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsCached() const noexcept { return m_isCached; }

		/**************************
		 * @brief Compares two objects for equality based on their payloads, ignoring the cached status.
		 *
		 * @param other The other object to compare with.
		 *
		 * @return True if the payloads are equal, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool operator==(const Object& other) const noexcept
		{
			return m_object == other.m_object;
		}

		/**************************
		 * @brief Compares two objects for ordering based on their payloads, ignoring the cached status.
		 *
		 * @param other The other object to compare with.
		 *
		 * @return True if the payload of this object is less than the other's payload, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool operator<(const Object& other) const noexcept
		{
			return m_object < other.m_object;
		}

	private:
		/**************************
		 * @brief Sets the cached status of the object.
		 *
		 * @param isCached The new cached status.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void SetCached(const bool isCached) noexcept { m_isCached = isCached; }

		// To manage the cached status
		friend class Single;
	};

private:
	Container<Object> m_container;
	MSAPI::Timer m_timestamp{ 0 };
	MSAPI::SString<512> m_path;
	MSAPI::SString<512> m_timestampPath;
	mutable MSAPI::Lock::AtomicRW m_lock;
	bool m_fullRewrite{};

public:
	/**************************
	 * @brief Normalizes the directory, creates it with missing parents, and stores the data and timestamp file paths.
	 * The data file path is "dir/.name" and the timestamp file path is "dir/.name_timestamp".
	 *
	 * @attention Empty directory, empty name, name with '/', path exceeding capacity, or failed directory creation
	 * leave the paths empty, and Load and Save fail.
	 *
	 * @param dir Directory containing the data and timestamp files.
	 * @param name Base name of the data and timestamp files.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Single(const std::string_view dir, const std::string_view name) noexcept
	{
		if (dir.empty() || name.empty() || name.find('/') != std::string_view::npos) [[unlikely]] {
			LOG_WARNING_NEW("Cannot initialize persistence, directory: \"{}\", name: \"{}\"", dir, name);
			return;
		}

		constexpr std::string_view SUFFIX{ "_timestamp" };
		constexpr auto CAPACITY{ std::min(m_path.GetCapacity(), m_timestampPath.GetCapacity()) };

		const bool addSeparator{ dir.back() != '/' };

		// Reserve space for the null terminator of the longest path
		if (dir.size() + addSeparator + 1 /* dot */ + name.size() + SUFFIX.size() >= CAPACITY) [[unlikely]] {
			LOG_ERROR_NEW(
				"Persistence path exceeds capacity: {}, directory: \"{}\", name: \"{}\"", CAPACITY, dir, name);
			return;
		}

		m_path = dir;
		if (addSeparator) {
			m_path += "/";
		}

		if (!m_path.NullTerminate() || (!MSAPI::IO::HasPath(m_path.Get()) && !MSAPI::IO::CreateDir(m_path.Get())))
			[[unlikely]] {

			m_path.Clear();
			return;
		}

		m_path += ".";
		m_path += name;

		m_timestampPath = m_path;
		m_timestampPath += SUFFIX;

		if (!m_path.NullTerminate() || !m_timestampPath.NullTerminate()) [[unlikely]] {
			LOG_ERROR_NEW("Failed to null terminate persistence paths, path: \"{}\"", m_path.Get());
			m_path.Clear();
			m_timestampPath.Clear();
			return;
		}
	}

	Single(const Single&) = delete;
	Single(Single&&) = delete;
	Single& operator=(const Single&) = delete;
	Single& operator=(Single&&) = delete;

	/**************************
	 * @brief Loads all records of the data file into an empty container and marks them as cached, then reads the
	 * timestamp file if it exists.
	 *
	 * @attention Missing data file is a failure. Timestamp is left at its previous value if the timestamp file does not
	 * exist. Failed reads leave the container empty and the timestamp unchanged. A full rewrite requested by Clear
	 * stays pending, so the next Save rewrites the data file with the loaded objects.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @return True if all records are loaded, false otherwise or if the container is not empty.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Load() noexcept
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };
		if (m_path.Empty()) [[unlikely]] {
			LOG_WARNING_NEW("Persistence path is empty, records: {}", m_container.size());
			return false;
		}

		if (!m_container.empty()) [[unlikely]] {
			LOG_WARNING_NEW(
				"Interrupt the attempt to load records into a nonempty container, path: \"{}\"", m_path.Get());
			return false;
		}

		// Records read before a failure must not stay in memory
		if (!MSAPI::IO::ReadBinaries(m_container, m_path.Get())) [[unlikely]] {
			m_container.clear();
			return false;
		}

		if (MSAPI::IO::HasPath(m_timestampPath.Get())) {
			MSAPI::Timer timestamp{ 0 };
			if (!MSAPI::IO::ReadBinary(&timestamp, m_timestampPath.Get())) [[unlikely]] {
				m_container.clear();
				return false;
			}

			m_timestamp = timestamp;
		}

		for (auto& object : m_container) {
			// Friend access
			object.SetCached(true);
		}

		LOG_DEBUG_NEW("Records: {}, from path: {} are loaded successfully", m_container.size(), m_path.Get());
		return true;
	}

	/**************************
	 * @brief Appends uncached objects to the data file and marks them as cached, then saves the timestamp. After Clear,
	 * rewrites the data file with all objects instead.
	 *
	 * @attention If appending fails, no object is marked as cached, while records written before the failure stay in
	 * the data file and a partially written record is not removed. A retry appends these objects again, and a partial
	 * record misaligns all following records, so Load fails. Recovery requires Clear and a successful full rewrite of
	 * the objects. A failed full rewrite stays pending and is repeated by the next Save. If only the timestamp save
	 * fails, appended objects stay cached and a retry saves only the timestamp.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @return True if both data and timestamp were saved successfully, false otherwise.
	 *
	 * @test Yes.
	 *
	 * @todo Make IO::SaveBinaries return std::expected with true on success and the number of written records on
	 * failure, to mark written objects as cached and to truncate a partially written record.
	 */
	FORCE_INLINE [[nodiscard]] bool Save() noexcept
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };

		if (m_path.Empty()) [[unlikely]] {
			LOG_WARNING_NEW("Persistence path is empty, records: {}", m_container.size());
			return false;
		}

		if (m_fullRewrite) {
			if (!IO::SaveBinaries<IO::OVERWRITE>(m_container, m_path.Get())) [[unlikely]] {
				return false;
			}

			m_fullRewrite = false;

			for (auto& object : m_container) {
				// Friend access
				object.SetCached(true);
			}

			LOG_DEBUG_NEW("Records at: {} fully rewritten to: {}, size: {}", m_timestamp.ToString(), m_path.Get(),
				m_container.size());
		}
		else {
			const auto end{ m_container.end() };
			auto begin{ std::find_if(m_container.begin(), end, [](const auto& object) { return !object.IsCached(); }) };

			if (!IO::SaveBinaries<IO::APPEND>(begin, end, m_path.Get())) [[unlikely]] {
				return false;
			}

			LOG_DEBUG_NEW("Records at: {} appended to: {}, new entries: {}", m_timestamp.ToString(), m_path.Get(),
				std::distance(begin, end));

			while (begin != end) {
				// Friend access
				begin->SetCached(true);
				++begin;
			}
		}

		return IO::SaveBinary(m_timestamp, m_timestampPath.Get());
	}

	/**************************
	 * @locking Read lock m_lock.
	 *
	 * @return Timestamp of the last modification.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] MSAPI::Timer GetTimestamp() const noexcept
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ m_lock };
		return m_timestamp;
	}

	/**************************
	 * @brief Updates the timestamp of the last modification manually, independently of Load and Save calls.
	 *
	 * @param timestamp New timestamp value.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void UpdateTimestamp(const MSAPI::Timer timestamp) noexcept
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };
		m_timestamp = timestamp;
	}

	/**************************
	 * @locking Requires a caller-held GetLock read guard throughout lookup, iteration, and copying. References and
	 * iterators must not escape into unsynchronized use.
	 *
	 * @return Const reference to the wrapper container.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] const Container<Object>& Get() const noexcept { return m_container; }

	/**************************
	 * @locking Is not required.
	 *
	 * @return Internal lock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] MSAPI::Lock::AtomicRW& GetLock() const noexcept { return m_lock; }

	/**************************
	 * @brief Emplaces an uncached object at the end of the container.
	 *
	 * @attention Allocation failure of the container terminates the process.
	 *
	 * @tparam ObjectType Type of the object to be emplaced.
	 *
	 * @param object Object to be emplaced.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @test Yes.
	 */
	template <typename ObjectType> FORCE_INLINE void EmplaceBack(ObjectType&& object) noexcept
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };
		(void)m_container.emplace_back(std::forward<ObjectType>(object));
	}

	/**************************
	 * @brief Removes all objects and requests a full rewrite on the next Save. Timestamp and paths are left untouched.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Clear() noexcept
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };
		m_container.clear();
		m_fullRewrite = true;
	}
};

} // namespace Persistence

} // namespace MSAPI

#endif // MSAPI_PERSISTENCE_INL
