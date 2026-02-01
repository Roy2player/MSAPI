/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2025-11-20
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

#include "../../../../library/source/help/io.inl"
#include "application.inl"

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
	if (MSAPI::IO::List<MSAPI::IO::FileType::Regular>(files, path.c_str())) {
		for (const auto& file : files) {
			(void)MSAPI::IO::Remove((path + file).c_str());
		}
	}

	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("UTApplication");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	return static_cast<int>(!MSAPI::UnitTest::Application());
}