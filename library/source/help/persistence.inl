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
 * @brief Functional abstractions to keep containers persisted in binary format on disk: Single keeps a sequence of
 * values, Pair keeps values by keys with modifications. Common logic is shared by Base.
 */

#ifndef MSAPI_PERSISTENCE_INL
#define MSAPI_PERSISTENCE_INL

#include "basicSString.inl"
#include "io.inl"
#include "lock.inl"
#include "log.h"
#include <algorithm>
#include <format>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace MSAPI {

namespace Persistence {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Common part of persistences: data and timestamp file paths, timestamp of the last change, lock, Read and
 * Save. Records are saved to the data file in append mode, so saved records are never removed from the data file.
 * Records handling is provided by Derived at compile time.
 *
 * @attention Data file and timestamp file paths are derived once from constructor arguments and never change.
 *
 * @note Each data file record is a raw copy of the Derived object wrapper, so the record size is the size of the
 * wrapper. The stored saved status is not meaningful and is overwritten on Read. Any change of the wrapper layout
 * makes existing data files unreadable. The timestamp file holds a raw copy of MSAPI::Timer.
 * @note After Save with CLEAR policy the container is empty, while its objects stay in the data file. Read must be
 * called before the next change.
 * @note Data and timestamp files are opened on construction and stay open until destruction, so reading and saving do
 * not open files. Files are bound to the opened descriptors: if a file is renamed or removed while the object exists,
 * reading and saving continue with the renamed or removed file and a new file at the path is not used.
 *
 * @tparam Derived Persistence type, which provides records handling.
 *
 * @concurrency Yes.
 *
 * @todo Purge mechanism should be considered to release memory automatically when memory usage is high, e.g. by
 * releasing already persisted objects. That is not clear yet who should trigger it and how that can be handled.
 */
template <typename Derived> class Base {
public:
	// Save policy: keep objects in the container after saving
	static inline constexpr bool REMAIN{ true };
	// Save policy: clear the container after saving
	static inline constexpr bool CLEAR{};
	// Emplacement policy: save the emplaced object right after emplacement
	static inline constexpr bool SAVE_IMMEDIATE{ true };
	// Emplacement policy: defer saving of the emplaced object until Save
	static inline constexpr bool SAVE_DELAY{};

protected:
	MSAPI::Timer m_timestamp{ 0 };
	MSAPI::SString<512> m_path;
	MSAPI::SString<512> m_timestampPath;
	// Opened for reading and for saving in append mode
	MSAPI::IO::FileGuard m_file;
	// Opened for reading and writing, overwritten on each save
	MSAPI::IO::FileGuard m_timestampFile;
	mutable MSAPI::Lock::AtomicRW m_lock;

public:
	/**************************
	 * @brief Normalizes the directory, creates it with missing parents, stores the data and timestamp file paths and
	 * opens both files for reading and writing, creating them empty if missing and keeping existing content. The data
	 * file path is "dir/.name" and the timestamp file path is "dir/.name_timestamp".
	 *
	 * @attention Empty directory, empty name, name with '/', path exceeding capacity, failed directory creation or
	 * failed opening of a file leave the paths empty, and Read, Save and emplacement with SAVE_IMMEDIATE policy fail.
	 *
	 * @param dir Directory containing the data and timestamp files.
	 * @param name Base name of the data and timestamp files.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Base(std::string_view dir, std::string_view name) noexcept;

protected:
	// Protected to forbid destruction via pointer to Base
	/**************************
	 * @brief Destroy the Base object.
	 *
	 * @locking Is not required.
	 */
	FORCE_INLINE ~Base() noexcept = default;

public:
	Base(const Base&) = delete;
	Base(Base&&) = delete;
	Base& operator=(const Base&) = delete;
	Base& operator=(Base&&) = delete;

	/**************************
	 * @brief Reads all records of the data file into an empty container and marks them as saved, then reads the
	 * timestamp file if it exists and is not empty.
	 *
	 * @attention Files opened on construction are read, see the class note. Empty data file has no records. Timestamp
	 * is left at its previous value if the timestamp file is empty, as it is created empty on construction. Failed
	 * reads, including invalid file descriptors after failed construction, leave the container empty and the timestamp
	 * unchanged.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @return True if all records are read, false otherwise or if the container is not empty.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Read() noexcept;

	/**************************
	 * @brief Saves unsaved objects to the data file and marks them as saved, then saves the timestamp. With CLEAR
	 * policy clears the container after successful saving to release memory.
	 *
	 * @attention If saving of objects fails, no object is marked as saved, while records written before the failure
	 * stay in the data file and a partially written record is not removed. A retry saves these objects again, and a
	 * partial record misaligns all following records, so Read fails and the data file requires manual repair. If only
	 * the timestamp save fails, saved objects stay saved and a retry saves only the timestamp. The container is cleared
	 * by CLEAR policy only if both data and timestamp are saved, so no object is lost on failure.
	 *
	 * @tparam Policy REMAIN to keep objects in the container, CLEAR to clear the container after saving.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @return True if both data and timestamp were saved successfully, false otherwise.
	 *
	 * @test Yes.
	 *
	 * @todo Make IO::SaveBinaries return std::expected with true on success and the number of written records on
	 * failure, to mark written objects as saved and to truncate a partially written record.
	 */
	template <bool Policy> FORCE_INLINE [[nodiscard]] bool Save() noexcept;

	/**************************
	 * @locking Read lock m_lock.
	 *
	 * @return Timestamp of the last change, or the timestamp restored by Read if nothing is changed after it.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] MSAPI::Timer GetTimestamp() const noexcept;

	/**************************
	 * @locking Is not required.
	 *
	 * @return Internal lock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] MSAPI::Lock::AtomicRW& GetLock() const noexcept;

protected:
	/**************************
	 * @brief Saves one record to the data file, used by emplacement with SAVE_IMMEDIATE policy.
	 *
	 * @attention Partially written record is not removed, see Save.
	 *
	 * @tparam Record Type of the record, wrapper of Derived.
	 *
	 * @param record Record to save.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @return True if the record is saved, false otherwise.
	 *
	 * @test Yes.
	 */
	template <typename Record> FORCE_INLINE [[nodiscard]] bool SaveRecord(const Record& record) noexcept;

	/**************************
	 * @brief Overwrites the timestamp file with the timestamp.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @return True if the timestamp is saved, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SaveTimestamp() noexcept;

private:
	/**************************
	 * @locking Is not required.
	 *
	 * @return Reference to the derived persistence.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] Derived& Self() noexcept;
};

/**************************
 * @brief Stores payload wrappers in a sequence container and incrementally saves unsaved wrappers to the data file
 * alongside the timestamp of the last emplaced object.
 *
 * @tparam Container Sequence container with emplace_back support.
 * @tparam Type Type of the objects stored in the container.
 *
 * @concurrency Yes.
 *
 * @purging Objects accumulate in memory through EmplaceBack and Read and are released only by Save with CLEAR policy.
 */
template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
class Single : public Base<Single<Container, Type>> {
private:
	using BaseT = Base<Single<Container, Type>>;

public:
	/**************************
	 * @brief Holds a payload and its in-memory persistence status. Ordering and equality ignore the status.
	 *
	 * @concurrency No.
	 */
	class Object {
	private:
		Type m_object;
		bool m_isSaved{};

	public:
		/**************************
		 * @brief Constructs an Object with the given payload.
		 *
		 * @param object The payload to store.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit Object(const Type& object) noexcept;

		/**************************
		 * @brief Constructs an Object with the given payload using move semantics.
		 *
		 * @param object The payload to store.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE explicit Object(Type&& object) noexcept;

		FORCE_INLINE Object(const Object&) noexcept = default;
		FORCE_INLINE Object(Object&&) noexcept = default;
		FORCE_INLINE Object& operator=(const Object&) noexcept = default;
		FORCE_INLINE Object& operator=(Object&&) noexcept = default;

		/**************************
		 * @return Const reference to the payload.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const Type& Get() const noexcept;

		/**************************
		 * @return True if the payload has already been persisted.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsSaved() const noexcept;

		/**************************
		 * @brief Compares two objects for equality based on their payloads, ignoring the saved status.
		 *
		 * @param other The other object to compare with.
		 *
		 * @return True if the payloads are equal, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool operator==(const Object& other) const noexcept;

		/**************************
		 * @brief Compares two objects for ordering based on their payloads, ignoring the saved status.
		 *
		 * @param other The other object to compare with.
		 *
		 * @return True if the payload of this object is less than the other's payload, false otherwise.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool operator<(const Object& other) const noexcept;

	private:
		/**************************
		 * @brief Sets the saved status of the object.
		 *
		 * @param isSaved The new saved status.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void SetSaved(bool isSaved) noexcept;

		// To manage the saved status
		friend class Single;
	};

private:
	Container<Object> m_container;

public:
	/**************************
	 * @brief Construct a new Single object.
	 *
	 * @param dir Directory containing the data and timestamp files.
	 * @param name Base name of the data and timestamp files.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Single(std::string_view dir, std::string_view name) noexcept;

	Single(const Single&) = delete;
	Single(Single&&) = delete;
	Single& operator=(const Single&) = delete;
	Single& operator=(Single&&) = delete;

	/**************************
	 * @locking Requires a caller-held GetLock read guard throughout lookup, iteration, and copying. References and
	 * iterators must not escape into unsynchronized use.
	 *
	 * @return Const reference to the wrapper container.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] const Container<Object>& Get() const noexcept;

	/**************************
	 * @brief Emplaces an unsaved object at the end of the container and updates the timestamp to the current time.
	 * With SAVE_IMMEDIATE policy saves the emplaced object to the data file, marks it as saved and saves the timestamp
	 * under the same lock. Objects emplaced before with SAVE_DELAY policy stay unsaved until Save.
	 *
	 * @attention Allocation failure of the container terminates the process.
	 * @attention If saving of the object fails, the object stays in the container as unsaved and is saved by the next
	 * Save. If only the timestamp save fails, the object stays saved and the next Save saves the timestamp, see Save
	 * for failure details.
	 * @attention With SAVE_IMMEDIATE policy the emplaced object is saved before unsaved objects emplaced earlier, so
	 * the order of records in the data file, and the order after Read, differs from the order of emplacement.
	 *
	 * @tparam SavePolicy SAVE_IMMEDIATE to save right after emplacement, SAVE_DELAY to defer saving.
	 * @tparam ObjectType Type of the object to be emplaced.
	 *
	 * @param object Object to be emplaced.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @return True if the object is emplaced and, with SAVE_IMMEDIATE policy, saved with the timestamp. False if saving
	 * failed.
	 *
	 * @test Yes.
	 */
	template <bool SavePolicy, typename ObjectType>
	FORCE_INLINE [[nodiscard]] bool EmplaceBack(ObjectType&& object) noexcept;

private:
	/**************************
	 * @locking Requires caller-held read lock m_lock.
	 *
	 * @return True if the container is empty, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool IsEmptyImpl() const noexcept;

	/**************************
	 * @brief Reads all records of the data file into the container and marks them as saved.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @return True if all records are read, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool ReadImpl() noexcept;

	/**************************
	 * @brief Saves unsaved objects to the data file and marks them as saved. Unsaved objects are always at the end of
	 * the container, mixed with objects saved by emplacement with SAVE_IMMEDIATE policy, which are skipped.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @return True if objects are saved, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SaveImpl() noexcept;

	/**************************
	 * @brief Clears the container.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void ClearImpl() noexcept;

	// Records handling for Read and Save
	friend BaseT;
};

/**************************
 * @brief Stores values by keys in a map and incrementally saves emplaced and modified wrappers to the data
 * file alongside the timestamp of the last change. Each wrapper keeps the key, the value, the create time and the
 * modify time.
 *
 * @attention Emplace of an existing key modifies its value and saves a new record of the key, so the data file
 * contains a record per emplacement and per saved modification. Read replays the records in order and the last record
 * of a key wins. Several modifications of a key with SAVE_DELAY policy between saves are saved as one record, each
 * emplacement with SAVE_IMMEDIATE policy is saved as a separate record.
 *
 * After Save with CLEAR policy Read is required before the next Emplace, otherwise an already persisted key is emplaced
 * as a new one and its create time is lost on the next Read.
 *
 * @note Order of records in the data file is the order of emplacements and saved modifications, it does not depend
 * on the order of objects in the map.
 *
 * @tparam Map Associative container with find, try_emplace and insert_or_assign support: std::map when objects are
 * required to be ordered by keys, std::unordered_map otherwise.
 * @tparam Key Type of the keys, satisfying requirements of the Map keys.
 * @tparam Value Type of the values.
 *
 * @concurrency Yes.
 *
 * @purging Objects accumulate in memory through Emplace and Read and are released only by Save with CLEAR policy.
 */
template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
class Pair : public Base<Pair<Map, Key, Value>> {
private:
	using BaseT = Base<Pair<Map, Key, Value>>;

public:
	/**************************
	 * @brief Holds a key, a value, the create time and the modify time of the value, and its in-memory persistence
	 * status.
	 *
	 * @concurrency No.
	 */
	class Object {
	private:
		Key m_key;
		Value m_value;
		MSAPI::Timer m_createTime;
		MSAPI::Timer m_modifyTime;
		bool m_isSaved{};

	public:
		/**************************
		 * @brief Constructs an Object of emplaced value, the modify time is equal to the create time.
		 *
		 * @tparam ValueType Type of the value, Value is constructible from it.
		 *
		 * @param key Key of the value.
		 * @param value Value, forwarded to be moved if possible.
		 * @param createTime Create time.
		 *
		 * @test Yes.
		 */
		template <typename ValueType>
			requires std::is_constructible_v<Value, ValueType&&>
		FORCE_INLINE Object(const Key& key, ValueType&& value, MSAPI::Timer createTime) noexcept;

		FORCE_INLINE Object(const Object&) noexcept = default;
		FORCE_INLINE Object(Object&&) noexcept = default;
		FORCE_INLINE Object& operator=(const Object&) noexcept = default;
		FORCE_INLINE Object& operator=(Object&&) noexcept = default;

		/**************************
		 * @return Const reference to the key.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const Key& GetKey() const noexcept;

		/**************************
		 * @return Const reference to the value.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] const Value& GetValue() const noexcept;

		/**************************
		 * @return Create time.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] MSAPI::Timer GetCreateTime() const noexcept;

		/**************************
		 * @return Modify time, equal to the create time if not modified.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] MSAPI::Timer GetModifyTime() const noexcept;

		/**************************
		 * @return True if the current value has already been persisted.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE [[nodiscard]] bool IsSaved() const noexcept;

	private:
		/**************************
		 * @brief Sets the saved status of the object.
		 *
		 * @param isSaved The new saved status.
		 *
		 * @test Yes.
		 */
		FORCE_INLINE void SetSaved(bool isSaved) noexcept;

		/**************************
		 * @brief Sets a new value and modify time, marks the object as unsaved.
		 *
		 * @tparam ValueType Type of the value, Value is assignable from it.
		 *
		 * @param value New value, forwarded to be moved if possible.
		 * @param modifyTime Modify time.
		 *
		 * @test Yes.
		 */
		template <typename ValueType>
			requires std::is_assignable_v<Value&, ValueType&&>
		FORCE_INLINE void Modify(ValueType&& value, MSAPI::Timer modifyTime) noexcept;

		// To manage the saved status and modifications
		friend class Pair;
	};

public:
	// Emplace policy: print the object before and after modification of an existing key
	static inline constexpr bool MODIFY_NOTIFY{ true };
	// Emplace policy: do not print modification of an existing key
	static inline constexpr bool MODIFY_SILENT{};

private:
	Map<Key, Object> m_map;
	// Keys of objects emplaced or modified after the last save, each key is stored once
	std::vector<Key> m_unsavedKeys;

public:
	/**************************
	 * @brief Construct a new Pair object, see Base constructor.
	 *
	 * @param dir Directory containing the data and timestamp files.
	 * @param name Base name of the data and timestamp files.
	 *
	 * @locking Is not required.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE Pair(std::string_view dir, std::string_view name) noexcept;

	Pair(const Pair&) = delete;
	Pair(Pair&&) = delete;
	Pair& operator=(const Pair&) = delete;
	Pair& operator=(Pair&&) = delete;

	/**************************
	 * @locking Requires a caller-held GetLock read guard throughout lookup, iteration, and copying. References and
	 * iterators must not escape into unsynchronized use.
	 *
	 * @return Const reference to the map of wrappers by keys.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] const Map<Key, Object>& Get() const noexcept;

	/**************************
	 * @brief Emplaces an unsaved object of the key and the value, or modifies the value of the key if it already
	 * exists. Emplacement sets the create time and the modify time to the current time, modification sets only modify
	 * time. The timestamp is set to the current time and the object is marked as unsaved. With SAVE_IMMEDIATE policy
	 * saves the object of the key to the data file, marks it as saved and saves the timestamp under the same lock.
	 * Objects of other keys emplaced or modified before with SAVE_DELAY policy stay unsaved until Save.
	 *
	 * @attention Allocation failure of the container terminates the process. With MODIFY_NOTIFY policy, the object
	 * before and after modification is printed in INFO level after the lock is released.
	 * @attention If saving of the object fails, the object stays in the map as unsaved and is saved by the next Save.
	 * If only the timestamp save fails, the object stays saved and the next Save saves the timestamp, see Save for
	 * failure details.
	 *
	 * @tparam SavePolicy SAVE_IMMEDIATE to save right after emplacement, SAVE_DELAY to defer saving.
	 * @tparam ModifyPolicy MODIFY_NOTIFY to print modification of an existing key, MODIFY_SILENT otherwise.
	 * MODIFY_NOTIFY requires Key and Value to be formattable.
	 * @tparam ValueType Type of the value, Value is constructible and assignable from it.
	 *
	 * @param key Key of the value.
	 * @param value Value, forwarded to be moved if possible.
	 *
	 * @locking Write lock m_lock.
	 *
	 * @return True if the object is emplaced or modified and, with SAVE_IMMEDIATE policy, saved with the timestamp.
	 * False if saving failed.
	 *
	 * @test Yes.
	 */
	template <bool SavePolicy, bool ModifyPolicy, typename ValueType>
		requires std::is_constructible_v<Value, ValueType&&> && std::is_assignable_v<Value&, ValueType&&>
		&& (!ModifyPolicy || (std::formattable<Key, char> && std::formattable<Value, char>))
	FORCE_INLINE [[nodiscard]] bool Emplace(const Key& key, ValueType&& value) noexcept;

private:
	/**************************
	 * @locking Requires caller-held read lock m_lock.
	 *
	 * @return True if the map is empty, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool IsEmptyImpl() const noexcept;

	/**************************
	 * @brief Reads all records of the data file and replays them in order into the map, the last record of a key wins.
	 * Marks objects as saved.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @return True if all records are read, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool ReadImpl() noexcept;

	/**************************
	 * @brief Saves objects of unsaved keys to the data file and marks them as saved. Objects of the keys saved by
	 * emplacement with SAVE_IMMEDIATE policy after the last save are skipped.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @return True if objects are saved, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool SaveImpl() noexcept;

	/**************************
	 * @brief Clears the map and unsaved keys.
	 *
	 * @locking Requires caller-held write lock m_lock.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void ClearImpl() noexcept;

	// Records handling for Read and Save
	friend BaseT;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
Base
---------------------------------------------------------------------------------*/

template <typename Derived>
FORCE_INLINE Base<Derived>::Base(const std::string_view dir, const std::string_view name) noexcept
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
		LOG_ERROR_NEW("Persistence path exceeds capacity: {}, directory: \"{}\", name: \"{}\"", CAPACITY, dir, name);
		return;
	}

	if (!m_path.Copy(dir) || (addSeparator && !m_path.Concatenate(std::string_view{ "/" }))) [[unlikely]] {
		LOG_ERROR_NEW("Failed to build persistence directory path, directory: \"{}\"", dir);
		m_path.Clear();
		return;
	}

	if (!m_path.NullTerminate() || (!MSAPI::IO::HasPath(m_path.Get()) && !MSAPI::IO::CreateDir(m_path.Get())))
		[[unlikely]] {

		m_path.Clear();
		return;
	}

	if (!m_path.Concatenate(std::string_view{ "." }) || !m_path.Concatenate(name) || !m_path.NullTerminate()
		|| !m_timestampPath.Copy(m_path.Get()) || !m_timestampPath.Concatenate(SUFFIX)
		|| !m_timestampPath.NullTerminate()) [[unlikely]] {

		LOG_ERROR_NEW("Failed to build persistence paths, path: \"{}\"", m_path.Get());
		m_path.Clear();
		m_timestampPath.Clear();
		return;
	}

	// Files are opened once to avoid opening them on each save, existing content is kept
	m_file = MSAPI::IO::FileGuard{ m_path.Get(), O_RDWR | O_CREAT | O_APPEND, 0644 };
	if (m_file.value == -1) [[unlikely]] {
		LOG_ERROR_NEW(
			"Cannot open persistence data file: \"{}\". Error №{}: {}", m_path.Get(), errno, std::strerror(errno));
		m_path.Clear();
		m_timestampPath.Clear();
		return;
	}

	m_timestampFile = MSAPI::IO::FileGuard{ m_timestampPath.Get(), O_RDWR | O_CREAT, 0644 };
	if (m_timestampFile.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Cannot open persistence timestamp file: \"{}\". Error №{}: {}", m_timestampPath.Get(), errno,
			std::strerror(errno));
		m_file.Clear();
		m_path.Clear();
		m_timestampPath.Clear();
		return;
	}
}

template <typename Derived> FORCE_INLINE [[nodiscard]] bool Base<Derived>::Read() noexcept
{
	// Invalid file descriptors after failed construction are rejected by IO
	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };
	if (!Self().IsEmptyImpl()) [[unlikely]] {
		LOG_WARNING_NEW("Interrupt the attempt to read records into a nonempty container, path: \"{}\"", m_path.Get());
		return false;
	}

	// Records read before a failure must not stay in memory
	if (!Self().ReadImpl()) [[unlikely]] {
		Self().ClearImpl();
		return false;
	}

	struct stat timestampStat { };
	if (fstat(m_timestampFile.value, &timestampStat) == -1) [[unlikely]] {
		LOG_ERROR_NEW("Cannot get status of persistence timestamp file: \"{}\". Error №{}: {}", m_timestampPath.Get(),
			errno, std::strerror(errno));
		Self().ClearImpl();
		return false;
	}

	// Timestamp file is created empty on construction, so an empty file has no timestamp yet
	if (timestampStat.st_size != 0) {
		MSAPI::Timer timestamp{ 0 };
		if (!MSAPI::IO::ReadBinary(&timestamp, m_timestampFile.value)) [[unlikely]] {
			Self().ClearImpl();
			return false;
		}

		m_timestamp = timestamp;
	}

	LOG_DEBUG_NEW("Records from path: {} are read successfully", m_path.Get());
	return true;
}

template <typename Derived> template <bool Policy> FORCE_INLINE [[nodiscard]] bool Base<Derived>::Save() noexcept
{
	// Invalid file descriptors after failed construction are rejected by IO
	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_lock };
	if (!Self().SaveImpl() || !SaveTimestamp()) [[unlikely]] {
		return false;
	}

	if constexpr (Policy == CLEAR) {
		Self().ClearImpl();
	}

	return true;
}

template <typename Derived> FORCE_INLINE [[nodiscard]] MSAPI::Timer Base<Derived>::GetTimestamp() const noexcept
{
	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ m_lock };
	return m_timestamp;
}

template <typename Derived> FORCE_INLINE [[nodiscard]] MSAPI::Lock::AtomicRW& Base<Derived>::GetLock() const noexcept
{
	return m_lock;
}

template <typename Derived>
template <typename Record>
FORCE_INLINE [[nodiscard]] bool Base<Derived>::SaveRecord(const Record& record) noexcept
{
	// Invalid file descriptor after failed construction is rejected by IO
	if (!IO::SaveBinary<IO::APPEND>(record, m_file.value)) [[unlikely]] {
		return false;
	}

	LOG_DEBUG_NEW("Record at: {} saved to: {}", m_timestamp.ToString(), m_path.Get());
	return true;
}

template <typename Derived> FORCE_INLINE [[nodiscard]] bool Base<Derived>::SaveTimestamp() noexcept
{
	return IO::SaveBinary(m_timestamp, m_timestampFile.value);
}

template <typename Derived> FORCE_INLINE [[nodiscard]] Derived& Base<Derived>::Self() noexcept
{
	return static_cast<Derived&>(*this);
}

/*---------------------------------------------------------------------------------
Single::Object
---------------------------------------------------------------------------------*/

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE Single<Container, Type>::Object::Object(const Type& object) noexcept
	: m_object{ object }
{
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE Single<Container, Type>::Object::Object(Type&& object) noexcept
	: m_object{ std::move(object) }
{
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] const Type& Single<Container, Type>::Object::Get() const noexcept
{
	return m_object;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::Object::IsSaved() const noexcept
{
	return m_isSaved;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::Object::operator==(const Object& other) const noexcept
{
	return m_object == other.m_object;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::Object::operator<(const Object& other) const noexcept
{
	return m_object < other.m_object;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE void Single<Container, Type>::Object::SetSaved(const bool isSaved) noexcept
{
	m_isSaved = isSaved;
}

/*---------------------------------------------------------------------------------
Single
---------------------------------------------------------------------------------*/

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE Single<Container, Type>::Single(const std::string_view dir, const std::string_view name) noexcept
	: BaseT{ dir, name }
{
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] const Container<typename Single<Container, Type>::Object>&
Single<Container, Type>::Get() const noexcept
{
	return m_container;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
template <bool SavePolicy, typename ObjectType>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::EmplaceBack(ObjectType&& object) noexcept
{
	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ BaseT::m_lock };
	auto& emplaced{ m_container.emplace_back(std::forward<ObjectType>(object)) };
	BaseT::m_timestamp = MSAPI::Timer{};

	if constexpr (SavePolicy == BaseT::SAVE_IMMEDIATE) {
		if (!BaseT::SaveRecord(emplaced)) [[unlikely]] {
			return false;
		}

		// Friend access
		emplaced.SetSaved(true);
		return BaseT::SaveTimestamp();
	}

	return true;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::IsEmptyImpl() const noexcept
{
	return m_container.empty();
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::ReadImpl() noexcept
{
	if (!MSAPI::IO::ReadBinaries(m_container, BaseT::m_file.value)) [[unlikely]] {
		return false;
	}

	for (auto& object : m_container) {
		// Friend access
		object.SetSaved(true);
	}

	return true;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE [[nodiscard]] bool Single<Container, Type>::SaveImpl() noexcept
{
	const auto end{ m_container.end() };
	auto begin{ std::find_if(m_container.begin(), end, [](const auto& object) { return !object.IsSaved(); }) };
	// Objects saved by emplacement with SAVE_IMMEDIATE policy can follow the first unsaved object
	auto unsaved{ std::ranges::subrange(begin, end)
		| std::views::filter([](const auto& object) { return !object.IsSaved(); }) };

	if (!IO::SaveBinaries<IO::APPEND>(unsaved.begin(), unsaved.end(), BaseT::m_file.value)) [[unlikely]] {
		return false;
	}

	LOG_DEBUG_NEW("Records at: {} saved to: {}, new entries: {}", BaseT::m_timestamp.ToString(), BaseT::m_path.Get(),
		std::ranges::distance(unsaved));

	while (begin != end) {
		// Friend access
		begin->SetSaved(true);
		++begin;
	}

	return true;
}

template <template <typename> typename Container, typename Type>
	requires std::is_trivially_copyable_v<Type>
FORCE_INLINE void Single<Container, Type>::ClearImpl() noexcept
{
	m_container.clear();
}

/*---------------------------------------------------------------------------------
Pair::Object
---------------------------------------------------------------------------------*/

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key>
	&& std::is_trivially_copyable_v<Value>
template <typename ValueType>
	requires std::is_constructible_v<Value, ValueType&&>
FORCE_INLINE Pair<Map, Key, Value>::Object::Object(
	const Key& key, ValueType&& value, const MSAPI::Timer createTime) noexcept
	: m_key{ key }
	, m_value{ std::forward<ValueType>(value) }
	, m_createTime{ createTime }
	, m_modifyTime{ createTime }
{
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] const Key& Pair<Map, Key, Value>::Object::GetKey() const noexcept
{
	return m_key;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] const Value& Pair<Map, Key, Value>::Object::GetValue() const noexcept
{
	return m_value;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] MSAPI::Timer Pair<Map, Key, Value>::Object::GetCreateTime() const noexcept
{
	return m_createTime;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] MSAPI::Timer Pair<Map, Key, Value>::Object::GetModifyTime() const noexcept
{
	return m_modifyTime;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] bool Pair<Map, Key, Value>::Object::IsSaved() const noexcept
{
	return m_isSaved;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE void Pair<Map, Key, Value>::Object::SetSaved(const bool isSaved) noexcept
{
	m_isSaved = isSaved;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key>
	&& std::is_trivially_copyable_v<Value>
template <typename ValueType>
	requires std::is_assignable_v<Value&, ValueType&&>
FORCE_INLINE void Pair<Map, Key, Value>::Object::Modify(ValueType&& value, const MSAPI::Timer modifyTime) noexcept
{
	m_value = std::forward<ValueType>(value);
	m_modifyTime = modifyTime;
	m_isSaved = false;
}

/*---------------------------------------------------------------------------------
Pair
---------------------------------------------------------------------------------*/

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE Pair<Map, Key, Value>::Pair(const std::string_view dir, const std::string_view name) noexcept
	: BaseT{ dir, name }
{
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] const Map<Key, typename Pair<Map, Key, Value>::Object>&
Pair<Map, Key, Value>::Get() const noexcept
{
	return m_map;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key>
	&& std::is_trivially_copyable_v<Value>
template <bool SavePolicy, bool ModifyPolicy, typename ValueType>
	requires std::is_constructible_v<Value, ValueType&&> && std::is_assignable_v<Value&, ValueType&&>
	&& (!ModifyPolicy || (std::formattable<Key, char> && std::formattable<Value, char>))
FORCE_INLINE [[nodiscard]] bool Pair<Map, Key, Value>::Emplace(const Key& key, ValueType&& value) noexcept
{
	// Copies of the modified object to print them after the lock is released
	std::optional<std::pair<Object, Object>> modification;
	bool result{ true };
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ BaseT::m_lock };
		const MSAPI::Timer now{};
		BaseT::m_timestamp = now;
		// Arguments are not moved from if the key already exists, so the value is still valid for modification
		const auto [it, isInserted] = m_map.try_emplace(key, key, std::forward<ValueType>(value), now);
		// Key is listed if its object was modified after the last save
		const bool isListed{ !isInserted && !it->second.IsSaved() };
		if (!isInserted) {
			if constexpr (ModifyPolicy == MODIFY_NOTIFY) {
				modification.emplace(it->second, it->second);
			}

			// Friend access
			it->second.Modify(std::forward<ValueType>(value), now);
			if constexpr (ModifyPolicy == MODIFY_NOTIFY) {
				modification->second = it->second;
			}
		}

		if constexpr (SavePolicy == BaseT::SAVE_IMMEDIATE) {
			result = BaseT::SaveRecord(it->second);
			if (result) [[likely]] {
				// Friend access
				it->second.SetSaved(true);
				result = BaseT::SaveTimestamp();
			}
		}

		// Unsaved object is saved by the next Save
		if (!isListed && !it->second.IsSaved()) {
			m_unsavedKeys.emplace_back(key);
		}
	}

	if constexpr (ModifyPolicy == MODIFY_NOTIFY) {
		if (modification.has_value()) {
			const auto& [before, after] = *modification;
			LOG_INFO_NEW("Persistence object is modified from: {}, to: {}. Path: \"{}\", key: {}, create time: {}, "
						 "previous modify time: {}, current modify time: {}",
				before.GetValue(), after.GetValue(), BaseT::m_path.Get(), before.GetKey(),
				before.GetCreateTime().ToString(), before.GetModifyTime().ToString(), after.GetModifyTime().ToString());
		}
	}

	return result;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] bool Pair<Map, Key, Value>::IsEmptyImpl() const noexcept
{
	return m_map.empty();
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] bool Pair<Map, Key, Value>::ReadImpl() noexcept
{
	std::vector<Object> records;
	if (!MSAPI::IO::ReadBinaries(records, BaseT::m_file.value)) [[unlikely]] {
		return false;
	}

	for (auto& record : records) {
		// Friend access
		record.SetSaved(true);
		(void)m_map.insert_or_assign(record.GetKey(), record);
	}

	return true;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE [[nodiscard]] bool Pair<Map, Key, Value>::SaveImpl() noexcept
{
	// Object of the key can be saved by emplacement with SAVE_IMMEDIATE policy after the key is listed
	const auto toObject{ [this](const Key& key) -> const Object& { return m_map.find(key)->second; } };
	const auto isUnsaved{ [](const Object& object) { return !object.IsSaved(); } };
	auto unsaved{ m_unsavedKeys | std::views::transform(toObject) | std::views::filter(isUnsaved) };

	if (!IO::SaveBinaries<IO::APPEND>(unsaved.begin(), unsaved.end(), BaseT::m_file.value)) [[unlikely]] {
		return false;
	}

	LOG_DEBUG_NEW("Records at: {} saved to: {}, new entries: {}", BaseT::m_timestamp.ToString(), BaseT::m_path.Get(),
		std::ranges::distance(unsaved));

	for (const auto& key : m_unsavedKeys) {
		// Friend access
		m_map.find(key)->second.SetSaved(true);
	}

	m_unsavedKeys.clear();
	return true;
}

template <template <typename...> typename Map, typename Key, typename Value>
	requires std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>
FORCE_INLINE void Pair<Map, Key, Value>::ClearImpl() noexcept
{
	m_map.clear();
	m_unsavedKeys.clear();
}

} // namespace Persistence

} // namespace MSAPI

#endif // MSAPI_PERSISTENCE_INL