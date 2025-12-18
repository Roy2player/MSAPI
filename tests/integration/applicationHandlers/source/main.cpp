/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2024-04-10
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
 *
 * @brief Test scenario is checking collecting, encoding and decoding standard protocol logic, especially
 * application handlers: HandleRunRequest, HandlePauseRequest, HandleModifyRequest, HandleHello, HandleMetadata,
 * HandleParameters. Checks that application signals are handled respectively to the manager application, outcome and
 * income connections, for paused and running application states.
 *
 * 1) Pseudo managers exchange actions;
 * 2) Pseudo manager sends metadata, parameters, run, pause and delete requests to outcome second pseudo manager
 * connection, all requests are handled as buffers;
 * 3) Wait for replays from the manager and pseudo manager on Hello message;
 * 4) Check manager default parameters;
 * 5) Client in paused state right after creation;
 * 6) Check that client sended hello message to the manager;
 * 7) Check that client sended hello message to the pseudo manager;
 * 8) Manager asks metadata from the client;
 * 9) Manager asks parameters from the client;
 * 10) Manager sends run request to the client, parameters are not valid, state is not changed;
 * 11) Pseudo manager sends metadata and parameters requests to the client, no reaction;
 * 12) Manager sends pause request to the client, state is not changed;
 * 13) Pseudo manager sends run request to the client, no reaction;
 * 14) Manager sends modify request for some of invalid parameters to the client, parameters are changed, state is not
 * changed;
 * 15) Pseudo manager sends data to the client, no reaction;
 * 16) Pseudo manager sends data to the second pseudo manager, handled as buffer;
 * 17) Pseudo manager sends metadata response to the second pseudo manager, handled as metadata response;
 * 18) Pseudo manager sends parameters response to the second pseudo manager, handled as parameters response;
 * 19) Second pseudo manager sends action hello to client connection, handled as hello;
 * 20) Manager sends run request to the client, parameters are not valid, state is not changed;
 * 21) Manager sends modify request with valid values for all parameters to the client, parameters are changed;
 * 22) Manager sends run request to the client, state is changed;
 * 23) Pseudo manager sends pause, metadata and parameters requests to the client, no reaction;
 * 24) Manager asks parameters from the client;
 * 25) Manager asks metadata from the client;
 * 26) Manager sends modify for some parameters to the client and make them invalid, parameters are changed, state is
 * paused;
 * 27) Manager sends run request to the client, state is not changed;
 * 28) Manager sends modify with valid values for all parameters to the client, parameters are changed, state is not
 * changed;
 * 29) Manager sends run request to the client, state is changed;
 * 30) Manager sends pause request to the client, state is changed;
 * 31) Pseudo manager sends delete request to the client, no reaction;
 * 32) Manager send modify request for some parameters to the client, parameters are changed, state is not changed;
 * 33) Manager sends pause request to the client, state is not changed;
 * 34) Manager asks metadata from the client;
 * 35) Manager asks parameters from the client;
 * 36) Manager sends run request to the client, state is changed;
 * 37) Manager sends run request to the client, state is not changed;
 * 38) Check parameters on client side;
 * 39) Manager asks parameters from the client;
 * 40) Manager asks metadata from the client;
 * 41) Manager stops, HandleDisconnect is called, client is paused;
 * 42) Manager starts, HandleReconnect is called, client is running;
 * 43) Manager sends delete request to the client, state is changed;
 * 44) Check actions and unhandled actions numbers for all applications.
 */

#include "../../../../library/source/help/io.inl"
#include "../../../../library/source/test/daemon.hpp"
#include "../../../../library/source/test/test.h"
#include "client.h"
#include "manager.h"
#include <memory>
#include <sys/mman.h>
#include <sys/resource.h>

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
	MSAPI::logger.SetName("TestAH");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	//* Manager
	auto managerPtr{ MSAPI::Daemon<Manager>::Create("Manager") };
	if (managerPtr == nullptr) {
		return 1;
	}
	auto manager{ static_cast<Manager*>(managerPtr->GetApp()) };

	//* Pseudo manager
	auto pseudoManagerPtr{ MSAPI::Daemon<Manager>::Create("PseudoManager") };
	if (pseudoManagerPtr == nullptr) {
		return 1;
	}
	auto pseudoManager{ static_cast<Manager*>(pseudoManagerPtr->GetApp()) };

	//* Second pseudo manager
	auto secondPseudoManagerPtr{ MSAPI::Daemon<Manager>::Create("SecondPseudoManager") };
	if (secondPseudoManagerPtr == nullptr) {
		return 1;
	}
	auto secondPseudoManager{ static_cast<Manager*>(secondPseudoManagerPtr->GetApp()) };
	if (!pseudoManager->OpenConnect(1, INADDR_LOOPBACK, secondPseudoManagerPtr->GetPort())) {
		return 1;
	}

	MSAPI::Test test;

	//* 1) Pseudo managers exchange actions
	secondPseudoManager->WaitActionsNumber(test, 50000, 1);
	test.Assert(
		secondPseudoManager->GetActionsNumber(), 1, "Correct number of actions on second pseudo manager side: 1");
	pseudoManager->WaitActionsNumber(test, 50000, 1);
	test.Assert(pseudoManager->GetActionsNumber(), 1, "Correct number of actions on pseudo manager side: 1");

	//* 2) Pseudo manager sends metadata, parameters, run, pause and delete requests to outcome second pseudo manager
	//* connection, all requests are handled as buffers
	pseudoManager->UseOutcomeConnection();
	pseudoManager->SendMetadataRequest();
	pseudoManager->SendParametersRequest();
	pseudoManager->SendActionRun();
	pseudoManager->SendActionPause();
	pseudoManager->SendActionDelete();
	test.Wait(50000, [&secondPseudoManager]() { return secondPseudoManager->GetUnhandledActions() == 5; });
	test.Assert(
		secondPseudoManager->GetUnhandledActions(), 5, "Correct number of unhandled actions on pseudo manager side: 5");

	//* Client
	auto clientPtr{ MSAPI::Daemon<Client>::Create("Client") };
	if (clientPtr == nullptr) {
		return 1;
	}
	auto client{ static_cast<Client*>(clientPtr->GetApp()) };
	if (!client->OpenConnect(0, INADDR_LOOPBACK, managerPtr->GetPort())
		|| !client->OpenConnect(1, INADDR_LOOPBACK, pseudoManagerPtr->GetPort())) {

		return 1;
	}

	//* 3) Wait for replays from the manager and pseudo manager on Hello message
	client->WaitUnhandledActions(test, 50000, 2);
	test.Assert(client->GetUnhandledActions(), 2, "Correct number of unhandled actions on client side: 2");

	//* 4) Check manager default parameters
	test.Assert(manager->Manager::GetParameters(),
		"Parameters:\n{\n\tSeconds between try to connect(1000001) : 1\n\tLimit of attempts to connection(1000002) : "
		"1000\n\tLimit of connections from one IP(1000003) : 5\n\tRecv buffer size(1000004) : 1024\n\tRecv buffer "
		"size limit(1000005) : 10485760\n\tServer state(1000006) const : Running\n\tMax connections(1000007) const "
		": 4096\n\tListening IP(1000008) const : 127.0.0.1\n\tListening port(1000009) const : "
			+ _S(managerPtr->GetPort())
			+ "\n\tName(2000001) const : Manager\n\tApplication state(2000002) const : Paused\n}",
		"Server default parameters");

	//* 5) Client in paused state right after creation
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client in paused state right after creation");

	//* 6) Check that client sended hello message to the manager
	manager->WaitActionsNumber(test, 50000, 1);
	const auto& actionsManager{ manager->GetActionsNumber() };
	test.Assert(actionsManager, 1, "Correct number of actions on manager side: 1");
	manager->UseClientConnection();

	//* 7) Check that client sended hello message to the pseudo manager
	pseudoManager->WaitActionsNumber(test, 50000, 2);
	const auto& actionsPseudoManager{ pseudoManager->GetActionsNumber() };
	test.Assert(actionsPseudoManager, 2, "Correct number of actions on pseudo manager side: 2");

	const std::string metadata{
		"{\"mutable\":{\"1\":{\"name\":\"Parameter 1 - int8_t\",\"type\":\"Int8\"},\"2\":{\"name\":\"Parameter 2 - "
		"int16_t\",\"type\":\"Int16\"},\"3\":{\"name\":\"Parameter 3 - "
		"int32_t\",\"type\":\"Int32\"},\"4\":{\"name\":\"Parameter 4 - "
		"int64_t\",\"type\":\"Int64\"},\"5\":{\"name\":\"Parameter 5 - "
		"uint8_t\",\"type\":\"Uint8\"},\"6\":{\"name\":\"Parameter 6 - "
		"uint16_t\",\"type\":\"Uint16\"},\"7\":{\"name\":\"Parameter 7 - "
		"uint32_t\",\"type\":\"Uint32\"},\"8\":{\"name\":\"Parameter 8 - "
		"uint64_t\",\"type\":\"Uint64\"},\"9\":{\"name\":\"Parameter 9 - "
		"float\",\"type\":\"Float\"},\"10\":{\"name\":\"Parameter 10 - "
		"double\",\"type\":\"Double\"},\"11\":{\"name\":\"Parameter 11 - "
		"double\",\"type\":\"Double\"},\"12\":{\"name\":\"Parameter 12 - "
		"optional<int8_t>\",\"type\":\"OptionalInt8\",\"canBeEmpty\":false},\"13\":{\"name\":\"Parameter 13 - "
		"optional<int8_t>\",\"type\":\"OptionalInt8\",\"canBeEmpty\":true},\"14\":{\"name\":\"Parameter 14 - "
		"optional<int16_t>\",\"type\":\"OptionalInt16\",\"canBeEmpty\":false},\"15\":{\"name\":\"Parameter 15 - "
		"optional<int16_t>\",\"type\":\"OptionalInt16\",\"canBeEmpty\":true},\"16\":{\"name\":\"Parameter 16 - "
		"optional<int32_t>\",\"type\":\"OptionalInt32\",\"canBeEmpty\":false},\"17\":{\"name\":\"Parameter 17 - "
		"optional<int32_t>\",\"type\":\"OptionalInt32\",\"canBeEmpty\":true},\"18\":{\"name\":\"Parameter 18 - "
		"optional<int64_t>\",\"type\":\"OptionalInt64\",\"canBeEmpty\":false},\"19\":{\"name\":\"Parameter 19 - "
		"optional<int64_t>\",\"type\":\"OptionalInt64\",\"canBeEmpty\":true},\"20\":{\"name\":\"Parameter 20 - "
		"optional<uint8_t>\",\"type\":\"OptionalUint8\",\"canBeEmpty\":false},\"21\":{\"name\":\"Parameter 21 - "
		"optional<uint8_t>\",\"type\":\"OptionalUint8\",\"canBeEmpty\":true},\"22\":{\"name\":\"Parameter 22 - "
		"optional<uint16_t>\",\"type\":\"OptionalUint16\",\"canBeEmpty\":false},\"23\":{\"name\":\"Parameter 23 - "
		"optional<uint16_t>\",\"type\":\"OptionalUint16\",\"canBeEmpty\":true},\"24\":{\"name\":\"Parameter 24 - "
		"optional<uint32_t>\",\"type\":\"OptionalUint32\",\"canBeEmpty\":false},\"25\":{\"name\":\"Parameter 25 - "
		"optional<uint32_t>\",\"type\":\"OptionalUint32\",\"canBeEmpty\":true},\"26\":{\"name\":\"Parameter 26 - "
		"optional<uint64_t>\",\"type\":\"OptionalUint64\",\"canBeEmpty\":false},\"27\":{\"name\":\"Parameter 27 - "
		"optional<uint64_t>\",\"type\":\"OptionalUint64\",\"min\":300,\"max\":6000,\"canBeEmpty\":false},\"28\":{"
		"\"name\":\"Parameter 28 - "
		"optional<float>\",\"type\":\"OptionalFloat\",\"canBeEmpty\":false},\"29\":{\"name\":\"Parameter 29 - "
		"optional<float>\",\"type\":\"OptionalFloat\",\"min\":-400.001007080,\"max\":400.001007080,\"canBeEmpty\":"
		"false},\"30\":{\"name\":\"Parameter 30 - "
		"optional<double>\",\"type\":\"OptionalDouble\",\"canBeEmpty\":false},\"31\":{\"name\":\"Parameter 31 - "
		"optional<double>\",\"type\":\"OptionalDouble\",\"canBeEmpty\":true},\"32\":{\"name\":\"Parameter 32 - "
		"optional<double>\",\"type\":\"OptionalDouble\",\"canBeEmpty\":false},\"33\":{\"name\":\"Parameter 33 - "
		"optional<double>\",\"type\":\"OptionalDouble\",\"canBeEmpty\":true},\"34\":{\"name\":\"Parameter 34 - "
		"string\",\"type\":\"String\",\"canBeEmpty\":true},\"35\":{\"name\":\"Parameter 35 - "
		"string\",\"type\":\"String\",\"canBeEmpty\":false},\"36\":{\"name\":\"Parameter 36 - "
		"Timer\",\"type\":\"Timer\",\"canBeEmpty\":true},\"37\":{\"name\":\"Parameter 37 - "
		"Timer\",\"type\":\"Timer\",\"canBeEmpty\":false},\"38\":{\"name\":\"Parameter 38 - "
		"Timer::Duration\",\"type\":\"Duration\",\"canBeEmpty\":true,\"durationType\":\"Seconds\"},\"39\":{\"name\":"
		"\"Parameter 39 - "
		"Timer::Duration\",\"type\":\"Duration\",\"max\":60000000000,\"canBeEmpty\":false,\"durationType\":\"Seconds\"}"
		",\"40\":{\"name\":\"Parameter 40 - bool\",\"type\":\"Bool\"},\"41\":{\"name\":\"Parameter 41 - "
		"Table\",\"type\":\"TableData\",\"canBeEmpty\":true,\"columns\":{\"411\":{\"type\":\"Bool\"},\"412\":{\"type\":"
		"\"Bool\"},\"413\":{\"type\":\"String\"},\"414\":{\"type\":\"String\"},\"415\":{\"type\":\"OptionalDouble\"}}},"
		"\"42\":{\"name\":\"Parameter 42 - "
		"Table\",\"type\":\"TableData\",\"canBeEmpty\":false,\"columns\":{\"4121\":{\"type\":\"Uint64\"},\"422\":{"
		"\"type\":\"Uint64\"}}},\"43\":{\"name\":\"Parameter 43 - "
		"Table\",\"type\":\"TableData\",\"canBeEmpty\":true,\"columns\":{\"11111\":{\"type\":\"String\"},\"22222\":{"
		"\"type\":\"Timer\"},\"33333\":{\"type\":\"Duration\"},\"44444\":{\"type\":\"Duration\"},\"55555\":{\"type\":"
		"\"Duration\"},\"66666\":{\"type\":\"Int8\"},\"77777\":{\"type\":\"Int16\"},\"88888\":{\"type\":\"Int32\"},"
		"\"99999\":{\"type\":\"Int64\"},\"1010101010\":{\"type\":\"Uint8\"},\"1111111111\":{\"type\":\"Uint16\"},"
		"\"1212121212\":{\"type\":\"Uint32\"},\"1313131313\":{\"type\":\"Uint64\"},\"1414141414\":{\"type\":\"Double\"}"
		",\"1515151515\":{\"type\":\"Float\"},\"1616161616\":{\"type\":\"Bool\"},\"1717171717\":{\"type\":"
		"\"OptionalInt8\"},\"1818181818\":{\"type\":\"OptionalInt16\"},\"1919191919\":{\"type\":\"OptionalInt32\"},"
		"\"2020202020\":{\"type\":\"OptionalInt64\"},\"2121212121\":{\"type\":\"OptionalUint8\"},\"2222222222\":{"
		"\"type\":\"OptionalUint16\"},\"2323232323\":{\"type\":\"OptionalUint32\"},\"2424242424\":{\"type\":"
		"\"OptionalUint64\"},\"2525252525\":{\"type\":\"OptionalDouble\"},\"2626262626\":{\"type\":\"OptionalFloat\"},"
		"\"2727272727\":{\"type\":\"String\"},\"2828282828\":{\"type\":\"Timer\"},\"2929292929\":{\"type\":"
		"\"Duration\"},\"3030303030\":{\"type\":\"Bool\"},\"3131313131\":{\"type\":\"String\"},\"3232323232\":{"
		"\"type\":\"OptionalDouble\"},\"3333333333\":{\"type\":\"OptionalDouble\"}}},\"44\":{\"name\":\"Parameter 44 - "
		"Table\",\"type\":\"TableData\",\"canBeEmpty\":false,\"columns\":{\"1\":{\"type\":\"Int32\"}}},\"1000001\":{"
		"\"name\":\"Seconds between try to connect\",\"type\":\"Uint32\",\"min\":1},\"1000002\":{\"name\":\"Limit of "
		"attempts to connection\",\"type\":\"Uint64\",\"min\":1},\"1000003\":{\"name\":\"Limit of connections from one "
		"IP\",\"type\":\"Uint64\",\"min\":1},\"1000004\":{\"name\":\"Recv buffer "
		"size\",\"type\":\"Uint64\",\"min\":3},\"1000005\":{\"name\":\"Recv buffer size "
		"limit\",\"type\":\"Uint64\",\"min\":1024}},\"const\":{\"1000006\":{\"name\":\"Server "
		"state\",\"type\":\"Int16\",\"stringInterpretations\":{\"0\":\"Undefined\",\"1\":\"Initialization\",\"2\":"
		"\"Running\",\"3\":\"Stopped\"}},\"1000007\":{\"name\":\"Max "
		"connections\",\"type\":\"Int32\"},\"1000008\":{\"name\":\"Listening "
		"IP\",\"type\":\"String\"},\"1000009\":{\"name\":\"Listening "
		"port\",\"type\":\"Uint16\"},\"2000001\":{\"name\":\"Name\",\"type\":\"String\"},\"2000002\":{\"name\":"
		"\"Application "
		"state\",\"type\":\"Int16\",\"stringInterpretations\":{\"0\":\"Undefined\",\"1\":\"Paused\",\"2\":\"Running\"}}"
		"}}"
	};

	//* 8) Manager asks metadata from the client
	manager->SendMetadataRequest();
	manager->WaitActionsNumber(test, 50000, 2);
	test.Assert(actionsManager, 2, "Correct number of actions on manager side: 2");
	test.Assert(manager->GetMetadata(), metadata, "Metadata is correct");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client state is not changed after metadata request");

	const auto checkParametersResponse{
		[&manager, &test](const int8_t s1, const int16_t s2, const int32_t s3, const int64_t s4, const uint8_t s5,
			const uint16_t s6, const uint32_t s7, const uint64_t s8, const float s9, const double s10, const double s11,
			const std::optional<int8_t> s12, const std::optional<int8_t> s13, const std::optional<int16_t> s14,
			const std::optional<int16_t> s15, const std::optional<int32_t> s16, const std::optional<int32_t> s17,
			const std::optional<int64_t> s18, const std::optional<int64_t> s19, const std::optional<uint8_t> s20,
			const std::optional<uint8_t> s21, const std::optional<uint16_t> s22, const std::optional<uint16_t> s23,
			const std::optional<uint32_t> s24, const std::optional<uint32_t> s25, const std::optional<uint64_t> s26,
			const std::optional<uint64_t> s27, const std::optional<float> s28, const std::optional<float> s29,
			const std::optional<double> s30, const std::optional<double> s31, const std::optional<double> s32,
			const std::optional<double> s33, const std::string s34, const std::string s35, const MSAPI::Timer s36,
			const MSAPI::Timer s37, const MSAPI::Timer::Duration s38, const MSAPI::Timer::Duration s39, const bool s40,
			const MSAPI::Table<bool, bool, std::string, std::string, std::optional<double>>& s41,
			const MSAPI::Table<uint64_t, uint64_t>& s42,
			const MSAPI::Table<std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::Timer::Duration,
				MSAPI::Timer::Duration, standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, bool,
				std::string, std::optional<double>, std::optional<double>>& s43,
			const MSAPI::Table<int32_t>& s44) {
			const auto& parametersResponse{ manager->GetParametersResponse() };

			const auto check{ [&parametersResponse, &test]<size_t I, typename T>(const T& expected) {
				auto it{ parametersResponse.find(I) };
				if (it == parametersResponse.end()) {
					LOG_ERROR("Parameter " + _S(I) + " is not found");
					return;
				}

				if constexpr (std::is_base_of_v<MSAPI::TableBase, T>) {
					if (!std::holds_alternative<MSAPI::TableData>(it->second)) {
						LOG_ERROR("Parameter " + _S(I) + " is not right type");
						throw std::bad_variant_access();
					}

					const auto* columns{ expected.GetColumns() };
					if (columns == nullptr) {
						LOG_ERROR("Columns are not set for expected table");
						return;
					}
					std::vector<size_t> ids;
					ids.reserve(columns->size());
					for (const auto& column : *columns) {
						ids.push_back(column.id);
					}

					T table{ std::move(ids) };
					table.Copy(std::get<MSAPI::TableData>(it->second));
					test.Assert(table, expected, "Parameter " + _S(I) + " inside parameters response is correct");
				}
				else {
					if (!std::holds_alternative<T>(it->second)) {
						LOG_ERROR("Parameter " + _S(I) + " is not right type");
						throw std::bad_variant_access();
					}
					test.Assert(std::get<T>(it->second), expected,
						"Parameter " + _S(I) + " inside parameters response is correct");
				}
			} };

			check.operator()<1>(s1);
			check.operator()<2>(s2);
			check.operator()<3>(s3);
			check.operator()<4>(s4);
			check.operator()<5>(s5);
			check.operator()<6>(s6);
			check.operator()<7>(s7);
			check.operator()<8>(s8);
			check.operator()<9>(s9);
			check.operator()<10>(s10);
			check.operator()<11>(s11);
			check.operator()<12>(s12);
			check.operator()<13>(s13);
			check.operator()<14>(s14);
			check.operator()<15>(s15);
			check.operator()<16>(s16);
			check.operator()<17>(s17);
			check.operator()<18>(s18);
			check.operator()<19>(s19);
			check.operator()<20>(s20);
			check.operator()<21>(s21);
			check.operator()<22>(s22);
			check.operator()<23>(s23);
			check.operator()<24>(s24);
			check.operator()<25>(s25);
			check.operator()<26>(s26);
			check.operator()<27>(s27);
			check.operator()<28>(s28);
			check.operator()<29>(s29);
			check.operator()<30>(s30);
			check.operator()<31>(s31);
			check.operator()<32>(s32);
			check.operator()<33>(s33);
			check.operator()<34>(s34);
			check.operator()<35>(s35);
			check.operator()<36>(s36);
			check.operator()<37>(s37);
			check.operator()<38>(s38);
			check.operator()<39>(s39);
			check.operator()<40>(s40);
			check.operator()<41>(s41);
			check.operator()<42>(s42);
			check.operator()<43>(s43);
			check.operator()<44>(s44);

			//* Check default Server and Application parameters
			test.Assert(
				parametersResponse.find(1000001) != parametersResponse.end(), true, "Parameter 1000001 is in response");
			test.Assert(
				parametersResponse.find(1000002) != parametersResponse.end(), true, "Parameter 1000002 is in response");
			test.Assert(
				parametersResponse.find(1000003) != parametersResponse.end(), true, "Parameter 1000003 is in response");
			test.Assert(
				parametersResponse.find(1000004) != parametersResponse.end(), true, "Parameter 1000004 is in response");
			test.Assert(
				parametersResponse.find(1000005) != parametersResponse.end(), true, "Parameter 1000005 is in response");
			test.Assert(
				parametersResponse.find(1000006) != parametersResponse.end(), true, "Parameter 1000006 is in response");
			test.Assert(
				parametersResponse.find(1000007) != parametersResponse.end(), true, "Parameter 1000007 is in response");
			test.Assert(
				parametersResponse.find(1000008) != parametersResponse.end(), true, "Parameter 1000008 is in response");
			test.Assert(
				parametersResponse.find(1000009) != parametersResponse.end(), true, "Parameter 1000009 is in response");
			test.Assert(
				parametersResponse.find(2000001) != parametersResponse.end(), true, "Parameter 2000001 is in response");
			test.Assert(
				parametersResponse.find(2000002) != parametersResponse.end(), true, "Parameter 2000002 is in response");

			test.Assert(parametersResponse.size(), 55, "Correct number of parameters in response");
		}
	};

	auto table1{ client->GetParameter41() };
	auto table2{ client->GetParameter42() };
	auto table3{ client->GetParameter43() };
	auto table4{ client->GetParameter44() };

	//* 9) Manager asks parameters from the client
	manager->SendParametersRequest();
	manager->WaitActionsNumber(test, 50000, 3);
	test.Assert(actionsManager, 3, "Correct number of actions on manager side: 3");
	checkParametersResponse(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, {}, 14, {}, 16, {}, 18, {}, 20, {}, 22, {}, 24, {},
		26, {}, 28, 29, 30, {}, 32, {}, "34", "", MSAPI::Timer::Create(2024, 4, 10, 23, 8, 30), 0,
		MSAPI::Timer::Duration::Create(10, 20, 40, 45, 99987653), {}, false, table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client state is not changed after parameters request");

	//* 10) Manager sends run request to the client, parameters are not valid, state is not changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 1);
	const auto& actions = client->GetActionsNumber();
	test.Assert(actions, 1, "Correct number of actions 1");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters in paused state after run request");

	//* 11) Pseudo manager sends metadata and parameters requests to the client, no reaction
	pseudoManager->UseClientConnection();
	pseudoManager->SendMetadataRequest();
	pseudoManager->SendParametersRequest();
	client->WaitUnhandledActions(test, 50000, 4);
	test.Assert(client->GetUnhandledActions(), 4, "Correct number of unhandled actions on client side: 4");

	//* 12) Manager sends pause request to the client, state is not changed
	manager->SendActionPause();
	client->WaitActionsNumber(test, 50000, 2);
	test.Assert(actions, 2, "Correct number of actions 2");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters still in paused state after pause request");

	//* 13) Pseudo manager sends run request to the client, no reaction
	pseudoManager->SendActionRun();
	client->WaitUnhandledActions(test, 50000, 5);
	test.Assert(client->GetUnhandledActions(), 5, "Correct number of unhandled actions on client side: 5");

	//* 14) Manager sends modify request for some of invalid parameters to the client, parameters are changed, state is
	//* not changed
	MSAPI::StandardProtocol::Data parametersData{ MSAPI::StandardProtocol::cipherActionModify };
	parametersData.SetData(3, int32_t{ -78234 });
	parametersData.SetData(27, std::optional<uint64_t>{ 6790004 });
	parametersData.SetData(29, std::optional<float>{ -400.00002 });
	parametersData.SetData(35, std::string{ "Hello, this is parameter update, I'm string. How are you, my dear?!" });
	parametersData.SetData(39, MSAPI::Timer::Duration::CreateMilliseconds(60000));
	manager->SendData(parametersData);
	client->WaitActionsNumber(test, 50000, 4);

	const auto checkAllParameters =
		[&test, &client](const int8_t s1, const int16_t s2, const int32_t s3, const int64_t s4, const uint8_t s5,
			const uint16_t s6, const uint32_t s7, const uint64_t s8, const float s9, const double s10, const double s11,
			const std::optional<int8_t> s12, const std::optional<int8_t> s13, const std::optional<int16_t> s14,
			const std::optional<int16_t> s15, const std::optional<int32_t> s16, const std::optional<int32_t> s17,
			const std::optional<int64_t> s18, const std::optional<int64_t> s19, const std::optional<uint8_t> s20,
			const std::optional<uint8_t> s21, const std::optional<uint16_t> s22, const std::optional<uint16_t> s23,
			const std::optional<uint32_t> s24, const std::optional<uint32_t> s25, const std::optional<uint64_t> s26,
			const std::optional<uint64_t> s27, const std::optional<float> s28, const std::optional<float> s29,
			const std::optional<double> s30, const std::optional<double> s31, const std::optional<double> s32,
			const std::optional<double> s33, const std::string s34, const std::string s35, const MSAPI::Timer s36,
			const MSAPI::Timer s37, const MSAPI::Timer::Duration s38, const MSAPI::Timer::Duration s39, const bool s40,
			const MSAPI::Table<bool, bool, std::string, std::string, std::optional<double>>& s41,
			const MSAPI::Table<size_t, uint64_t>& s42,
			const MSAPI::Table<std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::Timer::Duration,
				MSAPI::Timer::Duration, standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, bool,
				std::string, std::optional<double>, std::optional<double>>& s43,
			const MSAPI::Table<int32_t>& s44) {
			test.Assert(client->GetParameter1(), s1, "Parameter1 has expected value");
			test.Assert(client->GetParameter2(), s2, "Parameter2 has expected value");
			test.Assert(client->GetParameter3(), s3, "Parameter3 has expected value");
			test.Assert(client->GetParameter4(), s4, "Parameter4 has expected value");
			test.Assert(client->GetParameter5(), s5, "Parameter5 has expected value");
			test.Assert(client->GetParameter6(), s6, "Parameter6 has expected value");
			test.Assert(client->GetParameter7(), s7, "Parameter7 has expected value");
			test.Assert(client->GetParameter8(), s8, "Parameter8 has expected value");
			test.Assert(client->GetParameter9(), s9, "Parameter9 has expected value");
			test.Assert(client->GetParameter10(), s10, "Parameter10 has expected value");
			test.Assert(client->GetParameter11(), s11, "Parameter11 has expected value");
			test.Assert(client->GetParameter12(), s12, "Parameter12 has expected value");
			test.Assert(client->GetParameter13(), s13, "Parameter13 has expected value");
			test.Assert(client->GetParameter14(), s14, "Parameter14 has expected value");
			test.Assert(client->GetParameter15(), s15, "Parameter15 has expected value");
			test.Assert(client->GetParameter16(), s16, "Parameter16 has expected value");
			test.Assert(client->GetParameter17(), s17, "Parameter17 has expected value");
			test.Assert(client->GetParameter18(), s18, "Parameter18 has expected value");
			test.Assert(client->GetParameter19(), s19, "Parameter19 has expected value");
			test.Assert(client->GetParameter20(), s20, "Parameter20 has expected value");
			test.Assert(client->GetParameter21(), s21, "Parameter21 has expected value");
			test.Assert(client->GetParameter22(), s22, "Parameter22 has expected value");
			test.Assert(client->GetParameter23(), s23, "Parameter23 has expected value");
			test.Assert(client->GetParameter24(), s24, "Parameter24 has expected value");
			test.Assert(client->GetParameter25(), s25, "Parameter25 has expected value");
			test.Assert(client->GetParameter26(), s26, "Setting26 has expected value");
			test.Assert(client->GetParameter27(), s27, "Parameter27 has expected value");
			test.Assert(client->GetParameter28(), s28, "Parameter28 has expected value");
			test.Assert(client->GetParameter29(), s29, "Parameter29 has expected value");
			test.Assert(client->GetParameter30(), s30, "Parameter30 has expected value");
			test.Assert(client->GetParameter31(), s31, "Parameter31 has expected value");
			test.Assert(client->GetParameter32(), s32, "Parameter32 has expected value");
			test.Assert(client->GetParameter33(), s33, "Parameter33 has expected value");
			test.Assert(client->GetParameter34(), s34, "Parameter34 has expected value");
			test.Assert(client->GetParameter35(), s35, "Parameter35 has expected value");
			test.Assert(client->GetParameter36(), s36, "Parameter36 has expected value");
			test.Assert(client->GetParameter37(), s37, "Parameter37 has expected value");
			test.Assert(client->GetParameter38(), s38, "Parameter38 has expected value");
			test.Assert(client->GetParameter39(), s39, "Parameter39 has expected value");
			test.Assert(client->GetParameter40(), s40, "Parameter40 has expected value");
			test.Assert(client->GetParameter41(), s41, "Parameter41 has expected value");
			test.Assert(client->GetParameter42(), s42, "Parameter42 has expected value");
			test.Assert(client->GetParameter43(), s43, "Parameter43 has expected value");
			test.Assert(client->GetParameter44(), s44, "Parameter44 has expected value");
		};

	test.Assert(actions, 4, "Correct number of actions 4");
	checkAllParameters(1, 2, -78234, 4, 5, 6, 7, 8, 9, 10, 11, 12, {}, 14, {}, 16, {}, 18, {}, 20, {}, 22, {}, 24, {},
		26, 6790004, 28, -400.00002, 30, {}, 32, {}, "34",
		"Hello, this is parameter update, I'm string. How are you, my dear?!",
		MSAPI::Timer::Create(2024, 4, 10, 23, 8, 30), 0, MSAPI::Timer::Duration::Create(10, 20, 40, 45, 99987653),
		MSAPI::Timer::Duration::CreateSeconds(60), false, table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters in paused state after parameters update");

	//* 15) Pseudo manager sends data to the client, no reaction
	pseudoManager->SendData(parametersData);
	client->WaitUnhandledActions(test, 50000, 15);
	test.Assert(client->GetUnhandledActions(), 15, "Correct number of unhandled actions on client side: 15");

	//* 16) Pseudo manager sends data to the second pseudo manager, handled as buffer
	//* Previous reserved data must contain 160 bytes
	pseudoManager->UseOutcomeConnection();
	pseudoManager->SendData(parametersData);
	secondPseudoManager->WaitUnhandledActions(test, 50000, 15);
	test.Assert(secondPseudoManager->GetUnhandledActions(), 15,
		"Correct number of unhandled actions on pseudo manager side: 15");

	//* 17) Pseudo manager sends metadata response to the second pseudo manager, handled as metadata response
	pseudoManager->SendMetadataResponse();
	secondPseudoManager->WaitActionsNumber(test, 50000, 2);
	test.Assert(
		secondPseudoManager->GetActionsNumber(), 2, "Correct number of actions on second pseudo manager side: 2");

	//* 18) Pseudo manager sends parameters response to the second pseudo manager, handled as parameters response
	pseudoManager->SendParametersResponse();
	secondPseudoManager->WaitActionsNumber(test, 50000, 3);
	test.Assert(
		secondPseudoManager->GetActionsNumber(), 3, "Correct number of actions on second pseudo manager side: 3");

	//* 19) Second pseudo manager sends action hello to client connection, handled as hello
	secondPseudoManager->UseClientConnection();
	secondPseudoManager->SendActionHello();
	pseudoManager->WaitActionsNumber(test, 50000, 3);
	test.Assert(pseudoManager->GetActionsNumber(), 3, "Correct number of actions on pseudo manager side: 3");

	//* 20) Manager sends run request to the client, parameters are not valid, state is not changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 5);
	test.Assert(actions, 5, "Correct number of actions 5");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters in paused state after run request");

	//* 21) Manager sends modify request with valid values for all parameters to the client, parameters are changed
	parametersData.Clear();
	parametersData.SetData(1, int8_t{ 11 });
	parametersData.SetData(2, int16_t{ 22 });
	parametersData.SetData(3, int32_t{ 33 });
	parametersData.SetData(4, int64_t{ 44 });
	parametersData.SetData(5, uint8_t{ 55 });
	parametersData.SetData(6, uint16_t{ 66 });
	parametersData.SetData(7, uint32_t{ 77 });
	parametersData.SetData(8, uint64_t{ 88 });
	parametersData.SetData(9, float{ 99 });
	parametersData.SetData(10, double{ 1010 });
	parametersData.SetData(11, static_cast<double>(1111));
	parametersData.SetData(12, std::optional<int8_t>{ 1212 });
	parametersData.SetData(13, std::optional<int8_t>{ 1313 });
	parametersData.SetData(14, std::optional<int16_t>{ 1414 });
	parametersData.SetData(15, std::optional<int16_t>{ 1515 });
	parametersData.SetData(16, std::optional<int32_t>{ 1616 });
	parametersData.SetData(17, std::optional<int32_t>{ 1717 });
	parametersData.SetData(18, std::optional<int64_t>{ 1818 });
	parametersData.SetData(19, std::optional<int64_t>{ 1919 });
	parametersData.SetData(20, std::optional<uint8_t>{ 2020 });
	parametersData.SetData(21, std::optional<uint8_t>{ 2121 });
	parametersData.SetData(22, std::optional<uint16_t>{ 2222 });
	parametersData.SetData(23, std::optional<uint16_t>{ 2323 });
	parametersData.SetData(24, std::optional<uint32_t>{ 2424 });
	parametersData.SetData(25, std::optional<uint32_t>{ 2525 });
	parametersData.SetData(26, std::optional<uint64_t>{ 2626 });
	parametersData.SetData(27, std::optional<uint64_t>{ 2727 });
	parametersData.SetData(28, std::optional<float>{ 2828 });
	parametersData.SetData(29, std::optional<float>{ 300.94 });
	parametersData.SetData(30, std::optional<double>{ 3030 });
	parametersData.SetData(31, std::optional<double>{ 3131 });
	parametersData.SetData(32, std::optional<double>{ 3232 });
	parametersData.SetData(33, std::optional<double>{ 3333 });
	parametersData.SetData(34, std::string{ "3434" });
	parametersData.SetData(35, std::string{ "3535" });
	parametersData.SetData(36, MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10));
	parametersData.SetData(37, MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11));
	parametersData.SetData(38, MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654));
	parametersData.SetData(39, MSAPI::Timer::Duration::CreateSeconds(59));
	parametersData.SetData(40, bool{ true });
	table1.AddRow(true, true, "some string here 123123", "this is Sparta!", std::optional<double>{});
	table1.AddRow(
		false, false, "some string here 123123", "this is Sweden!", std::optional<double>{ -993939.847362948273001L });
	table1.AddRow(true, false, "some string here 123123", "hello from Stockholm!",
		std::optional<double>{ 0.46383745362820000230L });
	parametersData.SetData(41, MSAPI::TableData{ table1 });
	for (size_t index{ 0 }; index < 100; ++index) {
		table2.AddRow(index, index + 19971997);
	}
	parametersData.SetData(42, MSAPI::TableData{ table2 });
	table3.AddRow("", MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59),
		MSAPI::Timer::Duration::CreateMilliseconds(60000), int8_t(-50), int16_t(-10), int32_t(-37483948473),
		int64_t(473939476343), uint8_t(378), uint16_t(0), uint32_t(43234), uint64_t(23482349234234), 0.937363,
		-374823.334004f, false, std::optional<int8_t>(-50), std::optional<int16_t>(-10),
		std::optional<int32_t>(-37483948473), std::optional<int64_t>(473939476343), std::optional<uint8_t>{},
		std::optional<uint16_t>(0), std::optional<uint32_t>(43234), std::optional<uint64_t>(23482349234234),
		std::optional<double>{}, std::optional<float>(-374823.334004f),
		"Hello, this is parameter update, I'm string. How are you, my dear?",
		MSAPI::Timer::Create(2024, 4, 10, 23, 8, 30), MSAPI::Timer::Duration::Create(10, 20, 40, 45, 99987653), true,
		"Hello, this is parameter update, I'm string. How are you, my dear?", std::optional<double>{},
		std::optional<double>{});
	parametersData.SetData(43, MSAPI::TableData{ table3 });
	for (int index{ 0 }; index < 1000; ++index) {
		table4.AddRow(index);
	}
	parametersData.SetData(44, MSAPI::TableData{ table4 });
	manager->SendData(parametersData);
	client->WaitActionsNumber(test, 50000, 6);
	test.Assert(actions, 6, "Correct number of actions 6");
	checkAllParameters(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 1212, 1313, 1414, 1515, 1616, 1717, 1818, 1919,
		2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 300.94, 3030, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with valid parameters in paused state after parameters update");

	//* 22) Manager sends run request to the client, state is changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 7);
	test.Assert(actions, 7, "Correct number of actions 7");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client with valid parameters in running state after run request");

	//* 23) Pseudo manager sends pause, metadata and parameters requests to the client, no reaction
	pseudoManager->UseClientConnection();
	pseudoManager->SendActionPause();
	pseudoManager->SendMetadataRequest();
	pseudoManager->SendParametersRequest();
	client->WaitUnhandledActions(test, 50000, 18);
	test.Assert(client->GetUnhandledActions(), 18, "Correct number of unhandled actions on client side: 18");

	//* 24) Manager asks parameters from the client
	manager->SendParametersRequest();
	manager->WaitActionsNumber(test, 50000, 4);
	test.Assert(actionsManager, 4, "Correct number of actions on manager side: 4");
	checkParametersResponse(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 1212, 1313, 1414, 1515, 1616, 1717, 1818,
		1919, 2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 300.94, 3030, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client state is not changed after parameters request");

	//* 25) Manager asks metadata from the client
	manager->SendMetadataRequest();
	manager->WaitActionsNumber(test, 50000, 5);
	test.Assert(actionsManager, 5, "Correct number of actions on manager side: 5");
	test.Assert(manager->GetMetadata(), metadata, "Metadata is correct");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client state is not changed after metadata request");

	//* 26) Manager sends modify for some parameters to the client and make them invalid, parameters are changed, state
	//* is paused
	parametersData.Clear();
	parametersData.SetData(12, std::optional<int8_t>{});
	parametersData.SetData(29, std::optional<float>{ -5000 });
	parametersData.SetData(30, std::optional<double>{});
	table4.Clear();
	parametersData.SetData(44, MSAPI::TableData{ table4 });
	manager->SendData(parametersData);
	client->WaitActionsNumber(test, 50000, 9);
	test.Assert(actions, 9, "Correct number of actions 9");
	checkAllParameters(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, {}, 1313, 1414, 1515, 1616, 1717, 1818, 1919,
		2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, -5000, {}, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters in paused state after parameters update");

	//* 27) Manager sends run request to the client, state is not changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 10);
	test.Assert(actions, 10, "Correct number of actions 10");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters in paused state after run request");

	//* 28) Manager sends modify with valid values for all parameters to the client, parameters are changed, state is
	//* not changed
	parametersData.Clear();
	parametersData.SetData(12, std::optional<int8_t>{ 0 });
	parametersData.SetData(29, std::optional<float>{ 0 });
	parametersData.SetData(30, std::optional<double>{ 0 });
	table4.AddRow(0);
	parametersData.SetData(44, MSAPI::TableData{ table4 });
	manager->SendData(parametersData);
	client->WaitActionsNumber(test, 50000, 11);
	test.Assert(actions, 11, "Correct number of actions 11");
	checkAllParameters(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 0, 1313, 1414, 1515, 1616, 1717, 1818, 1919,
		2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 0, 0, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with invalid parameters in paused state after parameters update");

	//* 29) Manager sends run request to the client, state is changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 12);
	test.Assert(actions, 12, "Correct number of actions 12");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client with valid parameters in running state after run request");

	//* 30) Manager sends pause request to the client, state is changed
	manager->SendActionPause();
	client->WaitActionsNumber(test, 50000, 13);
	test.Assert(actions, 13, "Correct number of actions 13");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with valid parameters in paused state after pause request");

	//* 31) Pseudo manager sends delete request to the client, no reaction
	pseudoManager->SendActionDelete();
	client->WaitUnhandledActions(test, 50000, 19);
	test.Assert(client->GetUnhandledActions(), 19, "Correct number of unhandled actions on client side: 19");

	//* 32) Manager send modify request for some parameters to the client, parameters are changed, state is not changed
	parametersData.Clear();
	parametersData.SetData(12, std::optional<int8_t>{ 10 });
	parametersData.SetData(29, std::optional<float>{ 10 });
	parametersData.SetData(30, std::optional<double>{ 10 });
	for (size_t index{ 0 }; index < 1000; ++index) {
		table3.AddRow("", MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10),
			MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59),
			MSAPI::Timer::Duration::CreateMilliseconds(60000), int8_t(-50), int16_t(-10), int32_t(-37483948473),
			int64_t(473939476343), uint8_t(378), uint16_t(0), uint32_t(43234), uint64_t(23482349234234), 0.937363,
			-374823.334004f, false, std::optional<int8_t>(-50), std::optional<int16_t>(-10),
			std::optional<int32_t>(-37483948473), std::optional<int64_t>(473939476343), std::optional<uint8_t>{},
			std::optional<uint16_t>(0), std::optional<uint32_t>(43234), std::optional<uint64_t>(23482349234234),
			std::optional<double>{}, std::optional<float>(-374823.334004f),
			"Hello, this is parameter update, I'm string. How are you, my dear?",
			MSAPI::Timer::Create(2024, 4, 10, 23, 8, 30), MSAPI::Timer::Duration::Create(10, 20, 40, 45, 99987653),
			true, "Hello, this is parameter update, I'm string. How are you, my dear?", std::optional<double>{},
			std::optional<double>{});
	}
	parametersData.SetData(43, MSAPI::TableData{ table3 });
	table1.Clear();
	table1.AddRow(true, true, "Hello, motto", "this is Sparta!", std::optional<double>{ 550936483.374823004 });
	parametersData.SetData(41, MSAPI::TableData{ table1 });
	manager->SendData(parametersData);
	client->WaitActionsNumber(test, 200000, 14);
	test.Assert(actions, 14, "Correct number of actions 14");
	checkAllParameters(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 10, 1313, 1414, 1515, 1616, 1717, 1818, 1919,
		2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 10, 10, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with valid parameters still in paused state after parameters update");

	//* 33) Manager sends pause request to the client, state is not changed
	manager->SendActionPause();
	client->WaitActionsNumber(test, 50000, 15);
	test.Assert(actions, 15, "Correct number of actions 15");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with valid parameters still in paused state after pause request");

	//* 34) Manager asks metadata from the client
	manager->SendMetadataRequest();
	manager->WaitActionsNumber(test, 50000, 6);
	test.Assert(actionsManager, 6, "Correct number of actions on manager side: 6");
	test.Assert(manager->GetMetadata(), metadata, "Metadata is correct");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client state is not changed after metadata request");

	//* 35) Manager asks parameters from the client
	manager->SendParametersRequest();
	manager->WaitActionsNumber(test, 50000, 7);
	test.Assert(actionsManager, 7, "Correct number of actions on manager side: 7");
	checkParametersResponse(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 10, 1313, 1414, 1515, 1616, 1717, 1818,
		1919, 2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 10, 10, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client state is not changed after parameters request");

	//* 36) Manager sends run request to the client, state is changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 16);
	test.Assert(actions, 16, "Correct number of actions 16");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client with valid parameters in running state after run request");

	//* 37) Manager sends run request to the client, state is not changed
	manager->SendActionRun();
	client->WaitActionsNumber(test, 50000, 17);
	test.Assert(actions, 17, "Correct number of actions 17");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client with valid parameters still in running state after run request");

	//* 38) Check parameters on client side
	checkAllParameters(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 10, 1313, 1414, 1515, 1616, 1717, 1818, 1919,
		2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 10, 10, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);

	//* 39) Manager asks parameters from the client
	manager->SendParametersRequest();
	manager->WaitActionsNumber(test, 50000, 8);
	test.Assert(actionsManager, 8, "Correct number of actions on manager side: 8");
	checkParametersResponse(11, 22, 33, 44, 55, 66, 77, 88, 99, 1010, 1111, 10, 1313, 1414, 1515, 1616, 1717, 1818,
		1919, 2020, 2121, 2222, 2323, 2424, 2525, 2626, 2727, 2828, 10, 10, 3131, 3232, 3333, "3434", "3535",
		MSAPI::Timer::Create(2024, 6, 7, 8, 9, 10), MSAPI::Timer::Create(2024, 7, 8, 9, 10, 11),
		MSAPI::Timer::Duration::Create(12, 13, 14, 15, 99987654), MSAPI::Timer::Duration::CreateSeconds(59), true,
		table1, table2, table3, table4);
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client state is not changed after parameters request");

	//* 40) Manager asks metadata from the client
	manager->SendMetadataRequest();
	manager->WaitActionsNumber(test, 50000, 9);
	test.Assert(actionsManager, 9, "Correct number of actions on manager side: 9");
	test.Assert(manager->GetMetadata(), metadata, "Metadata is correct");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client state is not changed after metadata request");

	//* 41) Manager stops, HandleDisconnect is called, client is paused
	manager->Stop();
	MSAPI::Test::Wait(
		50000, [&manager]() { return manager->MSAPI::Server::GetState() == MSAPI::Server::State::Stopped; });
	test.Assert(manager->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Manager unexpectedly stopped on application side");
	test.Assert(manager->MSAPI::Server::GetState(), MSAPI::Server::State::Stopped,
		"Manager unexpectedly stopped on server side");
	client->WaitActionsNumber(test, 50000, 18);
	test.Assert(actions, 18, "Correct number of actions 18");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with valid parameters in paused state after manager stopped");

	//* 42) Manager starts, HandleReconnect is called, client is running
	(void)managerPtr->Start(INADDR_LOOPBACK, managerPtr->GetPort());
	manager->WaitActionsNumber(test, 5000000, 10);
	test.Assert(actionsManager, 10, "Correct number of actions on manager side: 10");
	test.Assert(manager->MSAPI::Server::GetState(), MSAPI::Server::State::Running, "Manager restarted successfully");
	test.Assert(manager->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Manager application is still paused");
	client->WaitActionsNumber(test, 50000, 19);
	test.Assert(actions, 19, "Correct number of actions 19");
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Running,
		"Client with valid parameters in running state after reconnect to manager");

	//* 43) Manager sends delete request to the client, state is changed
	manager->SendActionDelete();
	MSAPI::Test::Wait(
		50000, [&client]() { return client->MSAPI::Server::GetState() == MSAPI::Server::State::Stopped; });
	test.Assert(client->MSAPI::Application::GetState(), MSAPI::Application::State::Paused,
		"Client with valid parameters in paused state after delete request");
	test.Assert(client->MSAPI::Server::GetState(), MSAPI::Server::State::Stopped,
		"Client server's state is stopped state after delete request");

	//* 44) Check actions and unhandled actions numbers for all applications

	managerPtr.reset();
	secondPseudoManagerPtr.reset();
	pseudoManagerPtr.reset();
	clientPtr.reset();

	return test.Passed<int32_t>();
}