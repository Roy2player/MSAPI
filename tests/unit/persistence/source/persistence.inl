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
#include <thread>

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
		using Object = typename PersistenceType::Object;
		static_assert(std::is_trivially_copyable_v<Type>);
		static_assert(std::is_same_v<decltype(std::declval<const PersistenceType&>().Get()), const Container<Object>&>);
		const auto dir{ path + std::string{ suiteName } + "/" };
		const std::string_view dirV{ dir };
		const MSAPI::Timer timestamp{ 123456, 789 };
		const MSAPI::Timer updatedTimestamp{ 987654, 321 };
		const std::vector<Type> firstBatch{ makeObject(3), makeObject(1) };
		const std::vector<Type> secondBatch{ makeObject(4), makeObject(2) };

		const auto checkState{ [&t](PersistenceType& persistence, const std::vector<Type>& expected,
								   const int64_t cachedCount) -> bool {
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
			std::vector<Type> actual;
			int64_t cached{};
			for (const auto& object : persistence.Get()) {
				actual.emplace_back(object.Get());
				cached += object.IsCached();
			}
			RETURN_IF_FALSE(t.Assert(actual == expected, true, "Wrapper payloads"));
			RETURN_IF_FALSE(t.Assert(cached, cachedCount, "Cached object count"));
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
				t.Assert(IO::HasPath((missingDir + "/.data").c_str()), false, "Constructor creates no data file"));
			RETURN_IF_FALSE(t.Assert(persistence.Load(), false, "New directory has no data"));
			RETURN_IF_FALSE(checkState(persistence, {}, 0));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Save in created directory"));
			RETURN_IF_FALSE(t.Assert(persistence.Load(), true, "Load after first save"));
			RETURN_IF_FALSE(checkData(missingDir + "/.data", {}));
			RETURN_IF_FALSE(checkTimestamp(missingDir + "/.data", MSAPI::Timer{ 0 }));
			PersistenceType trailing{ std::string_view{ missingDir + "/" }, "data" };
			RETURN_IF_FALSE(t.Assert(trailing.Load(), true, "Trailing separator resolves same files"));
			PersistenceType emptyDirectory{ "", "data" };
			RETURN_IF_FALSE(t.Assert(emptyDirectory.Load(), false, "Empty directory load"));
			RETURN_IF_FALSE(t.Assert(emptyDirectory.Save(), false, "Empty directory save"));
			PersistenceType emptyName{ dirV, "" };
			RETURN_IF_FALSE(t.Assert(emptyName.Load(), false, "Empty name load"));
			RETURN_IF_FALSE(t.Assert(emptyName.Save(), false, "Empty name save"));
			PersistenceType separatorName{ dirV, "nested/data" };
			RETURN_IF_FALSE(t.Assert(separatorName.Save(), false, "Name with separator save"));
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
			RETURN_IF_FALSE(t.Assert(fullPath.Load(), false, "Reject path without terminator space on load"));
			RETURN_IF_FALSE(t.Assert(fullPath.Save(), false, "Reject path without terminator space on save"));

			const auto parent{ dir + "parentFile" };
			RETURN_IF_FALSE(t.Assert(IO::SaveStr("block", parent.c_str()), true, "Create regular-file parent"));
			PersistenceType blocked{ std::string_view{ parent + "/" }, "data" };
			RETURN_IF_FALSE(t.Assert(blocked.Load(), false, "Regular-file parent load"));
			RETURN_IF_FALSE(t.Assert(blocked.Save(), false, "Regular-file parent save"));
			const auto directoryFile{ dir + ".directoryFile" };
			RETURN_IF_FALSE(t.Assert(IO::CreateDir(directoryFile.c_str()), true, "Create data-path blocker"));
			PersistenceType directoryPersistence{ dirV, "directoryFile" };
			RETURN_IF_FALSE(t.Assert(directoryPersistence.Load(), false, "Directory data load"));
			RETURN_IF_FALSE(t.Assert(directoryPersistence.Save(), false, "Directory data save"));
		}

		{
			// Nothing to load
			PersistenceType persistence{ dirV, "empty" };
			RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), MSAPI::Timer{ 0 }, "Initial timestamp"));
			RETURN_IF_FALSE(t.Assert(persistence.Load(), false, "Missing data load"));
			RETURN_IF_FALSE(t.Assert(IO::HasPath((dir + ".empty").c_str()), false, "Failed load creates no file"));

			// Nothing to save
			persistence.UpdateTimestamp(timestamp);
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Save empty persistence"));
			RETURN_IF_FALSE(checkData(dir + ".empty", {}));
			RETURN_IF_FALSE(checkTimestamp(dir + ".empty", timestamp));
			RETURN_IF_FALSE(t.Assert(persistence.Load(), true, "Retry missing data load"));
			RETURN_IF_FALSE(t.Assert(persistence.Load(), true, "Empty persistence permits repeated load"));
			persistence.Clear();
			RETURN_IF_FALSE(t.Assert(persistence.Load(), true, "Clear permits reload"));
			RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), timestamp, "Clear preserves timestamp"));
			// Missing timestamp files retain the manually assigned timestamp.
			RETURN_IF_FALSE(
				t.Assert(IO::Remove((dir + ".empty_timestamp").c_str()), true, "Remove optional timestamp"));
			PersistenceType optional{ dirV, "empty" };
			optional.UpdateTimestamp(updatedTimestamp);
			RETURN_IF_FALSE(t.Assert(optional.Load(), true, "Load without timestamp"));
			RETURN_IF_FALSE(t.Assert(optional.GetTimestamp(), updatedTimestamp, "Keep missing timestamp value"));
			RETURN_IF_FALSE(checkState(optional, {}, 0));
		}

		{
			// Save objects
			PersistenceType persistence{ dirV, "roundTrip" };
			persistence.EmplaceBack(firstBatch[0]);
			persistence.EmplaceBack(makeObject(1));
			RETURN_IF_FALSE(checkState(persistence, firstBatch, 0));
			persistence.UpdateTimestamp(timestamp);
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Save first batch"));
			RETURN_IF_FALSE(checkState(persistence, firstBatch, 2));
			RETURN_IF_FALSE(checkData(dir + ".roundTrip", firstBatch));
			RETURN_IF_FALSE(checkTimestamp(dir + ".roundTrip", timestamp));

			// Load objects
			PersistenceType loaded{ dirV, "roundTrip" };
			RETURN_IF_FALSE(t.Assert(loaded.Load(), true, "Load first batch"));
			RETURN_IF_FALSE(checkState(loaded, firstBatch, 2));
			RETURN_IF_FALSE(t.Assert(loaded.Load(), false, "Loaded persistence rejects second load"));
			RETURN_IF_FALSE(checkState(loaded, firstBatch, 2));
			// Repeated and timestamp-only saves must leave the data file unchanged.
			std::string before;
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(before, (dir + ".roundTrip").c_str()), true, "Snapshot first save"));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Repeat save without insertions"));
			persistence.UpdateTimestamp(updatedTimestamp);
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Timestamp-only save"));
			std::string after;
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(after, (dir + ".roundTrip").c_str()), true, "Snapshot repeat save"));
			RETURN_IF_FALSE(t.Assert(after, before, "No repeated payload writes"));
			RETURN_IF_FALSE(checkTimestamp(dir + ".roundTrip", updatedTimestamp));

			// Save again
			for (const auto& object : secondBatch) {
				persistence.EmplaceBack(object);
			}
			auto allObjects{ firstBatch };
			allObjects.insert(allObjects.end(), secondBatch.begin(), secondBatch.end());
			RETURN_IF_FALSE(checkState(persistence, allObjects, 2));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Save incremental batch"));
			RETURN_IF_FALSE(checkState(persistence, allObjects, 4));
			auto diskObjects{ firstBatch };
			diskObjects.insert(diskObjects.end(), secondBatch.begin(), secondBatch.end());
			RETURN_IF_FALSE(checkData(dir + ".roundTrip", diskObjects));

			// Load again
			PersistenceType reloaded{ dirV, "roundTrip" };
			RETURN_IF_FALSE(t.Assert(reloaded.Load(), true, "Load incremental batches"));
			RETURN_IF_FALSE(checkState(reloaded, diskObjects, 4));
			RETURN_IF_FALSE(t.Assert(reloaded.GetTimestamp(), updatedTimestamp, "Reloaded timestamp"));
		}

		{
			// Append to existing files by default, then verify Clear requests a complete rewrite.
			const auto dataPath{ dir + ".rewrite" };
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinaries(wrap(firstBatch), dataPath.c_str()), true, "Preseed append file"));
			PersistenceType persistence{ dirV, "rewrite" };
			persistence.EmplaceBack(secondBatch[0]);
			RETURN_IF_FALSE(t.Assert(persistence.Load(), false, "Reject nonempty first load"));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Fresh persistence appends existing file"));
			auto appended{ firstBatch };
			appended.emplace_back(secondBatch[0]);
			RETURN_IF_FALSE(checkData(dataPath, appended));
			persistence.UpdateTimestamp(timestamp);
			persistence.Clear();
			RETURN_IF_FALSE(checkState(persistence, {}, 0));
			RETURN_IF_FALSE(t.Assert(persistence.GetTimestamp(), timestamp, "Rewrite keeps timestamp"));
			persistence.EmplaceBack(secondBatch[1]);
			const auto backupPath{ dataPath + ".backup" };
			// A failed rewrite remains pending until the blocked path is repaired.
			RETURN_IF_FALSE(t.Assert(IO::Rename(dataPath.c_str(), backupPath.c_str()), true, "Back up before blocker"));
			RETURN_IF_FALSE(t.Assert(IO::CreateDir(dataPath.c_str()), true, "Block full rewrite"));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), false, "Failed full rewrite"));
			RETURN_IF_FALSE(checkState(persistence, { secondBatch[1] }, 0));
			RETURN_IF_FALSE(t.Assert(IO::Remove(dataPath.c_str()), true, "Remove rewrite blocker"));
			RETURN_IF_FALSE(t.Assert(IO::Rename(backupPath.c_str(), dataPath.c_str()), true, "Restore old records"));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Retry full rewrite"));
			RETURN_IF_FALSE(checkData(dataPath, { secondBatch[1] }));
			persistence.Clear();
			RETURN_IF_FALSE(t.Assert(persistence.Load(), true, "Clear permits nonempty reload"));
			RETURN_IF_FALSE(checkState(persistence, { secondBatch[1] }, 1));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Rewrite includes cached loaded objects"));
			RETURN_IF_FALSE(checkData(dataPath, { secondBatch[1] }));
			persistence.Clear();
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Clear truncates to empty"));
			RETURN_IF_FALSE(checkData(dataPath, {}));
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinaries(wrap(firstBatch), dataPath.c_str()), true, "Preseed after empty save"));
			persistence.Clear();
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Clear empty persistence still rewrites"));
			RETURN_IF_FALSE(checkData(dataPath, {}));
		}

		{
			// Retry timestamp failures without writing already persisted payloads again.
			const auto dataPath{ dir + ".retry" };
			const auto timestampPath{ dataPath + "_timestamp" };
			PersistenceType persistence{ dirV, "retry" };
			persistence.EmplaceBack(firstBatch[0]);
			persistence.UpdateTimestamp(timestamp);
			RETURN_IF_FALSE(t.Assert(IO::CreateDir(timestampPath.c_str()), true, "Block timestamp write"));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), false, "Timestamp failure after data write"));
			RETURN_IF_FALSE(checkState(persistence, { firstBatch[0] }, 1));
			RETURN_IF_FALSE(checkData(dataPath, { firstBatch[0] }));
			RETURN_IF_FALSE(t.Assert(IO::Remove(timestampPath.c_str()), true, "Repair timestamp path"));
			RETURN_IF_FALSE(t.Assert(persistence.Save(), true, "Retry timestamp without duplicate records"));
			RETURN_IF_FALSE(checkData(dataPath, { firstBatch[0] }));
			RETURN_IF_FALSE(checkTimestamp(dataPath, timestamp));
			// Malformed timestamp reads must preserve memory and permit a repaired retry.
			RETURN_IF_FALSE(t.Assert(IO::SaveStr("x", timestampPath.c_str()), true, "Truncate timestamp fixture"));
			PersistenceType loaded{ dirV, "retry" };
			loaded.UpdateTimestamp(updatedTimestamp);
			RETURN_IF_FALSE(t.Assert(loaded.Load(), false, "Reject malformed timestamp"));
			RETURN_IF_FALSE(checkState(loaded, {}, 0));
			RETURN_IF_FALSE(t.Assert(loaded.GetTimestamp(), updatedTimestamp, "Failed load keeps timestamp"));
			RETURN_IF_FALSE(t.Assert(IO::SaveBinary(timestamp, timestampPath.c_str()), true, "Repair timestamp file"));
			RETURN_IF_FALSE(t.Assert(loaded.Load(), true, "Retry malformed timestamp load"));
			RETURN_IF_FALSE(checkState(loaded, { firstBatch[0] }, 1));
			// A partial trailing record must not commit earlier staged payloads to memory.
			const int8_t partial{ 1 };
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinary<IO::APPEND>(partial, dataPath.c_str()), true, "Append partial record"));
			loaded.Clear();
			RETURN_IF_FALSE(t.Assert(loaded.Load(), false, "Reject malformed data after valid record"));
			RETURN_IF_FALSE(checkState(loaded, {}, 0));
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinaries(wrap(firstBatch), dataPath.c_str()), true, "Repair malformed data"));
			RETURN_IF_FALSE(t.Assert(loaded.Load(), true, "Retry malformed data load"));
			RETURN_IF_FALSE(checkState(loaded, firstBatch, 2));
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
						if (worker < 2) {
							persistence.EmplaceBack(makeObject(100 + worker * 64 + index));
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
							const bool saved{ t.Assert(persistence.Save(), true, "Concurrent incremental save") };
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
				saves[0] = t.Assert(persistence.Save(), true, "First competing save");
			} };
			std::jthread secondSave{ [&] {
				saveStart.arrive_and_wait();
				saves[1] = t.Assert(persistence.Save(), true, "Second competing save");
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

			PersistenceType loaded{ dirV, "concurrent" };
			// Only one of two competing loads may consume the successful-load allowance.
			std::barrier loadStart{ 2 };
			std::array<bool, 2> loads{};
			std::jthread firstLoad{ [&] {
				loadStart.arrive_and_wait();
				loads[0] = loaded.Load();
			} };
			std::jthread secondLoad{ [&] {
				loadStart.arrive_and_wait();
				loads[1] = loaded.Load();
			} };
			firstLoad.join();
			secondLoad.join();
			RETURN_IF_FALSE(t.Assert(loads[0] != loads[1], true, "Exactly one competing load succeeds"));
			RETURN_IF_FALSE(checkState(loaded, diskObjects, 96));

			// Guarded readers remain valid while another thread clears and rewrites the persistence.
			auto allowed{ expected };
			allowed.insert(allowed.end(), secondBatch.begin(), secondBatch.end());
			std::barrier rewriteStart{ 2 };
			bool lookupsValid{ true };
			std::jthread lookup{ [&] {
				rewriteStart.arrive_and_wait();
				for (int32_t iteration{}; iteration < 48; ++iteration) {
					const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ persistence.GetLock() };
					const auto& objects{ persistence.Get() };
					const bool valid{ t.Assert(std::all_of(objects.begin(), objects.end(),
												   [&allowed](const auto& object) {
													   return std::find(allowed.begin(), allowed.end(), object.Get())
														   != allowed.end();
												   }),
						true, "Concurrent lookup during rewrite") };
					lookupsValid = valid && lookupsValid;
				}
			} };
			rewriteStart.arrive_and_wait();
			persistence.Clear();
			for (const auto& object : secondBatch) {
				persistence.EmplaceBack(object);
			}
			const bool saved{ persistence.Save() };
			lookup.join();
			RETURN_IF_FALSE(t.Assert(lookupsValid && saved, true, "Guarded lookup during clear and rewrite"));
			RETURN_IF_FALSE(checkState(persistence, secondBatch, 2));
			RETURN_IF_FALSE(checkData(dir + ".concurrent", secondBatch));
		}
		return true;
	} };

	const auto makeInteger{ [](const int32_t number) { return number; } };
	RETURN_IF_FALSE((testPersistence.template operator()<std::vector, int32_t>("vectorInt", makeInteger)));
	RETURN_IF_FALSE((testPersistence.template operator()<std::list, int32_t>("listInt", makeInteger)));

	const auto makeObject{ [](const int32_t number) { return TestObject{ number }; } };
	RETURN_IF_FALSE((testPersistence.template operator()<std::vector, TestObject>("vectorObject", makeObject)));
	RETURN_IF_FALSE((testPersistence.template operator()<std::list, TestObject>("listObject", makeObject)));

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_PERSISTENCE_INL