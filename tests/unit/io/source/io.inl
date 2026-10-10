/**************************
 * @file        io.inl
 * @date        2025-12-13
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

#ifndef MSAPI_UNIT_TEST_IO_INL
#define MSAPI_UNIT_TEST_IO_INL

#include "../../../../library/source/help/io.inl"
#include "../../../../library/source/test/test.inl"
#include <array>
#include <forward_list>
#include <list>
#include <ranges>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for IO.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool Io();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool Io()
{
	static_assert(IO::APPEND, "Append global is true");
	static_assert(!IO::OVERWRITE, "Overwrite global is false");

	static_assert(IO::MULTIPLE, "Multiple global is true");
	static_assert(!IO::SINGLE, "Single global is false");

	static_assert(IO::SuggestFlags(true) == (O_WRONLY | O_CREAT | O_APPEND), "SuggestFlags true failed");
	static_assert(IO::SuggestFlags(false) == (O_WRONLY | O_CREAT | O_TRUNC), "SuggestFlags false failed");

	static_assert(IO::SuggestPsm<int8_t, 32>() == 4, "PSM for int8");
	static_assert(IO::SuggestPsm<uint8_t, 32>() == 3, "PSM for uint8");
	static_assert(IO::SuggestPsm<int16_t, 32>() == 6, "PSM for int16");
	static_assert(IO::SuggestPsm<uint16_t, 32>() == 5, "PSM for uint16");
	static_assert(IO::SuggestPsm<int32_t, 32>() == 11, "PSM for int32");
	static_assert(IO::SuggestPsm<uint32_t, 32>() == 10, "PSM for uint32");
	static_assert(IO::SuggestPsm<int64_t, 32>() == 20, "PSM for int64");
	static_assert(IO::SuggestPsm<uint64_t, 32>() == 20, "PSM for uint64");
	static_assert(IO::SuggestPsm<float, 32>() == 14, "PSM for float");
	static_assert(IO::SuggestPsm<double, 31>() == 32, "PSM for double, less than minimum");
	static_assert(IO::SuggestPsm<double, 33>() == 33, "PSM for double, greater than minimum");
	static_assert(IO::SuggestPsm<long double, 31>() == 32, "PSM for long double, less than minimum");
	static_assert(IO::SuggestPsm<long double, 33>() == 33, "PSM for long double, greater than minimum");
	static_assert(IO::SuggestPsm<bool, 32>() == 4, "PSM for bool");
	static_assert(IO::SuggestPsm<char, 32>() == 1, "PSM for char");

	static_assert(IO::EnumToString(IO::FileType::Unknown) == "Unknown", "EnumToString Unknown failed");
	static_assert(IO::EnumToString(IO::FileType::Fifo) == "Fifo", "EnumToString Fifo failed");
	static_assert(IO::EnumToString(IO::FileType::Char) == "Char", "EnumToString Char failed");
	static_assert(IO::EnumToString(IO::FileType::Directory) == "Directory", "EnumToString Directory failed");
	static_assert(IO::EnumToString(IO::FileType::Blk) == "Blk", "EnumToString Blk failed");
	static_assert(IO::EnumToString(IO::FileType::Regular) == "Regular", "EnumToString Regular failed");
	static_assert(IO::EnumToString(IO::FileType::Lnk) == "Lnk", "EnumToString Lnk failed");
	static_assert(IO::EnumToString(IO::FileType::Sock) == "Sock", "EnumToString Sock failed");

	LOG_INFO("MSAPI UNIT TEST IO");
	MSAPI::Test::Test t;

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

	std::string testData;
	testData.reserve(16384);
	std::string readData;
	readData.reserve(16384);
	{
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathV), false, "Dir should not exist"));
		RETURN_IF_FALSE(t.Assert(IO::CreateDir(pathV), true, "Create dir"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathV), true, "Dir should exist now"));

		const auto& pathChild3{ path + "childDir/childDir2/childDir3" };
		const std::string_view pathChild3V{ pathChild3 };
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathChild3V), false, "Nested dir should not exist"));
		RETURN_IF_FALSE(t.Assert(IO::CreateDir(pathChild3V), true, "Create nested dir"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathChild3V), true, "Nested dir should exist now"));

		const auto& path1{ path + "someNameForFileToTest1" };
		const std::string_view path1V{ path1 };
		RETURN_IF_FALSE(t.Assert(IO::HasPath(path1V), false, "File should not exist"));
		for (int i{}; i < 200; ++i) {
			if (i % 4 == 0) {
				testData += "\n";
			}
			std::format_to(std::back_inserter(testData), "{} {}", i, "Some test data is here");
		}
		RETURN_IF_FALSE(t.Assert(IO::SaveStr(testData, path1V), true, "Save str to file"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(path1V), true, "File should exist now"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, path1V), true, "Read str from file"));
		RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

		RETURN_IF_FALSE(
			t.Assert(IO::SaveStr<IO::APPEND>("2 Some test data is here", path1V), true, "Overwrite str to file"));
		testData += "\n2 Some test data is here";
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, path1V), true, "Read str from file"));
		RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

		RETURN_IF_FALSE(t.Assert(IO::SaveStr("3 Some test data is here", path1V), true, "Overwrite str to file"));
		testData = "3 Some test data is here";
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, path1V), true, "Read str from file"));
		RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

		const auto& path2{ path + "someNameForFileToTest2" };
		const std::string_view path2V{ path2 };
		RETURN_IF_FALSE(t.Assert(IO::CopyFile(path1V, path2V), true, "Copy file"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(path2V), true, "Copied file should exist"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, path2V), true, "Read str from copied file"));
		RETURN_IF_FALSE(t.Assert(readData, testData, "Read data from copied file should be equal to saved data"));

		const auto& pathRenamed{ path + "someRenamedFile" };
		const std::string_view pathRenamedV{ pathRenamed };
		RETURN_IF_FALSE(t.Assert(IO::Rename(path2V, pathRenamedV), true, "Rename file"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(path2V), false, "Old file should not exist now"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathRenamedV), true, "Renamed file should exist now"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathRenamedV), true, "Read str from renamed file"));
		RETURN_IF_FALSE(t.Assert(readData, testData, "Read data from renamed file should be equal to saved data"));

		const auto& pathCopied{ path + "childDir/childDir2/childDir3/someCopiedFile" };
		const std::string_view pathCopiedV{ pathCopied };
		RETURN_IF_FALSE(t.Assert(IO::CopyFile(pathRenamedV, pathCopiedV), true, "Copy file to nested dir"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathCopiedV), true, "Copied to nested dir file should exist"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathCopiedV), true, "Read str from copied to nested dir file"));
		RETURN_IF_FALSE(
			t.Assert(readData, testData, "Read data from copied to nested dir file should be equal to saved data"));

		const auto testList{ [&t](const auto pathOrDirChild3, const auto pathOrDir, const auto pathOrDirChildDir2) {
			{
				std::set<std::string> names;
				RETURN_IF_FALSE(t.Assert(
					IO::List<IO::FileType::Regular>(names, pathOrDirChild3), true, "List files in nested dir"));
				RETURN_IF_FALSE(t.Assert(names.size(), 1, "There should be one file in nested dir"));
				RETURN_IF_FALSE(t.Assert(*names.begin(), "someCopiedFile", "File name should be correct"));
			}

			std::vector<std::string> names;
			RETURN_IF_FALSE(
				t.Assert(IO::List<IO::FileType::Regular>(names, pathOrDir), true, "List files in test dir"));
			RETURN_IF_FALSE(t.Assert(names.size(), 2, "There should be two files in test dir"));
			RETURN_IF_FALSE(t.Assert(names[0] == "someRenamedFile" || names[0] == "someNameForFileToTest1", true,
				"First file name should be correct"));
			RETURN_IF_FALSE(t.Assert(names[1] == "someRenamedFile" || names[1] == "someNameForFileToTest1", true,
				"Second file name should be correct"));
			RETURN_IF_FALSE(t.Assert(names[0] != names[1], true, "File names should be different"));

			names.clear();
			RETURN_IF_FALSE(
				t.Assert(IO::List<IO::FileType::Directory>(names, pathOrDir), true, "List dirs in test dir"));
			RETURN_IF_FALSE(t.Assert(names.size(), 1, "There should be one dir in test dir"));
			RETURN_IF_FALSE(t.Assert(names[0], "childDir", "Dir name should be correct"));

			names.clear();
			RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Regular>(names, pathOrDirChildDir2), true,
				"Listing files in nested dir level 2 should succeed"));
			RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no files in nested dir level 2"));

			names.clear();
			RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Directory>(names, pathOrDirChild3), true,
				"Listing dirs in nested dir level 3 should succeed"));
			RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no dirs in nested dir level 3"));

			return true;
		} };

		{
			const auto pathOrDirChildDir2{ path + "childDir/childDir2" };
			RETURN_IF_FALSE(t.Assert(testList(pathChild3V, pathV, std::string_view{ pathOrDirChildDir2 }), true,
				"Test listing files and dirs with paths"));
		}

		{
			MSAPI::IO::DirGuard dirPathChild3{ pathChild3V };
			RETURN_IF_FALSE(t.Assert(dirPathChild3.value != nullptr, true, "Open directory"));
			MSAPI::IO::DirGuard dirPath{ pathV };
			RETURN_IF_FALSE(t.Assert(dirPath.value != nullptr, true, "Open directory"));
			MSAPI::IO::DirGuard dirPathChildDir2{ std::string_view{ path + "childDir/childDir2" } };
			RETURN_IF_FALSE(t.Assert(dirPathChildDir2.value != nullptr, true, "Open directory"));
			RETURN_IF_FALSE(t.Assert(testList(dirPathChild3.value, dirPath.value, dirPathChildDir2.value), true,
				"Test listing files and dirs with dirs"));
		}

		{
			std::vector<std::string> names;
			const auto& pathNonExistingDir{ path + "nonExistingDir" };
			const std::string_view pathNonExistingDirV{ pathNonExistingDir };
			RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Regular>(names, pathNonExistingDirV), false,
				"Listing files in non existing dir should fail"));
			RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no files in non existing dir"));

			names.clear();
			RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Directory>(names, pathNonExistingDirV), false,
				"Listing dirs in non existing dir should fail"));
			RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no dirs in non existing dir"));
		}

		RETURN_IF_FALSE(t.Assert(IO::Remove(path1V), true, "Remove file"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(path1V), false, "File should not exist now"));
		const auto& pathChild1{ path + "childDir" };
		const std::string_view pathChild1V{ pathChild1 };
		RETURN_IF_FALSE(t.Assert(IO::Remove(pathChild1V), true, "Remove nested dir"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathChild1V), false, "Nested dir should not exist now"));

		const auto& renamedChildPath{ path + "renamedChildDir" };
		const std::string_view renamedChildPathV{ renamedChildPath };
		RETURN_IF_FALSE(
			t.Assert(IO::Rename(pathChild1V, renamedChildPathV), false, "Renaming non existing dir should fail"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathChild1V), false, "Non existing dir should still not exist"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(renamedChildPathV), false, "Renamed non existing dir should not exist"));
	}

	{
		const auto testPrimitive{ [&t, &testData, &readData, &path](const auto& data, const auto& dataD) {
			testData.clear();
			std::format_to(std::back_inserter(testData), "{}", data[0]);
			for (uint64_t i{ 2 }; i <= data.size(); ++i) {
				std::format_to(std::back_inserter(testData), ",{}", data[i - 1]);
			}
			const auto testDataCopy{ testData };

			const auto& pathPrimitives{ path + "primitives" };
			const std::string_view pathPrimitivesV{ pathPrimitives };
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives(data, pathPrimitivesV, ','), true, "Save primitives"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read primitives from file"));
			RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

			constexpr std::string_view sectionSeparator{ "==================================================" };
			RETURN_IF_FALSE(t.Assert(IO::SaveStr<IO::APPEND>(sectionSeparator, pathPrimitivesV), true,
				"Overwrite primitives file with some other data"));
			std::format_to(std::back_inserter(testData), "\n{}", sectionSeparator);

			const auto& pathPrimitivesCopy{ path + "primitivesCopy" };
			const std::string_view pathPrimitivesCopyV{ pathPrimitivesCopy };
			RETURN_IF_FALSE(t.Assert(IO::CopyFile(pathPrimitivesV, pathPrimitivesCopyV), true, "Copy primitives file"));
			RETURN_IF_FALSE(
				t.Assert(IO::ReadStr(readData, pathPrimitivesCopyV), true, "Read str from copied primitives file"));
			RETURN_IF_FALSE(
				t.Assert(readData, testData, "Read data from copied primitives file should be equal to saved data"));

			RETURN_IF_FALSE(t.Assert(IO::SaveStr<IO::APPEND>(readData, pathPrimitivesV), true,
				"Append copied primitives data to original primitives file"));
			std::format_to(std::back_inserter(testData), "\n{}", readData);
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read str from primitives file"));
			RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives<IO::APPEND>(data, pathPrimitivesV, ','), true,
				"Append primitives to primitives file"));
			std::format_to(std::back_inserter(testData), "\n{}", testDataCopy);
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read str from primitives file"));
			RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

			RETURN_IF_FALSE(
				t.Assert(IO::SavePrimitives(data, pathPrimitivesV, ','), true, "Overwrite primitives file"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read str from primitives file"));
			RETURN_IF_FALSE(t.Assert(readData, testDataCopy, "Read data should be equal to saved data"));

			// Iterator pairs preserve whole-container formatting without changing the existing save interface.
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives(data.cbegin(), data.cend(), pathPrimitivesV, ','), true,
				"Save primitive iterator range"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read primitive iterator range"));
			RETURN_IF_FALSE(t.Assert(readData, testDataCopy, "Primitive iterator formatting matches container"));

			testData.clear();
			std::format_to(std::back_inserter(testData), "{}", _S(dataD[0]));
			for (uint64_t i{ 2 }; i <= dataD.size(); ++i) {
				std::format_to(std::back_inserter(testData), ",{}", _S(dataD[i - 1]));
			}

			const auto& pathD{ path + "primitivesD" };
			const std::string_view pathDV{ pathD };
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives(dataD, pathDV, ','), true, "Save primitives"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathDV), true, "Read primitives from file"));
			RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

			return true;
		} };

		{
			std::vector<int32_t> data;
			data.emplace_back(-1);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				data.emplace_back(i * ((-1 * i % 2) | 0x01));
			}

			std::vector<double> dataD;
			dataD.emplace_back(-1.);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				dataD.emplace_back(i * ((-1 * i % 2) | 0x01) / 3.);
			}

			RETURN_IF_FALSE(t.Assert(testPrimitive(data, dataD), true, "Test primitive int32 and double"));
		}

		{
			std::vector<uint64_t> data;
			data.emplace_back(1);
			for (uint64_t i{ 2 }; i <= 4096; ++i) {
				data.emplace_back(i * ((i % 2) | 0x01));
			}

			std::vector<float> dataD;
			dataD.emplace_back(-1.f);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				dataD.emplace_back(static_cast<float>(i) * (static_cast<float>((-1 * i % 2) | 0x01) / 3.f));
			}

			RETURN_IF_FALSE(t.Assert(testPrimitive(data, dataD), true, "Test primitive uint64 and float"));
		}

		{
			std::vector<uint32_t> data;
			data.emplace_back(1);
			for (uint32_t i{ 2 }; i <= 4096; ++i) {
				data.emplace_back(i * ((i % 2) | 0x01));
			}

			std::vector<int64_t> dataD;
			dataD.emplace_back(-1);
			for (int64_t i{ 2 }; i <= 4096; ++i) {
				dataD.emplace_back(i * ((-1 * i % 2) | 0x01));
			}

			RETURN_IF_FALSE(t.Assert(testPrimitive(data, dataD), true, "Test primitive uint32 and int64"));
		}

		{
			std::vector<uint16_t> data;
			data.emplace_back(1);
			for (uint16_t i{ 2 }; i <= 4096; ++i) {
				data.emplace_back(i * ((i % 2) | 0x01));
			}

			std::vector<int16_t> dataD;
			dataD.emplace_back(-1);
			for (int16_t i{ 2 }; i <= 4096; ++i) {
				dataD.emplace_back(i * ((-1 * i % 2) | 0x01));
			}

			RETURN_IF_FALSE(t.Assert(testPrimitive(data, dataD), true, "Test primitive uint16 and int16"));
		}

		{
			std::vector<uint8_t> data;
			data.emplace_back(1);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				uint8_t j{ static_cast<uint8_t>(i % 256) };
				data.emplace_back(j * ((j % 2) | 0x01));
			}

			std::vector<int8_t> dataD;
			dataD.emplace_back(-1);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				int8_t j{ static_cast<int8_t>(i % 256) };
				dataD.emplace_back(j * ((-1 * j % 2) | 0x01));
			}

			RETURN_IF_FALSE(t.Assert(testPrimitive(data, dataD), true, "Test primitive uint8 and int8"));
		}

		{
			std::vector<char> data;
			data.emplace_back(1);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				data.emplace_back(i % 94 + 32);
			}

			std::vector<bool> dataD;
			dataD.emplace_back(false);
			for (int32_t i{ 2 }; i <= 4096; ++i) {
				dataD.emplace_back(i % 2 == 0);
			}

			RETURN_IF_FALSE(t.Assert(testPrimitive(data, dataD), true, "Test primitive char and bool"));
		}
	}

	{
		struct TestStruct {
			uint64_t x1{};
			double x2{};
			bool x3{};
			int64_t x4{};

			bool operator==(const TestStruct& other) const noexcept
			{
				return x1 == other.x1 && Helper::FloatEqual(x2, other.x2) && x3 == other.x3 && x4 == other.x4;
			}

			std::string ToString() const noexcept
			{
				return std::format("TestStruct{{"
								   "\n\tx1: {}"
								   "\n\tx2: {:.17f}"
								   "\n\tx3: {}"
								   "\n\tx4: {}"
								   "\n}}",
					x1, x2, x3, x4);
			}
		};

		std::vector<TestStruct> vec;
		vec.reserve(8192);
		std::vector<TestStruct> vecRead;
		vecRead.reserve(8192);

		const auto testBinary{ [&t, &vec, &vecRead](const auto o1PathOrFd, std::string_view o1Path,
								   const auto o3PathOrFd, std::string_view o3Path, const auto vecPathOrFd,
								   std::string_view vecPath) {
			vec.clear();
			vecRead.clear();

			TestStruct o1{ 0x1122334455667788, 3.14159265358979323, true, -1234567890123456789 };
			RETURN_IF_FALSE(t.Assert(IO::SaveBinary(&o1, o1PathOrFd), true, "Save binary struct"));
			TestStruct o2{};
			RETURN_IF_FALSE(t.Assert(IO::ReadBinary(&o2, o1Path), true, "Read binary struct"));
			RETURN_IF_FALSE(t.Assert(o2, o1, "Read struct should be equal to saved struct"));
			// Descriptor is read from the beginning of the file regardless of its offset after saving
			o2 = TestStruct{};
			RETURN_IF_FALSE(
				t.Assert(IO::ReadBinary(&o2, o1PathOrFd), true, "Read binary struct by path or descriptor"));
			RETURN_IF_FALSE(t.Assert(o2, o1, "Struct read by path or descriptor should be equal to saved struct"));
			TestStruct o3{};
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinary<IO::APPEND>(&o3, o3PathOrFd), true, "Save binary struct in append mode"));
			RETURN_IF_FALSE(t.Assert(IO::ReadBinary(&o2, o3Path), true, "Read binary struct from append file"));
			RETURN_IF_FALSE(t.Assert(o2, o3, "Read struct from append file should be equal to saved struct"));
			vec.push_back(o3);
			vec.push_back(o3);
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinary<IO::APPEND>(&o3, o3PathOrFd), true, "Save binary struct in append mode"));
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, o3Path), true, "Read binaries from append file"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Read structs from append file should be equal to saved structs"));
			vecRead.clear();
			RETURN_IF_FALSE(t.Assert(
				IO::ReadBinaries(vecRead, o3PathOrFd), true, "Read binaries from append file by path or descriptor"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Structs read by path or descriptor from append file"));
			vec.erase(vec.end() - 1);
			vecRead.clear();
			RETURN_IF_FALSE(t.Assert(IO::SaveBinary(&o3, o3PathOrFd), true, "Save binary struct in overwrite mode"));
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, o3Path), true, "Read binaries from overwritten file"));
			RETURN_IF_FALSE(
				t.Assert(vecRead, vec, "Read structs from overwritten file should be equal to saved structs"));

			vec.clear();
			vecRead.clear();
			for (uint64_t i{ 1 }; i <= 8192; ++i) {
				vec.push_back(TestStruct{ i, static_cast<double>(i) / 7. + 0.12345678901234567, (i % 2) == 0,
					-static_cast<int64_t>(i * 1234567890) });
			}
			RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(vec, vecPathOrFd), true, "Save binaries"));
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, vecPath), true, "Read binaries"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Read binaries should be equal to saved binaries"));
			vecRead.clear();
			RETURN_IF_FALSE(
				t.Assert(IO::ReadBinaries(vecRead, vecPathOrFd), true, "Read binaries by path or descriptor"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Binaries read by path or descriptor"));

			// Iterator pairs preserve the same binary format for paths and open descriptors.
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinaries(vec.cbegin(), vec.cend(), vecPathOrFd), true, "Save binary iterator range"));
			vecRead.clear();
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, vecPath), true, "Read binary iterator range"));
			RETURN_IF_FALSE(t.Assert(vecRead == vec, true, "Binary iterator format matches container"));

			for (uint64_t i{}; i < vec.size(); i += 256) {
				using S = typename decltype(vec)::value_type;
				S v{};
				vec[i] = v;
				RETURN_IF_FALSE(t.Assert(IO::SaveBinaryOnOffset(&v, vecPathOrFd, INT64(i) * INT64(sizeof(S))), true,
					"Save binary struct at specific offset"));
			}
			vecRead.clear();
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, vecPath), true, "Read binaries after offset saves"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Read binaries after offset saves should be equal to expected"));
			vecRead.clear();
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, vecPathOrFd), true,
				"Read binaries after offset saves by path or descriptor"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Binaries read by path or descriptor after offset saves"));

			return true;
		} };

		const auto& pathO1{ path + "o1" };
		const std::string_view pathO1V{ pathO1 };
		const auto& pathO3{ path + "o3" };
		const std::string_view pathO3V{ pathO3 };
		const auto& pathVec{ path + "vec" };
		const std::string_view pathVecV{ pathVec };
		RETURN_IF_FALSE(t.Assert(
			testBinary(pathO1V, pathO1V, pathO3V, pathO3V, pathVecV, pathVecV), true, "Test binary with paths"));

		const auto& pathFd1{ path + "o1Fd" };
		const std::string_view pathFd1V{ pathFd1 };
		const auto& pathFd3{ path + "o3Fd" };
		const std::string_view pathFd3V{ pathFd3 };
		const auto& pathVecFd{ path + "vecFd" };
		const std::string_view pathVecFdV{ pathVecFd };
		IO::FileGuard fd1;
		RETURN_IF_FALSE(t.Assert(fd1.value, -1, "Open empty file descriptor for o1Fd"));
		fd1 = IO::FileGuard{ pathFd1V, O_RDWR | O_CREAT, 0644 };
		RETURN_IF_FALSE(t.Assert(fd1.value != -1, true, "Open initialized file descriptor for o1Fd"));

		int32_t fd;
		{
			IO::FileGuard fd3{ pathFd3V, O_RDWR | O_CREAT, 0644 };
			fd = fd3.value;
			RETURN_IF_FALSE(t.Assert(fd3.value != -1, true, "Open file descriptor for o3Fd"));
			IO::FileGuard fdVec{ pathVecFdV, O_RDWR | O_CREAT, 0644 };
			IO::FileGuard fdVec2{ std::move(fdVec) };
			RETURN_IF_FALSE(t.Assert(fdVec.value, -1, "Open file descriptor for vecFd"));
			RETURN_IF_FALSE(t.Assert(fdVec2.value != -1, true, "Open file descriptor for fdVec2"));

			RETURN_IF_FALSE(t.Assert(testBinary(fd1.value, pathFd1V, fd3.value, pathFd3V, fdVec2.value, pathVecFdV),
				true, "Test binary with file descriptors"));
		}
		RETURN_IF_FALSE(t.Assert(fcntl(fd, F_GETFD) == -1 && errno == EBADF, true,
			"File descriptor should be closed in OS after Guard destruction"));

		fd = fd1.value;
		fd1.Clear();
		RETURN_IF_FALSE(t.Assert(fd1.value, -1, "File descriptor should be closed after Clear()"));
		RETURN_IF_FALSE(t.Assert(
			fcntl(fd, F_GETFD) == -1 && errno == EBADF, true, "File descriptor should be closed in OS after Clear()"));
	}

	{
		struct TestStruct {
			uint64_t x1;
			double x2;
			bool x3;
			int64_t x4;

			TestStruct() = delete;

			explicit TestStruct(const uint64_t x1, const double x2, const bool x3, const int64_t x4) noexcept
				: x1{ x1 }
				, x2{ x2 }
				, x3{ x3 }
				, x4{ x4 }
			{
			}

			bool operator==(const TestStruct& other) const noexcept
			{
				return x1 == other.x1 && Helper::FloatEqual(x2, other.x2) && x3 == other.x3 && x4 == other.x4;
			}

			bool operator<(const TestStruct& other) const noexcept
			{
				if (x1 != other.x1) {
					return x1 < other.x1;
				}

				const int cmp{ Helper::CompareFloats(x2, other.x2) };
				if (cmp != 0) {
					return cmp < 0;
				}

				if (x3 != other.x3) {
					return x3 < other.x3;
				}

				return x4 < other.x4;
			}

			std::string ToString() const noexcept
			{
				return std::format("TestStruct{{"
								   "\n\tx1: {}"
								   "\n\tx2: {:.17f}"
								   "\n\tx3: {}"
								   "\n\tx4: {}"
								   "\n}}",
					x1, x2, x3, x4);
			}
		};

		std::set<TestStruct> set;
		for (uint64_t i{ 1 }; i <= 4096; ++i) {
			set.emplace(TestStruct{ i, static_cast<double>(i) / 7. + 0.12345678901234567, (i % 2) == 0,
				-static_cast<int64_t>(i * 1234567890) });
		}
		const auto& pathSet{ path + "set" };
		const std::string_view pathSetV{ pathSet };
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(set, pathSetV), true, "Save binaries from set"));
		std::set<TestStruct> setRead;
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(setRead, pathSetV), true, "Read binaries to set"));
		RETURN_IF_FALSE(t.Assert(setRead, set, "Read set should be equal to saved set"));
	}

	{
		// Half-open binary ranges exclude surrounding elements and retain append/overwrite semantics.
		const std::array<int32_t, 5> values{ 11, 22, 33, 44, 55 };
		const std::vector<int32_t> selected{ 22, 33, 44 };
		const auto rangePath{ path + "binaryRanges" };
		std::vector<int32_t> actual;

		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(values.data() + 1, values.data() + 4, rangePath.c_str()), true,
			"Save raw-pointer binary subrange"));
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read binary subrange"));
		RETURN_IF_FALSE(t.Assert(actual == selected, true, "Binary subrange excludes endpoints"));
		RETURN_IF_FALSE(
			t.Assert(IO::SaveBinaries<IO::APPEND>(values.cbegin() + 1, values.cbegin() + 4, rangePath.c_str()), true,
				"Append binary const-iterator subrange"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read appended binary range"));
		RETURN_IF_FALSE(t.Assert(actual == std::vector<int32_t>{ 22, 33, 44, 22, 33, 44 }, true,
			"Appended binary ranges contain only selected records"));

		// Forward-only and distinct-sentinel ranges use the same interface.
		const std::forward_list<int32_t> linked{ 11, 22, 33, 44, 55 };
		RETURN_IF_FALSE(
			t.Assert(IO::SaveBinaries(std::next(linked.cbegin()), std::next(linked.cbegin(), 4), rangePath.c_str()),
				true, "Save forward-only binary subrange"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read forward-only records"));
		RETURN_IF_FALSE(t.Assert(actual == selected, true, "Forward-only binary payloads"));
		RETURN_IF_FALSE(t.Assert(
			IO::SaveBinaries(std::counted_iterator{ values.data() + 1, 3 }, std::default_sentinel, rangePath.c_str()),
			true, "Save binary distinct sentinel"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read distinct-sentinel records"));
		RETURN_IF_FALSE(t.Assert(actual == selected, true, "Distinct-sentinel binary payloads"));

		// Lazy values and pointer records serialize payload bytes, not adapters or addresses.
		int64_t predicateCalls{};
		auto filtered{ values | std::views::filter([&predicateCalls](const int32_t value) {
			++predicateCalls;
			return value % 22 == 0;
		}) | std::views::transform([](const int32_t value) { return value * 10; }) };

		RETURN_IF_FALSE(t.Assert(
			IO::SaveBinaries(filtered.begin(), filtered.end(), rangePath.c_str()), true, "Save lazy binary range"));
		RETURN_IF_FALSE(t.Assert(predicateCalls, values.size(), "Binary iterator save evaluates filter once"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read lazy binary payloads"));
		RETURN_IF_FALSE(t.Assert(actual == std::vector<int32_t>{ 220, 440 }, true, "Lazy binary selection"));
		const std::array<const int32_t*, 3> pointers{ &values[1], &values[2], &values[3] };
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(pointers.cbegin(), pointers.cend(), rangePath.c_str()), true,
			"Save pointer-record iterator range"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read pointed-to records"));
		RETURN_IF_FALSE(t.Assert(actual == selected, true, "Pointer records contain payloads"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(testData, rangePath.c_str()), true, "Snapshot pointer record bytes"));
		RETURN_IF_FALSE(t.Assert(testData.size(), selected.size() * sizeof(int32_t), "Range binary record size"));
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries<IO::APPEND>(values.begin(), values.begin(), rangePath.c_str()), true,
			"Empty binary append"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, rangePath.c_str()), true, "Read empty binary append"));
		RETURN_IF_FALSE(t.Assert(readData, testData, "Empty binary append preserves bytes"));
		RETURN_IF_FALSE(t.Assert(
			IO::SaveBinaries(values.begin(), values.begin(), rangePath.c_str()), true, "Empty binary overwrite"));
		RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, rangePath.c_str()), true, "Read empty binary overwrite"));
		RETURN_IF_FALSE(t.Assert(readData.empty(), true, "Empty binary overwrite truncates"));

		// Descriptor ranges honor modes without taking ownership of the caller's descriptor.
		IO::FileGuard file{ rangePath.c_str(), O_RDWR | O_CREAT, 0644 };
		RETURN_IF_FALSE(t.Assert(file.value != -1, true, "Open binary range descriptor"));
		RETURN_IF_FALSE(t.Assert(
			IO::SaveBinaries(values.begin(), values.end(), file.value), true, "Save full binary descriptor range"));
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(values.begin() + 1, values.begin() + 2, file.value), true,
			"Overwrite binary descriptor with shorter range"));
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries<IO::APPEND>(values.begin() + 2, values.begin() + 4, file.value), true,
			"Append binary descriptor range"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, rangePath.c_str()), true, "Read descriptor range modes"));
		RETURN_IF_FALSE(t.Assert(actual == selected, true, "Binary descriptor overwrite and append"));
		RETURN_IF_FALSE(t.Assert(fcntl(file.value, F_GETFD) != -1, true, "Binary range descriptor remains open"));
		actual.clear();
		RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, file.value), true, "Read binary descriptor range"));
		RETURN_IF_FALSE(t.Assert(actual == selected, true, "Binary descriptor range read by descriptor"));
		int32_t first{};
		RETURN_IF_FALSE(t.Assert(IO::ReadBinary(&first, file.value), true, "Read first record by descriptor"));
		RETURN_IF_FALSE(t.Assert(first, selected.front(), "Descriptor is read from the beginning of the file"));
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(values.begin(), values.end(), int32_t{ -1 }), false,
			"Binary range rejects invalid descriptor"));
		RETURN_IF_FALSE(
			t.Assert(IO::ReadBinaries(actual, int32_t{ -1 }), false, "Binary read rejects invalid descriptor"));
		RETURN_IF_FALSE(
			t.Assert(IO::ReadBinary(&first, int32_t{ -1 }), false, "Record read rejects invalid descriptor"));

		// Reading and saving alternate on one descriptor opened in append mode, saving always goes to the end.
		{
			IO::FileGuard appendFile{ rangePath.c_str(), O_RDWR | O_CREAT | O_APPEND | O_TRUNC, 0644 };
			RETURN_IF_FALSE(t.Assert(appendFile.value != -1, true, "Open append descriptor for reading"));
			RETURN_IF_FALSE(t.Assert(IO::SaveBinaries<IO::APPEND>(values.begin(), values.begin() + 2, appendFile.value),
				true, "Save first records by append descriptor"));
			actual.clear();
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(actual, appendFile.value), true, "Read by append descriptor"));
			RETURN_IF_FALSE(
				t.Assert(actual == std::vector<int32_t>{ 11, 22 }, true, "First records read by append descriptor"));
			RETURN_IF_FALSE(t.Assert(IO::SaveBinary<IO::APPEND>(values[2], appendFile.value), true,
				"Save record by append descriptor after reading"));
			actual.clear();
			RETURN_IF_FALSE(
				t.Assert(IO::ReadBinaries(actual, appendFile.value), true, "Read by append descriptor after saving"));
			RETURN_IF_FALSE(t.Assert(actual == std::vector<int32_t>{ 11, 22, 33 }, true,
				"Saving after reading goes to the end of the file"));
		}

		// Descriptor opened only for writing cannot be read.
		{
			IO::FileGuard writeOnly{ rangePath.c_str(), O_WRONLY, 0 };
			RETURN_IF_FALSE(t.Assert(writeOnly.value != -1, true, "Open write-only descriptor"));
			actual.clear();
			RETURN_IF_FALSE(t.Assert(
				IO::ReadBinaries(actual, writeOnly.value), false, "Binary read rejects write-only descriptor"));
			RETURN_IF_FALSE(
				t.Assert(IO::ReadBinary(&first, writeOnly.value), false, "Record read rejects write-only descriptor"));
		}
		RETURN_IF_FALSE(t.Assert(IO::SaveBinaries(values.begin(), values.end(), (path + "missing/range").c_str()),
			false, "Binary range rejects missing parent"));
	}

	{
		// Compare iterator formatting with the unchanged container overload across primitive types and iterators.
		const auto testPrimitiveRange{ [&t, &path](auto& values, const std::string_view name) -> bool {
			using Value = typename std::remove_cvref_t<decltype(values)>::value_type;
			const auto rangePath{ path + std::string{ name } };
			const auto expectedPath{ rangePath + "Expected" };
			const auto begin{ std::next(values.begin()) };
			const auto end{ std::next(values.begin(), 4) };
			const std::vector<Value> selected{ begin, end };
			std::string expected;
			std::string actual;

			RETURN_IF_FALSE(t.Assert(
				IO::SavePrimitives(selected, expectedPath.c_str(), ';'), true, "Save primitive range baseline"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(expected, expectedPath.c_str()), true, "Read primitive baseline"));
			RETURN_IF_FALSE(
				t.Assert(IO::SavePrimitives<IO::OVERWRITE, 0644, 64, 32>(begin, end, rangePath.c_str(), ';'), true,
					"Save primitive subrange with custom buffer"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(actual, rangePath.c_str()), true, "Read primitive subrange"));
			RETURN_IF_FALSE(t.Assert(actual, expected, "Primitive subrange matches container format"));
			RETURN_IF_FALSE(t.Assert(
				IO::SavePrimitives(std::counted_iterator{ begin, 3 }, std::default_sentinel, rangePath.c_str(), ';'),
				true, "Save primitive distinct sentinel"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(actual, rangePath.c_str()), true, "Read primitive sentinel range"));
			RETURN_IF_FALSE(t.Assert(actual, expected, "Primitive sentinel matches container format"));

			IO::FileGuard file{ rangePath.c_str(), O_RDWR | O_CREAT, 0644 };
			RETURN_IF_FALSE(t.Assert(file.value != -1, true, "Open primitive range descriptor"));
			RETURN_IF_FALSE(t.Assert(
				IO::SavePrimitives(begin, end, file.value, ';'), true, "Overwrite primitive descriptor range"));
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives<IO::APPEND>(begin, end, file.value, ';'), true,
				"Append primitive descriptor range"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(actual, rangePath.c_str()), true, "Read primitive descriptor modes"));
			RETURN_IF_FALSE(t.Assert(actual, expected + "\n" + expected, "Primitive descriptor append newline"));
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives<IO::APPEND>(begin, end, rangePath.c_str(), ';'), true,
				"Append primitive path range"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(actual, rangePath.c_str()), true, "Read primitive path append"));
			RETURN_IF_FALSE(
				t.Assert(actual, expected + "\n" + expected + "\n" + expected, "Primitive path append newline"));
			RETURN_IF_FALSE(t.Assert(
				IO::SavePrimitives(begin, end, rangePath.c_str(), ';'), true, "Overwrite shorter primitive range"));
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives(begin, begin, file.value, ';'), true,
				"Empty primitive descriptor overwrite is a no-op"));
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives<IO::APPEND>(begin, begin, rangePath.c_str(), ';'), true,
				"Empty primitive path append is a no-op"));
			RETURN_IF_FALSE(
				t.Assert(IO::ReadStr(actual, rangePath.c_str()), true, "Read after empty primitive ranges"));
			RETURN_IF_FALSE(t.Assert(actual, expected, "Empty primitive ranges preserve bytes"));

			const auto absentPath{ rangePath + "Empty" };
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives(begin, begin, absentPath.c_str(), ';'), true,
				"Empty primitive range succeeds without file creation"));
			RETURN_IF_FALSE(t.Assert(IO::HasPath(absentPath.c_str()), false, "Empty primitive range creates no file"));
			RETURN_IF_FALSE(t.Assert(fcntl(file.value, F_GETFD) != -1, true, "Primitive descriptor remains open"));
			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives(begin, end, int32_t{ -1 }, ';'), false,
				"Primitive range rejects invalid descriptor"));

			return true;
		} };

		std::array<int32_t, 5> integers{ -10, -2, 0, 7, 99 };
		RETURN_IF_FALSE(testPrimitiveRange(integers, "rangeInt"));

		std::list<double> doubles{ -10., -1. / 7., 0., 2. / 3., 99. };
		RETURN_IF_FALSE(testPrimitiveRange(doubles, "rangeDouble"));

		std::forward_list<float> floats{ -10.f, -1.f / 7.f, 0.f, 2.f / 3.f, 99.f };
		RETURN_IF_FALSE(testPrimitiveRange(floats, "rangeFloat"));

		std::array<long double, 5> longDoubles{ -10.L, -1.L / 7.L, 0.L, 2.L / 3.L, 99.L };
		RETURN_IF_FALSE(testPrimitiveRange(longDoubles, "rangeLongDouble"));

		std::vector<bool> bits{ true, false, true, false, true };
		RETURN_IF_FALSE(testPrimitiveRange(bits, "rangeBoolProxy"));

		std::array<char, 5> characters{ 'a', 'b', 'c', 'd', 'e' };
		RETURN_IF_FALSE(testPrimitiveRange(characters, "rangeChar"));
	}

	{
		IO::DirGuard dir{ pathV };
		auto* const dirPtr{ dir.value };
		RETURN_IF_FALSE(t.Assert(dirPtr != nullptr, true, "Open directory"));

		const int fd{ dirfd(dirPtr) };
		RETURN_IF_FALSE(t.Assert(fd != -1, true, "Get directory fd"));

		IO::DirGuard dir1{ std::move(dir) };
		RETURN_IF_FALSE(t.Assert(dir.value, nullptr, "Moved directory should be null"));
		RETURN_IF_FALSE(t.Assert(dir1.value, dirPtr, "Moved directory should be valid"));

		IO::DirGuard dir2;
		dir2 = std::move(dir1);

		RETURN_IF_FALSE(t.Assert(dir1.value, nullptr, "Moved directory should be null"));
		RETURN_IF_FALSE(t.Assert(dir2.value, dirPtr, "Moved directory should be valid"));

		dir2.Clear();
		RETURN_IF_FALSE(t.Assert(dir2.value, nullptr, "Directory should be closed after Clear()"));

		RETURN_IF_FALSE(t.Assert(
			fcntl(fd, F_GETFD) == -1 && errno == EBADF, true, "Directory fd should be closed in OS after Clear()"));
	}

	RETURN_IF_FALSE(t.Assert(IO::EnumToString(static_cast<IO::FileType>(U(IO::FileType::Sock) + 1)), "Unknown",
		"EnumToString(unknown FileType) should return 'Unknown'"));

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_IO_INL