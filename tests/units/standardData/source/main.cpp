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

#include "../../../../library/source/help/bin.h"
#include "standardData.inl"

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

	//* Clear old files
	std::vector<std::string> files;
	if (MSAPI::Bin::List<MSAPI::Bin::FileType::Regular>(files, path)) {
		for (const auto& file : files) {
			MSAPI::Bin::Remove(path + file);
		}
	}

	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("UTStandardData");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	return static_cast<int>(!MSAPI::UnitTest::StandardData());
}