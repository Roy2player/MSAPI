/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2023-09-24
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
#include "../../../library/source/help/html.h"
#include "../../../library/source/help/json.h"
#include "../../../library/source/help/table.h"
#include "../../../library/source/protocol/object.h"
#include "../../../library/source/protocol/standard.h"
#include "../../../library/source/server/application.h"
#include "../../../library/source/test/test.h"

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
	if (!MSAPI::Json::UNITTEST() || !MSAPI::HTML::UNITTEST() || !MSAPI::Timer::UNITTEST()
		|| !MSAPI::ObjectProtocol::Data::UNITTEST() || !MSAPI::Helper::UNITTEST() || !MSAPI::Application::UNITTEST()
		|| !MSAPI::DataHeader::UNITTEST() || !MSAPI::StandardProtocol::Data::UNITTEST()
		|| !MSAPI::TableData::UNITTEST()) {

		return 1;
	};

	return 0;
}