/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2025-10-23
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
#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/test.h"
#include <memory>
#include <sys/mman.h>
#include <sys/resource.h>

struct ServerImpl : MSAPI::Server {
	void HandleBuffer([[maybe_unused]] MSAPI::RecvBufferInfo* recvBufferInfo) override { }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	MSAPI_MLOCKALL_CURRENT_FUTURE

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
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
	MSAPI::logger.SetName("TestServer");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	MSAPI::Test test;

	ServerImpl serverImpl;
	test.Assert(serverImpl.GetState(), MSAPI::Server::State::Initialization, "Server state is Initialization");
	serverImpl.Stop();
	test.Assert(serverImpl.GetState(), MSAPI::Server::State::Stopped, "Server state is Stopped");
	MSAPI::Timer timer;
	serverImpl.Start(INADDR_ANY, 1134);
	test.Assert(MSAPI::Timer{} - timer < MSAPI::Timer::Duration::CreateMilliseconds(1), true,
		"Server cannot start not in initialization state");

	return test.Passed<int32_t>();
}