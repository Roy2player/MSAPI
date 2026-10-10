/**************************
 * @file        persistence.inl
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

#ifndef MSAPI_UNIT_TEST_PERSISTENCE_INL
#define MSAPI_UNIT_TEST_PERSISTENCE_INL

#include "../../../../library/source/help/persistence.inl"
#include "../../../../library/source/test/test.inl"
#include <array>
#include <barrier>
#include <list>
#include <map>
#include <optional>
#include <thread>
#include <unordered_map>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Persistence.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool Persistence();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool Persistence()
{
	LOG_INFO("MSAPI UNIT TEST Persistence");

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		LOG_ERROR("Cannot get executable path");
		return false;
	}

	path += "testData/";

	const std::string_view pathV{ path };
	struct Cleaner {
		const std::string_view path;

		FORCE_INLINE Cleaner(const std::string_view path) noexcept
			: path{ path }
		{
		}

		FORCE_INLINE ~Cleaner() noexcept
		{
			if (!path.empty() && IO::HasPath(path)) {
				if (!IO::Remove(path)) {
					LOG_ERROR_NEW("Cannot remove test dir: {}, clean it before next test execution", path);
				}
			}
		}
	} cleaner{ pathV };

	MSAPI::Test::Test t;

	RETURN_IF_FALSE(t.Assert(IO::HasPath(pathV), false, "Test directory absent"));
	RETURN_IF_FALSE(t.Assert(IO::CreateDir(pathV), true, "Create test directory"));

	class TestObject {
	private:
		int32_t m_number{};
		double m_value{};
		bool m_enabled{};

	public:
		FORCE_INLINE explicit TestObject(const int32_t number) noexcept
			: m_number{ number }
			, m_value{ static_cast<double>(number) / 7. }
			, m_enabled{ number % 2 == 0 }
		{
		}

		TestObject(const TestObject&) = default;
		TestObject(TestObject&&) = default;
		TestObject& operator=(const TestObject&) = default;
		TestObject& operator=(TestObject&&) = default;

		FORCE_INLINE [[nodiscard]] double GetValue() const noexcept { return m_value; }

		FORCE_INLINE [[nodiscard]] bool operator==(const TestObject& other) const noexcept
		{
			return m_number == other.m_number && Helper::FloatEqual(m_value, other.m_value)
				&& m_enabled == other.m_enabled;
		}

		FORCE_INLINE [[nodiscard]] bool operator<(const TestObject& other) const noexcept
		{
			return m_number < other.m_number;
		}
	};

	const auto testPersistence{ [&t, &path]<template <typename> typename Container, typename Type>(
									const std::string_view suiteName, const auto& makeObject) -> bool {
		LOG_INFO_NEW("Persistence suite: {}", suiteName);
		using PersistenceType = MSAPI::Persistence::Single<Container, Type>;
		constexpr bool REMAIN{ PersistenceType::REMAIN };
		constexpr bool CLEAR{ PersistenceType::CLEAR };
		constexpr bool SAVE_IMMEDIATE{ PersistenceType::SAVE_IMMEDIATE };
		constexpr bool SAVE_DELAY{ PersistenceType::SAVE_DELAY };
		using Object = typename PersistenceType::Object;
		static_assert(std::is_trivially_copyable_v<Type>);
		static_assert(std::is_same_v<decltype(std::declval<const PersistenceType&>().Get()), const Container<Object>&>);
		const auto dir{ path + std::string{ suiteName } + "/" };
		const std::string_view dirV{ dir };
		const MSAPI::Timer timestamp{ 123456, 789 };
		const std::vector<Type> firstBatch{ makeObject(3), makeObject(1) };
		const std::vector<Type> secondBatch{ makeObject(4), makeObject(2) };

		const auto checkState{ [&t](PersistenceType& persistence, const std::vector<Type>& expected,
								   const int64_t savedCount) -> bool {
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
			std::vector<Type> actual;
			int64_t saved{};
			for (const auto& object : persistence.Get()) {
				actual.emplace_back(object.Get());
				saved += object.IsSaved();
			}
			RETURN_IF_FALSE(t.Assert(actual == expected, true, "Wrapper payloads"));
			RETURN_IF_FALSE(t.Assert(saved, savedCount, "Saved object count"));
			if constexpr (std::is_same_v<Type, TestObject>) {
				for (int64_t index{}; index < INT64(actual.size()); ++index) {
					RETURN_IF_FALSE(
						t.Assert(actual[UINT64(index)].GetValue(), expected[UINT64(index)].GetValue(), "FP payload"));
				}
			}
			return true;
		} };

		// Payloads are persisted together with their wrapper.
		const auto wrap{ [](const std::vector<Type>& payloads) {
			std::vector<Object> objects;
			for (const auto& payload : payloads) {
				objects.emplace_back(payload);
			}
			return objects;
		} };

		const auto readData{ [&t](const std::string& dataPath, std::vector<Type>& payloads) -> bool {
			std::vector<Object> objects;
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(objects, dataPath.c_str()), true, "Read wrapper records"));
			for (const auto& object : objects) {
				payloads.emplace_back(object.Get());
			}
			return true;
		} };

		const auto checkData{ [&t, &readData](const std::string& dataPath, const std::vector<Type>& expected) -> bool {
			RETURN_IF_FALSE(t.Assert(IO::HasPath(dataPath.c_str()), true, "Data file exists"));
			std::vector<Type> actual;
			RETURN_IF_FALSE(readData(dataPath, actual));
			RETURN_IF_FALSE(t.Assert(actual == expected, true, "Disk payloads and order"));
			std::string bytes;
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(bytes, dataPath.c_str()), true, "Read binary bytes"));
			RETURN_IF_FALSE(t.Assert(bytes.size(), expected.size() * sizeof(Object), "Wrapper record byte count"));
			return true;
		} };

		const auto checkTimestamp{ [&t](const std::string& dataPath, const MSAPI::Timer expected) -> bool {
			const auto timestampPath{ dataPath + "_timestamp" };
			RETURN_IF_FALSE(t.Assert(IO::HasPath(timestampPath.c_str()), true, "Timestamp file exists"));
			MSAPI::Timer actual{ 0 };
			RETURN_IF_FALSE(t.Assert(IO::ReadBinary(&actual, timestampPath.c_str()), true, "Read timestamp"));
			RETURN_IF_FALSE(t.Assert(actual, expected, "Disk timestamp"));
			return true;
		} };

		RETURN_IF_FALSE(t.Assert(IO::CreateDir(dir.c_str()), true, "Create suite directory"));

		{
			// Create missing directories and normalize the optional trailing separator.
			const auto missingDir{ dir + "missing/nested" };
			PersistenceType persistence{ std::string_view{ missingDir }, "data" };
			RETURN_IF_FALSE(t.Assert(IO::HasPath(missingDir.c_str()), true, "Constructor creates nested directory"));
			RETURN_IF_FALSE(
				t.Assert(IO::HasPath((missingDir + "/.data").c_str()), true, "Constructor creates data file"));
			RETURN_IF_FALSE(t.Assert(
				IO::HasPath((missingDir + "/.data_timestamp").c_str()), true, "Constructor creates timestamp file"));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "New directory reads empty data"));
			RETURN_IF_FALSE(checkState(persistence, {}, 0));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save in created directory"));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "Read after first save"));
			RETURN_IF_FALSE(checkData(missingDir + "/.data", {}));
			RETURN_IF_FALSE(checkTimestamp(missingDir + "/.data", MSAPI::Timer{ 0 }));
			PersistenceType trailing{ std::string_view{ missingDir + "/" }, "data" };
			RETURN_IF_FALSE(t.Assert(trailing.Read(), true, "Trailing separator resolves same files"));
			PersistenceType emptyDirectory{ "", "data" };
			RETURN_IF_FALSE(t.Assert(emptyDirectory.Read(), false, "Empty directory read"));
			RETURN_IF_FALSE(t.Assert(emptyDirectory.template Save<REMAIN>(), false, "Empty directory save"));
			PersistenceType emptyName{ dirV, "" };
			RETURN_IF_FALSE(t.Assert(emptyName.Read(), false, "Empty name read"));
			RETURN_IF_FALSE(t.Assert(emptyName.template Save<REMAIN>(), false, "Empty name save"));
			PersistenceType separatorName{ dirV, "nested/data" };
			RETURN_IF_FALSE(t.Assert(separatorName.template Save<REMAIN>(), false, "Name with separator save"));
			RETURN_IF_FALSE(
				t.Assert(IO::HasPath((dir + ".nested").c_str()), false, "Name with separator creates no file"));

			// A timestamp path occupying the entire fixed buffer must be rejected to reserve its null terminator.
			MSAPI::SString<512> fullName;
			const auto nameSize{ fullName.GetCapacity() - dirV.size()
				- 1 /* dot */ - std::string_view{ "_timestamp" }.size() };
			std::fill_n(fullName.GetBuffer(), nameSize, 'x');
			fullName.GetBuffer()[nameSize] = '\0';
			RETURN_IF_FALSE(t.Assert(fullName.UpdateSize(), nameSize, "Prepare fixed-path capacity boundary"));
			PersistenceType fullPath{ dirV, fullName.Get() };
			RETURN_IF_FALSE(t.Assert(fullPath.Read(), false, "Reject path without terminator space on read"));
			RETURN_IF_FALSE(
				t.Assert(fullPath.template Save<REMAIN>(), false, "Reject path without terminator space on save"));

			const auto parent{ dir + "parentFile" };
			RETURN_IF_FALSE(t.Assert(IO::SaveStr("block", parent.c_str()), true, "Create regular-file parent"));
			PersistenceType blocked{ std::string_view{ parent + "/" }, "data" };
			RETURN_IF_FALSE(t.Assert(blocked.Read(), false, "Regular-file parent read"));
			RETURN_IF_FALSE(t.Assert(blocked.template Save<REMAIN>(), false, "Regular-file parent save"));
			const auto directoryFile{ dir + ".directoryFile" };
			RETURN_IF_FALSE(t.Assert(IO::CreateDir(directoryFile.c_str()), true, "Create data-path blocker"));
			PersistenceType directoryPersistence{ dirV, "directoryFile" };
			RETURN_IF_FALSE(t.Assert(directoryPersistence.Read(), false, "Directory data read"));
			RETURN_IF_FALSE(t.Assert(directoryPersistence.template Save<REMAIN>(), false, "Directory data save"));

			// Failed persisting keeps the emplaced object unsaved
			RETURN_IF_FALSE(t.Assert(directoryPersistence.template EmplaceBack<SAVE_IMMEDIATE>(firstBatch[0]), false,
				"Persist emplacement without opened files"));
			RETURN_IF_FALSE(checkState(directoryPersistence, { firstBatch[0] }, 0));
			const auto directoryTimestamp{ dir + ".directoryTimestamp_timestamp" };
			RETURN_IF_FALSE(t.Assert(IO::CreateDir(directoryTimestamp.c_str()), true, "Create timestamp-path blocker"));
			PersistenceType timestampBlocked{ dirV, "directoryTimestamp" };
			RETURN_IF_FALSE(t.Assert(timestampBlocked.Read(), false, "Directory timestamp read"));
			RETURN_IF_FALSE(t.Assert(timestampBlocked.template Save<REMAIN>(), false, "Directory timestamp save"));
		}

		{
			// Nothing to read
			PersistenceType persistence{ dirV, "empty" };
			RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), MSAPI::Timer{ 0 }, "Initial timestamp"));
			RETURN_IF_FALSE(checkData(dir + ".empty", {}));
			std::string timestampBytes;
			RETURN_IF_FALSE(t.Assert(
				IO::ReadStr(timestampBytes, (dir + ".empty_timestamp").c_str()), true, "Read created timestamp file"));
			RETURN_IF_FALSE(t.Assert(timestampBytes.empty(), true, "Created timestamp file is empty"));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "Read created empty files"));
			RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), MSAPI::Timer{ 0 }, "Empty timestamp file is ignored"));

			// Nothing to save, timestamp stays initial without emplaced objects
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save empty persistence"));
			RETURN_IF_FALSE(checkData(dir + ".empty", {}));
			RETURN_IF_FALSE(checkTimestamp(dir + ".empty", MSAPI::Timer{ 0 }));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "Read after empty save"));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "Empty persistence permits repeated read"));

			// Missing timestamp file keeps the current timestamp.
			RETURN_IF_FALSE(
				t.Assert(IO::Remove((dir + ".empty_timestamp").c_str()), true, "Remove optional timestamp"));
			PersistenceType optional{ dirV, "empty" };
			RETURN_IF_FALSE(t.Assert(optional.Read(), true, "Read without timestamp"));
			RETURN_IF_FALSE(
				t.Assert(optional.GetTimestamp(), MSAPI::Timer{ 0 }, "Keep timestamp without timestamp file"));
			RETURN_IF_FALSE(checkState(optional, {}, 0));

			// Files are bound on construction, removed data file is still saved and read through the opened descriptor
			PersistenceType removed{ dirV, "removed" };
			RETURN_IF_FALSE(t.Assert(IO::Remove((dir + ".removed").c_str()), true, "Remove created data file"));
			RETURN_IF_FALSE(t.Assert(
				removed.template EmplaceBack<SAVE_IMMEDIATE>(firstBatch[0]), true, "Save into removed data file"));
			RETURN_IF_FALSE(t.Assert(IO::HasPath((dir + ".removed").c_str()), false, "No new data file at the path"));
			RETURN_IF_FALSE(t.Assert(removed.template Save<CLEAR>(), true, "Clear objects saved into removed file"));
			RETURN_IF_FALSE(t.Assert(removed.Read(), true, "Read removed data file"));
			RETURN_IF_FALSE(checkState(removed, { firstBatch[0] }, 1));
		}

		{
			// Save objects
			PersistenceType persistence{ dirV, "roundTrip" };
			const MSAPI::Timer beforeEmplace{};
			RETURN_IF_FALSE(
				t.Assert(persistence.template EmplaceBack<SAVE_DELAY>(firstBatch[0]), true, "Delayed emplacement"));
			const auto firstTimestamp{ persistence.GetTimestamp() };
			RETURN_IF_FALSE(t.Assert(firstTimestamp >= beforeEmplace, true, "EmplaceBack updates timestamp"));
			RETURN_IF_FALSE(t.Assert(
				persistence.template EmplaceBack<SAVE_DELAY>(makeObject(1)), true, "Next delayed emplacement"));
			const auto emplacedTimestamp{ persistence.GetTimestamp() };
			RETURN_IF_FALSE(
				t.Assert(emplacedTimestamp >= firstTimestamp, true, "Next EmplaceBack moves timestamp forward"));
			RETURN_IF_FALSE(checkState(persistence, firstBatch, 0));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save first batch"));
			RETURN_IF_FALSE(checkState(persistence, firstBatch, 2));
			RETURN_IF_FALSE(checkData(dir + ".roundTrip", firstBatch));
			RETURN_IF_FALSE(checkTimestamp(dir + ".roundTrip", emplacedTimestamp));

			// Read objects
			PersistenceType loaded{ dirV, "roundTrip" };
			RETURN_IF_FALSE(t.Assert(loaded.Read(), true, "Read first batch"));
			RETURN_IF_FALSE(checkState(loaded, firstBatch, 2));
			RETURN_IF_FALSE(t.Assert(loaded.Read(), false, "Read persistence rejects second read"));
			RETURN_IF_FALSE(checkState(loaded, firstBatch, 2));

			// Repeated save must leave the data file and the timestamp unchanged.
			std::string before;
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(before, (dir + ".roundTrip").c_str()), true, "Snapshot first save"));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Repeat save without insertions"));
			std::string after;
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(after, (dir + ".roundTrip").c_str()), true, "Snapshot repeat save"));
			RETURN_IF_FALSE(t.Assert(after, before, "No repeated payload writes"));
			RETURN_IF_FALSE(checkTimestamp(dir + ".roundTrip", emplacedTimestamp));

			// Save again
			for (const auto& object : secondBatch) {
				RETURN_IF_FALSE(t.Assert(
					persistence.template EmplaceBack<SAVE_DELAY>(object), true, "Delayed emplacement of second batch"));
			}
			auto allObjects{ firstBatch };
			allObjects.insert(allObjects.end(), secondBatch.begin(), secondBatch.end());
			RETURN_IF_FALSE(checkState(persistence, allObjects, 2));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save incremental batch"));
			RETURN_IF_FALSE(checkState(persistence, allObjects, 4));
			auto diskObjects{ firstBatch };
			diskObjects.insert(diskObjects.end(), secondBatch.begin(), secondBatch.end());
			RETURN_IF_FALSE(checkData(dir + ".roundTrip", diskObjects));

			// Read again
			PersistenceType reloaded{ dirV, "roundTrip" };
			RETURN_IF_FALSE(t.Assert(reloaded.Read(), true, "Read incremental batches"));
			RETURN_IF_FALSE(checkState(reloaded, diskObjects, 4));
			RETURN_IF_FALSE(t.Assert(reloaded.GetTimestamp(), persistence.GetTimestamp(), "Reloaded timestamp"));
		}

		{
			// Saving appends to existing files, CLEAR policy releases memory without removing persisted records.
			const auto dataPath{ dir + ".clear" };
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinaries(wrap(firstBatch), dataPath.c_str()), true, "Preseed append file"));
			PersistenceType persistence{ dirV, "clear" };
			RETURN_IF_FALSE(t.Assert(
				persistence.template EmplaceBack<SAVE_DELAY>(secondBatch[0]), true, "Delayed emplacement before read"));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), false, "Reject nonempty first read"));
			RETURN_IF_FALSE(
				t.Assert(persistence.template Save<REMAIN>(), true, "Fresh persistence appends existing file"));
			auto appended{ firstBatch };
			appended.emplace_back(secondBatch[0]);
			RETURN_IF_FALSE(checkData(dataPath, appended));
			RETURN_IF_FALSE(checkState(persistence, { secondBatch[0] }, 1));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<CLEAR>(), true, "Save with clearing"));
			RETURN_IF_FALSE(checkState(persistence, {}, 0));
			RETURN_IF_FALSE(checkData(dataPath, appended));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "Read after save with clearing"));
			RETURN_IF_FALSE(checkState(persistence, appended, 3));

			// Files are bound on construction, saving continues into the renamed data file.
			RETURN_IF_FALSE(t.Assert(persistence.template EmplaceBack<SAVE_DELAY>(secondBatch[1]), true,
				"Delayed emplacement before rename"));
			auto expected{ appended };
			expected.emplace_back(secondBatch[1]);
			const auto renamedPath{ dataPath + ".renamed" };
			RETURN_IF_FALSE(t.Assert(IO::Rename(dataPath.c_str(), renamedPath.c_str()), true, "Rename data file"));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<CLEAR>(), true, "Save into renamed data file"));
			RETURN_IF_FALSE(checkState(persistence, {}, 0));
			RETURN_IF_FALSE(t.Assert(IO::HasPath(dataPath.c_str()), false, "No new data file at the path"));
			RETURN_IF_FALSE(checkData(renamedPath, expected));
			RETURN_IF_FALSE(t.Assert(IO::Rename(renamedPath.c_str(), dataPath.c_str()), true, "Restore data file"));
			RETURN_IF_FALSE(checkData(dataPath, expected));
			RETURN_IF_FALSE(t.Assert(persistence.Read(), true, "Read restores all persisted records"));
			RETURN_IF_FALSE(checkState(persistence, expected, 4));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<CLEAR>(), true, "Clear read objects"));
			RETURN_IF_FALSE(checkData(dataPath, expected));
		}

		{
			// Malformed files are rejected by Read and can be repaired.
			const auto dataPath{ dir + ".retry" };
			const auto timestampPath{ dataPath + "_timestamp" };
			PersistenceType persistence{ dirV, "retry" };
			RETURN_IF_FALSE(
				t.Assert(persistence.template EmplaceBack<SAVE_DELAY>(firstBatch[0]), true, "Delayed emplacement"));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save record and timestamp"));
			RETURN_IF_FALSE(checkData(dataPath, { firstBatch[0] }));
			RETURN_IF_FALSE(checkTimestamp(dataPath, persistence.GetTimestamp()));

			// Malformed timestamp reads must preserve memory and permit a repaired retry.
			RETURN_IF_FALSE(t.Assert(IO::SaveStr("x", timestampPath.c_str()), true, "Truncate timestamp fixture"));
			PersistenceType loaded{ dirV, "retry" };
			RETURN_IF_FALSE(t.Assert(loaded.Read(), false, "Reject malformed timestamp"));
			RETURN_IF_FALSE(checkState(loaded, {}, 0));
			RETURN_IF_FALSE(t.Assert(loaded.GetTimestamp(), MSAPI::Timer{ 0 }, "Failed read keeps timestamp"));
			RETURN_IF_FALSE(t.Assert(IO::SaveBinary(timestamp, timestampPath.c_str()), true, "Repair timestamp file"));
			RETURN_IF_FALSE(t.Assert(loaded.Read(), true, "Retry malformed timestamp read"));
			RETURN_IF_FALSE(checkState(loaded, { firstBatch[0] }, 1));

			// A partial trailing record must not commit earlier staged payloads to memory.
			const int8_t partial{ 1 };
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinary<IO::APPEND>(partial, dataPath.c_str()), true, "Append partial record"));
			RETURN_IF_FALSE(t.Assert(loaded.template Save<CLEAR>(), true, "Clear read objects"));
			RETURN_IF_FALSE(t.Assert(loaded.Read(), false, "Reject malformed data after valid record"));
			RETURN_IF_FALSE(checkState(loaded, {}, 0));
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinaries(wrap(firstBatch), dataPath.c_str()), true, "Repair malformed data"));
			RETURN_IF_FALSE(t.Assert(loaded.Read(), true, "Retry malformed data read"));
			RETURN_IF_FALSE(checkState(loaded, firstBatch, 2));
		}

		{
			// SAVE_IMMEDIATE policy saves only the emplaced object right after emplacement, SAVE_DELAY waits for Save.
			const auto dataPath{ dir + ".persist" };
			PersistenceType persistence{ dirV, "persist" };
			RETURN_IF_FALSE(
				t.Assert(persistence.template EmplaceBack<SAVE_DELAY>(firstBatch[0]), true, "Delayed emplacement"));
			RETURN_IF_FALSE(checkData(dataPath, {}));
			RETURN_IF_FALSE(checkState(persistence, { firstBatch[0] }, 0));
			RETURN_IF_FALSE(t.Assert(
				persistence.template EmplaceBack<SAVE_IMMEDIATE>(firstBatch[1]), true, "Persisted emplacement"));
			RETURN_IF_FALSE(checkState(persistence, firstBatch, 1));
			RETURN_IF_FALSE(checkData(dataPath, { firstBatch[1] }));
			RETURN_IF_FALSE(checkTimestamp(dataPath, persistence.GetTimestamp()));

			// Save saves the delayed object only, the immediately saved one is not duplicated
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save after persisted emplacement"));
			RETURN_IF_FALSE(checkState(persistence, firstBatch, 2));
			std::vector<Type> expected{ firstBatch[1], firstBatch[0] };
			RETURN_IF_FALSE(checkData(dataPath, expected));
			RETURN_IF_FALSE(t.Assert(
				persistence.template EmplaceBack<SAVE_IMMEDIATE>(secondBatch[0]), true, "Next persisted emplacement"));
			expected.emplace_back(secondBatch[0]);
			RETURN_IF_FALSE(checkData(dataPath, expected));
			RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save without unsaved objects"));
			RETURN_IF_FALSE(checkData(dataPath, expected));
			PersistenceType loaded{ dirV, "persist" };
			RETURN_IF_FALSE(t.Assert(loaded.Read(), true, "Read persisted emplacements"));
			RETURN_IF_FALSE(checkState(loaded, expected, 3));
			RETURN_IF_FALSE(t.Assert(loaded.GetTimestamp(), persistence.GetTimestamp(), "Read persisted timestamp"));
		}

		{
			// Interleave insertions, guarded lookups, and incremental saves on one persistence.
			PersistenceType persistence{ dirV, "concurrent" };
			std::vector<Type> expected;
			for (int32_t worker{}; worker < 2; ++worker) {
				for (int32_t index{}; index < 48; ++index) {
					expected.emplace_back(makeObject(100 + worker * 64 + index));
				}
			}
			std::barrier start{ 4 };
			std::array<bool, 4> succeeded{ true, true, true, true };
			std::array<std::jthread, 4> workers;
			for (int32_t worker{}; worker < 4; ++worker) {
				workers[UINT64(worker)] = std::jthread{ [&, worker] {
					start.arrive_and_wait();
					for (int32_t index{}; index < 48; ++index) {
						if (worker == 0) {
							const bool emplaced{ t.Assert(
								persistence.template EmplaceBack<SAVE_DELAY>(makeObject(100 + worker * 64 + index)),
								true, "Concurrent delayed emplacement") };
							succeeded[UINT64(worker)] = emplaced && succeeded[UINT64(worker)];
						}
						else if (worker == 1) {
							const bool emplaced{ t.Assert(
								persistence.template EmplaceBack<SAVE_IMMEDIATE>(makeObject(100 + worker * 64 + index)),
								true, "Concurrent persisted emplacement") };
							succeeded[UINT64(worker)] = emplaced && succeeded[UINT64(worker)];
						}
						else if (worker == 2) {
							const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
							const auto& objects{ persistence.Get() };
							const bool valid{ t.Assert(std::all_of(objects.begin(), objects.end(),
														   [&expected](const auto& object) {
															   return std::find(expected.begin(), expected.end(),
																		  object.Get())
																   != expected.end();
														   }),
								true, "Concurrent guarded lookup") };
							succeeded[UINT64(worker)] = valid && succeeded[UINT64(worker)];
						}
						else {
							const bool saved{ t.Assert(
								persistence.template Save<REMAIN>(), true, "Concurrent incremental save") };
							succeeded[UINT64(worker)] = saved && succeeded[UINT64(worker)];
						}
					}
				} };
			}

			for (auto& worker : workers) {
				worker.join();
			}

			for (const bool success : succeeded) {
				RETURN_IF_FALSE(t.Assert(success, true, "Concurrent insert lookup and save"));
			}

			// Competing saves must serialize and persist every payload exactly once.
			std::barrier saveStart{ 2 };
			std::array<bool, 2> saves{};
			std::jthread firstSave{ [&] {
				saveStart.arrive_and_wait();
				saves[0] = t.Assert(persistence.template Save<REMAIN>(), true, "First competing save");
			} };
			std::jthread secondSave{ [&] {
				saveStart.arrive_and_wait();
				saves[1] = t.Assert(persistence.template Save<REMAIN>(), true, "Second competing save");
			} };
			firstSave.join();
			secondSave.join();

			RETURN_IF_FALSE(t.Assert(saves[0] && saves[1], true, "Competing saves complete"));
			std::vector<Type> diskObjects;
			RETURN_IF_FALSE(readData(dir + ".concurrent", diskObjects));
			auto sortedDisk{ diskObjects };
			std::sort(sortedDisk.begin(), sortedDisk.end());
			std::sort(expected.begin(), expected.end());
			RETURN_IF_FALSE(t.Assert(sortedDisk == expected, true, "Concurrent payloads persisted exactly once"));
			RETURN_IF_FALSE(checkData(dir + ".concurrent", diskObjects));

			std::vector<Type> memoryObjects;
			{
				const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
				for (const auto& object : persistence.Get()) {
					memoryObjects.emplace_back(object.Get());
				}
			}
			RETURN_IF_FALSE(checkState(persistence, memoryObjects, 96));

			// Only one of two competing reads may consume the successful-read allowance.
			PersistenceType loaded{ dirV, "concurrent" };
			std::barrier loadStart{ 2 };
			std::array<bool, 2> loads{};
			std::jthread firstLoad{ [&] {
				loadStart.arrive_and_wait();
				loads[0] = loaded.Read();
			} };
			std::jthread secondLoad{ [&] {
				loadStart.arrive_and_wait();
				loads[1] = loaded.Read();
			} };
			firstLoad.join();
			secondLoad.join();
			RETURN_IF_FALSE(t.Assert(loads[0] != loads[1], true, "Exactly one competing read succeeds"));
			RETURN_IF_FALSE(checkState(loaded, diskObjects, 96));

			// Guarded readers remain valid while another thread saves with clearing, reads and appends the persistence.
			auto allowed{ expected };
			allowed.insert(allowed.end(), secondBatch.begin(), secondBatch.end());
			std::barrier clearStart{ 2 };
			bool lookupsValid{ true };
			std::jthread lookup{ [&] {
				clearStart.arrive_and_wait();
				for (int32_t iteration{}; iteration < 48; ++iteration) {
					const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
					const auto& objects{ persistence.Get() };
					const bool valid{ t.Assert(std::all_of(objects.begin(), objects.end(),
												   [&allowed](const auto& object) {
													   return std::find(allowed.begin(), allowed.end(), object.Get())
														   != allowed.end();
												   }),
						true, "Concurrent lookup during clearing save") };
					lookupsValid = valid && lookupsValid;
				}
			} };
			clearStart.arrive_and_wait();

			const bool cleared{ persistence.template Save<CLEAR>() };
			const bool reloaded{ persistence.Read() };
			bool emplaced{ true };
			for (const auto& object : secondBatch) {
				emplaced = persistence.template EmplaceBack<SAVE_DELAY>(object) && emplaced;
			}

			const bool saved{ persistence.template Save<REMAIN>() };
			lookup.join();
			RETURN_IF_FALSE(t.Assert(
				lookupsValid && cleared && reloaded && emplaced && saved, true, "Guarded lookup during clearing save"));
			auto appendedDisk{ diskObjects };
			appendedDisk.insert(appendedDisk.end(), secondBatch.begin(), secondBatch.end());
			RETURN_IF_FALSE(checkState(persistence, appendedDisk, INT64(appendedDisk.size())));
			RETURN_IF_FALSE(checkData(dir + ".concurrent", appendedDisk));
		}
		return true;
	} };

	const auto testPair{ [&t, &path]<template <typename...> typename Map>(const std::string_view suiteName) -> bool {
		LOG_INFO_NEW("Persistence pair suite: {}", suiteName);
		using PersistenceType = MSAPI::Persistence::Pair<Map, int32_t, int64_t>;
		using Object = typename PersistenceType::Object;
		constexpr bool REMAIN{ PersistenceType::REMAIN };
		constexpr bool CLEAR{ PersistenceType::CLEAR };
		constexpr bool MODIFY_NOTIFY{ PersistenceType::MODIFY_NOTIFY };
		constexpr bool MODIFY_SILENT{ PersistenceType::MODIFY_SILENT };
		constexpr bool SAVE_IMMEDIATE{ PersistenceType::SAVE_IMMEDIATE };
		constexpr bool SAVE_DELAY{ PersistenceType::SAVE_DELAY };
		static_assert(std::is_trivially_copyable_v<Object>);
		static_assert(
			std::is_same_v<decltype(std::declval<const PersistenceType&>().Get()), const Map<int32_t, Object>&>);

		// Values without formatter can be emplaced only silently
		using SilentType = MSAPI::Persistence::Pair<Map, int32_t, TestObject>;
		static_assert(requires(SilentType& persistence) {
			persistence.template Emplace<SilentType::SAVE_DELAY, SilentType::MODIFY_SILENT>(1, TestObject{ 1 });
		});
		static_assert(!requires(SilentType& persistence) {
			persistence.template Emplace<SilentType::SAVE_DELAY, SilentType::MODIFY_NOTIFY>(1, TestObject{ 1 });
		});

		const auto dir{ path + std::string{ suiteName } + "/" };
		const std::string_view dirV{ dir };
		RETURN_IF_FALSE(t.Assert(IO::CreateDir(dir.c_str()), true, "Create pair suite directory"));
		const auto dataPath{ dir + ".pair" };

		// Copy of the object of the key, empty if the key does not exist
		const auto find{ [](const PersistenceType& persistence, const int32_t key) -> std::optional<Object> {
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
			const auto it{ persistence.Get().find(key) };
			if (it == persistence.Get().end()) {
				return {};
			}

			return it->second;
		} };

		const auto checkObject{ [&t, &find](const PersistenceType& persistence, const int32_t key, const int64_t value,
									const bool isSaved) -> bool {
			const auto object{ find(persistence, key) };
			RETURN_IF_FALSE(t.Assert(object.has_value(), true, "Key exists"));
			RETURN_IF_FALSE(t.Assert(object->GetKey(), key, "Key of wrapper"));
			RETURN_IF_FALSE(t.Assert(object->GetValue(), value, "Value of wrapper"));
			RETURN_IF_FALSE(t.Assert(object->IsSaved(), isSaved, "Saved status of wrapper"));
			return true;
		} };

		const auto readRecords{ [&t, &dataPath](std::vector<Object>& records) -> bool {
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(records, dataPath.c_str()), true, "Read pair records"));
			return true;
		} };

		PersistenceType persistence{ dirV, "pair" };

		// Emplace new keys
		RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), MSAPI::Timer{ 0 }, "Initial pair timestamp"));
		RETURN_IF_FALSE(
			t.Assert(persistence.template Emplace<SAVE_DELAY, MODIFY_SILENT>(1, 10), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(
			t.Assert(persistence.template Emplace<SAVE_DELAY, MODIFY_SILENT>(2, 20), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(checkObject(persistence, 1, 10, false));
		RETURN_IF_FALSE(checkObject(persistence, 2, 20, false));

		// Value is accepted by lvalue reference as well as by rvalue reference
		{
			PersistenceType references{ dirV, "references" };
			const int64_t constValue{ 70 };
			int64_t mutableValue{ 80 };
			RETURN_IF_FALSE(t.Assert(references.template Emplace<SAVE_DELAY, MODIFY_SILENT>(7, constValue), true,
				"Delayed pair emplacement"));
			RETURN_IF_FALSE(t.Assert(references.template Emplace<SAVE_DELAY, MODIFY_SILENT>(8, mutableValue), true,
				"Delayed pair emplacement"));
			RETURN_IF_FALSE(t.Assert(references.template Emplace<SAVE_DELAY, MODIFY_SILENT>(7, std::move(mutableValue)),
				true, "Delayed pair emplacement"));
			RETURN_IF_FALSE(checkObject(references, 7, 80, false));
			RETURN_IF_FALSE(checkObject(references, 8, 80, false));
		}
		const auto firstEmplaced{ *find(persistence, 1) };
		RETURN_IF_FALSE(t.Assert(firstEmplaced.GetModifyTime(), firstEmplaced.GetCreateTime(),
			"Modify time of emplaced object is its create time"));
		RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), find(persistence, 2)->GetCreateTime(),
			"Timestamp is the time of the last emplacement"));

		// Emplace of an existing key modifies it, several modifications before save are one record
		RETURN_IF_FALSE(
			t.Assert(persistence.template Emplace<SAVE_DELAY, MODIFY_NOTIFY>(1, 11), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(
			t.Assert(persistence.template Emplace<SAVE_DELAY, MODIFY_SILENT>(1, 12), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(checkObject(persistence, 1, 12, false));
		const auto firstModified{ *find(persistence, 1) };
		RETURN_IF_FALSE(t.Assert(
			firstModified.GetCreateTime(), firstEmplaced.GetCreateTime(), "Modification keeps the create time"));
		RETURN_IF_FALSE(t.Assert(firstModified.GetModifyTime() >= firstEmplaced.GetModifyTime(), true,
			"Modification moves the modify time forward"));
		RETURN_IF_FALSE(
			t.Assert(persistence.GetTimestamp(), firstModified.GetModifyTime(), "Timestamp is the modify time"));
		{
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
			RETURN_IF_FALSE(t.Assert(persistence.Get().size(), 2, "Modification does not add keys"));
		}

		RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save pairs"));
		RETURN_IF_FALSE(checkObject(persistence, 1, 12, true));
		RETURN_IF_FALSE(checkObject(persistence, 2, 20, true));
		std::vector<Object> records;
		RETURN_IF_FALSE(readRecords(records));
		RETURN_IF_FALSE(t.Assert(records.size(), 2, "One record per key emplaced and modified before save"));
		RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Repeat pair save without changes"));
		records.clear();
		RETURN_IF_FALSE(readRecords(records));
		RETURN_IF_FALSE(t.Assert(records.size(), 2, "Repeat save appends nothing"));

		// Modification after save appends a new record of the key
		RETURN_IF_FALSE(
			t.Assert(persistence.template Emplace<SAVE_DELAY, MODIFY_SILENT>(2, 21), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(
			t.Assert(persistence.template Emplace<SAVE_DELAY, MODIFY_NOTIFY>(2, 22), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(checkObject(persistence, 2, 22, false));
		RETURN_IF_FALSE(checkObject(persistence, 1, 12, true));
		RETURN_IF_FALSE(t.Assert(persistence.template Save<REMAIN>(), true, "Save modified pair"));
		records.clear();
		RETURN_IF_FALSE(readRecords(records));
		RETURN_IF_FALSE(t.Assert(records.size(), 3, "Modification appends one record"));
		RETURN_IF_FALSE(t.Assert(records.back().GetKey(), 2, "Appended record key"));
		RETURN_IF_FALSE(t.Assert(records.back().GetValue(), 22, "Appended record value"));

		// Read replays records, the last record of a key wins
		const auto secondModified{ *find(persistence, 2) };
		PersistenceType loaded{ dirV, "pair" };
		RETURN_IF_FALSE(t.Assert(loaded.Read(), true, "Read pairs"));
		RETURN_IF_FALSE(checkObject(loaded, 1, 12, true));
		RETURN_IF_FALSE(checkObject(loaded, 2, 22, true));
		const auto loadedSecond{ *find(loaded, 2) };
		RETURN_IF_FALSE(t.Assert(loadedSecond.GetCreateTime(), secondModified.GetCreateTime(), "Read create time"));
		RETURN_IF_FALSE(t.Assert(loadedSecond.GetModifyTime(), secondModified.GetModifyTime(), "Read modify time"));
		RETURN_IF_FALSE(t.Assert(loaded.GetTimestamp(), persistence.GetTimestamp(), "Read pair timestamp"));
		RETURN_IF_FALSE(t.Assert(loaded.Read(), false, "Read pair persistence rejects second read"));

		// Save with clearing releases memory, Read restores pairs and emplace of a read key modifies it
		RETURN_IF_FALSE(t.Assert(loaded.template Save<CLEAR>(), true, "Save pairs with clearing"));
		{
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ loaded.GetLock() };
			RETURN_IF_FALSE(t.Assert(loaded.Get().empty(), true, "Pairs are cleared"));
		}
		RETURN_IF_FALSE(t.Assert(loaded.Read(), true, "Read pairs after clearing"));
		RETURN_IF_FALSE(checkObject(loaded, 1, 12, true));
		RETURN_IF_FALSE(checkObject(loaded, 2, 22, true));
		RETURN_IF_FALSE(
			t.Assert(loaded.template Emplace<SAVE_DELAY, MODIFY_SILENT>(2, 23), true, "Delayed pair emplacement"));
		RETURN_IF_FALSE(checkObject(loaded, 2, 23, false));
		RETURN_IF_FALSE(t.Assert(find(loaded, 2)->GetCreateTime(), secondModified.GetCreateTime(),
			"Modification of read key keeps the create time"));

		// SAVE_IMMEDIATE policy saves only the object of the key right after emplacement or modification
		{
			const auto persistedPath{ dir + ".persisted" };
			PersistenceType persisted{ dirV, "persisted" };
			const auto countRecords{ [&t, &persistedPath](
										 const uint64_t expected, const std::string_view name) -> bool {
				std::vector<Object> persistedRecords;
				RETURN_IF_FALSE(t.Assert(
					IO::ReadBinaries(persistedRecords, persistedPath.c_str()), true, "Read persisted pair records"));
				RETURN_IF_FALSE(t.Assert(persistedRecords.size(), expected, name));
				return true;
			} };

			RETURN_IF_FALSE(t.Assert(
				persisted.template Emplace<SAVE_DELAY, MODIFY_SILENT>(1, 10), true, "Delayed pair emplacement"));
			RETURN_IF_FALSE(t.Assert(
				persisted.template Emplace<SAVE_IMMEDIATE, MODIFY_SILENT>(2, 20), true, "Persisted pair emplacement"));
			RETURN_IF_FALSE(checkObject(persisted, 1, 10, false));
			RETURN_IF_FALSE(checkObject(persisted, 2, 20, true));
			RETURN_IF_FALSE(countRecords(1, "Only the persisted key is appended"));
			RETURN_IF_FALSE(t.Assert(
				persisted.template Emplace<SAVE_IMMEDIATE, MODIFY_NOTIFY>(2, 21), true, "Persisted pair modification"));
			RETURN_IF_FALSE(checkObject(persisted, 2, 21, true));
			RETURN_IF_FALSE(countRecords(2, "Persisted modification appends one record"));

			// Delayed modification of a saved key, then persisted modification of the same key
			RETURN_IF_FALSE(t.Assert(
				persisted.template Emplace<SAVE_DELAY, MODIFY_SILENT>(2, 22), true, "Delayed pair modification"));
			RETURN_IF_FALSE(checkObject(persisted, 2, 22, false));
			RETURN_IF_FALSE(t.Assert(
				persisted.template Emplace<SAVE_IMMEDIATE, MODIFY_SILENT>(2, 23), true, "Persist delayed key"));
			RETURN_IF_FALSE(checkObject(persisted, 2, 23, true));
			RETURN_IF_FALSE(countRecords(3, "Persisted key appends the latest value"));

			// Save saves the delayed key only, the immediately saved key is skipped
			RETURN_IF_FALSE(t.Assert(persisted.template Save<REMAIN>(), true, "Save after persisted modification"));
			RETURN_IF_FALSE(checkObject(persisted, 1, 10, true));
			RETURN_IF_FALSE(countRecords(4, "Save skips persisted keys"));
			RETURN_IF_FALSE(t.Assert(persisted.template Save<REMAIN>(), true, "Repeat save after persisting"));
			RETURN_IF_FALSE(countRecords(4, "Repeat save appends nothing"));
			PersistenceType loadedPersisted{ dirV, "persisted" };
			RETURN_IF_FALSE(t.Assert(loadedPersisted.Read(), true, "Read persisted pairs"));
			RETURN_IF_FALSE(checkObject(loadedPersisted, 1, 10, true));
			RETURN_IF_FALSE(checkObject(loadedPersisted, 2, 23, true));
			RETURN_IF_FALSE(
				t.Assert(loadedPersisted.GetTimestamp(), persisted.GetTimestamp(), "Read persisted pair timestamp"));
		}

		// Order of objects is defined by the map, order of records is the order of changes
		if constexpr (std::is_same_v<Map<int32_t, int64_t>, std::map<int32_t, int64_t>>) {
			PersistenceType ordered{ dirV, "ordered" };
			RETURN_IF_FALSE(
				t.Assert(ordered.template Emplace<SAVE_DELAY, MODIFY_SILENT>(5, 50), true, "Delayed pair emplacement"));
			RETURN_IF_FALSE(
				t.Assert(ordered.template Emplace<SAVE_DELAY, MODIFY_SILENT>(3, 30), true, "Delayed pair emplacement"));
			RETURN_IF_FALSE(
				t.Assert(ordered.template Emplace<SAVE_DELAY, MODIFY_SILENT>(4, 40), true, "Delayed pair emplacement"));

			std::vector<int32_t> keys;
			{
				const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ ordered.GetLock() };
				for (const auto& [key, object] : ordered.Get()) {
					keys.emplace_back(key);
				}
			}

			RETURN_IF_FALSE(t.Assert(keys == std::vector<int32_t>{ 3, 4, 5 }, true, "Ordered map keeps keys order"));
			RETURN_IF_FALSE(t.Assert(ordered.template Save<REMAIN>(), true, "Save ordered pairs"));
			std::vector<Object> orderedRecords;
			RETURN_IF_FALSE(t.Assert(
				IO::ReadBinaries(orderedRecords, (dir + ".ordered").c_str()), true, "Read ordered pair records"));
			RETURN_IF_FALSE(t.Assert(orderedRecords.size(), 3, "Ordered pair records count"));
			std::vector<int32_t> recordKeys;

			for (const auto& record : orderedRecords) {
				recordKeys.emplace_back(record.GetKey());
			}

			RETURN_IF_FALSE(
				t.Assert(recordKeys == std::vector<int32_t>{ 5, 3, 4 }, true, "Records keep the order of changes"));
		}

		return true;
	} };

	const auto makeInteger{ [](const int32_t number) { return number; } };
	RETURN_IF_FALSE((testPersistence.template operator()<std::vector, int32_t>("vectorInt", makeInteger)));
	RETURN_IF_FALSE((testPersistence.template operator()<std::list, int32_t>("listInt", makeInteger)));

	const auto makeObject{ [](const int32_t number) { return TestObject{ number }; } };
	RETURN_IF_FALSE((testPersistence.template operator()<std::vector, TestObject>("vectorObject", makeObject)));
	RETURN_IF_FALSE((testPersistence.template operator()<std::list, TestObject>("listObject", makeObject)));

	RETURN_IF_FALSE((testPair.template operator()<std::map>("mapPair")));
	RETURN_IF_FALSE((testPair.template operator()<std::unordered_map>("unorderedMapPair")));

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_PERSISTENCE_INL