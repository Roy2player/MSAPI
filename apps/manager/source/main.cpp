/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2024-02-19
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

#include "manager.h"
#include <iostream>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>

MSAPI_APPLICATION_SIGNAL_HANDLER

void CheckVforkedApps([[maybe_unused]] const int signal) { static_cast<Manager*>(app)->CheckVforkedApps(); }

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	MSAPI_MLOCKALL_CURRENT_FUTURE
	MSAPI_APPLICATION_SIGNAL_ACTION
	signal(SIGCHLD, CheckVforkedApps);

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		return 1;
	}
	path += "../";

	MSAPI::logger.SetParentPath(path);
	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("MSAPI Manager");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	Manager manager;
	app = &manager;
	manager.SetName("MSAPI Manager");
	manager.HandleModifyRequest({ { 1001, path + "web/" }, { 1000003, size_t{ 99999 } } });
	manager.HandleRunRequest();
	manager.Start(INADDR_ANY, 1134);

	return 0;
}