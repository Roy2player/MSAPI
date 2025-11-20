/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2025-11-20
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

#include "../../../library/source/help/bin.h"
#include "../../../library/source/test/test.h"

// Include all unit test .inl files
#include "../dataHeader/source/dataHeader.inl"
#include "../application/source/application.inl"
#include "../objectData/source/objectData.inl"
#include "../standardData/source/standardData.inl"
#include "../htmlHelper/source/htmlHelper.inl"
#include "../json/source/json.inl"
#include "../table/source/table.inl"
#include "../helper/source/helper.inl"
#include "../timer/source/timer.inl"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		std::cerr << "Cannot get executable path" << std::endl;
		return 1;
	}
	path += "../";
	MSAPI::logger.SetParentPath(path);

	path += "logs/";
	{

		auto logs{ MSAPI::Bin::ListFiles<MSAPI::Bin::FileType::Regular, std::vector<std::string>>(path) };
		for (const auto& file : logs) {
			MSAPI::Bin::Remove(path + file);
		}
	}

	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("TestUnits");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	LOG_INFO("Unit tests");
	if (!MSAPI::Test::TestJson() || !MSAPI::Test::TestHTML() || !MSAPI::Test::TestTimer()
		|| !MSAPI::Test::TestObjectData() || !MSAPI::Test::TestHelper() || !MSAPI::Test::TestApplication()
		|| !MSAPI::Test::TestDataHeader() || !MSAPI::Test::TestStandardData()
		|| !MSAPI::Test::TestTableData()) {

		return 1;
	};

	return 0;
}