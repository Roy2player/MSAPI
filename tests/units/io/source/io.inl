/**************************
 * @file        io.inl
 * @version     6.0
 * @date        2025-12-13
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#ifndef MSAPI_UNIT_TEST_IO_INL
#define MSAPI_UNIT_TEST_IO_INL

#include "../../../../library/source/help/io.inl"
#include "../../../../library/source/test/test.h"
#include <cmath>

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for IO.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Io();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Io()
{
	LOG_INFO_UNITTEST("MSAPI IO");
	MSAPI::Test t;

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
			t.Assert(IO::SaveStr<IO::append>("2 Some test data is here", path1V), true, "Overwrite str to file"));
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
		RETURN_IF_FALSE(t.Assert(IO::RenameFile(path2V, pathRenamedV), true, "Rename file"));
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

		std::vector<std::string> names;
		RETURN_IF_FALSE(
			t.Assert(IO::List<IO::FileType::Regular>(names, pathChild3V), true, "List files in nested dir"));
		RETURN_IF_FALSE(t.Assert(names.size(), 1, "There should be one file in nested dir"));
		RETURN_IF_FALSE(t.Assert(names[0], "someCopiedFile", "File name should be correct"));

		names.clear();
		RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Regular>(names, pathV), true, "List files in test dir"));
		RETURN_IF_FALSE(t.Assert(names.size(), 2, "There should be two files in test dir"));
		RETURN_IF_FALSE(t.Assert(names[0] == "someRenamedFile" || names[0] == "someNameForFileToTest1", true,
			"First file name should be correct"));
		RETURN_IF_FALSE(t.Assert(names[1] == "someRenamedFile" || names[1] == "someNameForFileToTest1", true,
			"Second file name should be correct"));
		RETURN_IF_FALSE(t.Assert(names[0] != names[1], true, "File names should be different"));

		names.clear();
		RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Directory>(names, pathV), true, "List dirs in test dir"));
		RETURN_IF_FALSE(t.Assert(names.size(), 1, "There should be one dir in test dir"));
		RETURN_IF_FALSE(t.Assert(names[0], "childDir", "Dir name should be correct"));

		names.clear();
		const auto& pathNonExistingDir{ path + "nonExistingDir" };
		const std::string_view pathNonExistingDirV{ pathNonExistingDir };
		RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Regular>(names, pathNonExistingDirV), false,
			"Listing files in non existing dir should fail"));
		RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no files in non existing dir"));

		names.clear();
		RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Directory>(names, pathNonExistingDirV), false,
			"Listing dirs in non existing dir should fail"));
		RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no dirs in non existing dir"));

		names.clear();
		RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Regular>(names, (path + "childDir/childDir2").c_str()), true,
			"Listing files in nested dir level 2 should succeed"));
		RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no files in nested dir level 2"));

		names.clear();
		RETURN_IF_FALSE(t.Assert(IO::List<IO::FileType::Directory>(names, pathChild3V), true,
			"Listing dirs in nested dir level 3 should succeed"));
		RETURN_IF_FALSE(t.Assert(names.size(), 0, "There should be no dirs in nested dir level 3"));

		RETURN_IF_FALSE(t.Assert(IO::Remove(path1V), true, "Remove file"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(path1V), false, "File should not exist now"));
		const auto& pathChild1{ path + "childDir" };
		const std::string_view pathChild1V{ pathChild1 };
		RETURN_IF_FALSE(t.Assert(IO::Remove(pathChild1V), true, "Remove nested dir"));
		RETURN_IF_FALSE(t.Assert(IO::HasPath(pathChild1V), false, "Nested dir should not exist now"));
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
			RETURN_IF_FALSE(t.Assert(IO::SaveStr<IO::append>(sectionSeparator, pathPrimitivesV), true,
				"Overwrite primitives file with some other data"));
			std::format_to(std::back_inserter(testData), "\n{}", sectionSeparator);

			const auto& pathPrimitivesCopy{ path + "primitivesCopy" };
			const std::string_view pathPrimitivesCopyV{ pathPrimitivesCopy };
			RETURN_IF_FALSE(t.Assert(IO::CopyFile(pathPrimitivesV, pathPrimitivesCopyV), true, "Copy primitives file"));
			RETURN_IF_FALSE(
				t.Assert(IO::ReadStr(readData, pathPrimitivesCopyV), true, "Read str from copied primitives file"));
			RETURN_IF_FALSE(
				t.Assert(readData, testData, "Read data from copied primitives file should be equal to saved data"));

			RETURN_IF_FALSE(t.Assert(IO::SaveStr<IO::append>(readData, pathPrimitivesV), true,
				"Append copied primitives data to original primitives file"));
			std::format_to(std::back_inserter(testData), "\n{}", readData);
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read str from primitives file"));
			RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

			RETURN_IF_FALSE(t.Assert(IO::SavePrimitives<IO::append>(data, pathPrimitivesV, ','), true,
				"Append primitives to primitives file"));
			std::format_to(std::back_inserter(testData), "\n{}", testDataCopy);
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read str from primitives file"));
			RETURN_IF_FALSE(t.Assert(readData, testData, "Read data should be equal to saved data"));

			RETURN_IF_FALSE(
				t.Assert(IO::SavePrimitives(data, pathPrimitivesV, ','), true, "Overwrite primitives file"));
			RETURN_IF_FALSE(t.Assert(IO::ReadStr(readData, pathPrimitivesV), true, "Read str from primitives file"));
			RETURN_IF_FALSE(t.Assert(readData, testDataCopy, "Read data should be equal to saved data"));

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
			TestStruct o3{};
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinary<IO::append>(&o3, o3PathOrFd), true, "Save binary struct in append mode"));
			RETURN_IF_FALSE(t.Assert(IO::ReadBinary(&o2, o3Path), true, "Read binary struct from append file"));
			RETURN_IF_FALSE(t.Assert(o2, o3, "Read struct from append file should be equal to saved struct"));
			vec.push_back(o3);
			vec.push_back(o3);
			RETURN_IF_FALSE(
				t.Assert(IO::SaveBinary<IO::append>(&o3, o3PathOrFd), true, "Save binary struct in append mode"));
			RETURN_IF_FALSE(t.Assert(IO::ReadBinaries(vecRead, o3Path), true, "Read binaries from append file"));
			RETURN_IF_FALSE(t.Assert(vecRead, vec, "Read structs from append file should be equal to saved structs"));
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
		IO::FileDescriptor::ExitGuard fd1;
		RETURN_IF_FALSE(t.Assert(fd1.value, -1, "Open empty file descriptor for o1Fd"));
		fd1 = IO::FileDescriptor::ExitGuard{ pathFd1V, O_RDWR | O_CREAT, 0644 };
		RETURN_IF_FALSE(t.Assert(fd1.value != -1, true, "Open initialized file descriptor for o1Fd"));
		IO::FileDescriptor::ExitGuard fd3{ pathFd3V, O_RDWR | O_CREAT, 0644 };
		RETURN_IF_FALSE(t.Assert(fd3.value != -1, true, "Open file descriptor for o3Fd"));
		IO::FileDescriptor::ExitGuard fdVec{ pathVecFdV, O_RDWR | O_CREAT, 0644 };
		IO::FileDescriptor::ExitGuard fdVec2{ std::move(fdVec) };
		RETURN_IF_FALSE(t.Assert(fdVec.value, -1, "Open file descriptor for vecFd"));
		RETURN_IF_FALSE(t.Assert(fdVec2.value != -1, true, "Open file descriptor for fdVec2"));

		RETURN_IF_FALSE(t.Assert(testBinary(fd1.value, pathFd1V, fd3.value, pathFd3V, fdVec2.value, pathVecFdV), true,
			"Test binary with file descriptors"));
	}

	RETURN_IF_FALSE(t.Assert(IO::EnumToString(static_cast<IO::FileType>(U(IO::FileType::Sock) + 1)), "Unknown",
		"EnumToString(unknown FileType) should return 'Unknown'"));

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_IO_INL