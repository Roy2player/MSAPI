/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2024-12-03
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
 * @brief Integration test for connection callbacks functionality.
 * Tests HandleOpenConnectionRequest and HandleCloseConnectionRequest callbacks
 * which are triggered by Manager through cipherActionConnect and cipherActionDisconnect.
 */

#include "../../../../library/source/test/daemon.hpp"
#include "client.h"
#include "manager.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	MSAPI::Test test;

//* Create manager
auto managerPtr{ MSAPI::Daemon<Manager>::Create("Manager") };
if (managerPtr == nullptr) {
return 1;
}
auto manager{ static_cast<Manager*>(managerPtr->GetApp()) };
test.Assert(manager->MSAPI::Server::GetState(), MSAPI::Server::State::Running, "Manager is in running state");

//* Create client
auto clientPtr{ MSAPI::Daemon<Client>::Create("Client") };
if (clientPtr == nullptr) {
return 1;
}
auto client{ static_cast<Client*>(clientPtr->GetApp()) };
test.Assert(client->MSAPI::Server::GetState(), MSAPI::Server::State::Running, "Client is in running state");

//* Client connects to manager (id=0 means manager connection)
if (!client->OpenConnect(0, INADDR_LOOPBACK, managerPtr->GetPort())) {
return 1;
}

const size_t& actions{ client->GetActionsNumber() };
const size_t& actionsManager{ manager->GetActionsNumber() };

//* 1) Wait for hello from client to manager
manager->WaitActionsNumber(test, 50000, 1);
test.Assert(actionsManager, 1, "Correct number of actions on manager side: 1 (hello received)");

//* 2) Manager sends connect action to the client with ip, port and needReconnection=true
const int32_t testIp1{ 0x7F000001 };           // 127.0.0.1 in network byte order
const int16_t testPort1{ 8888 };               // Test port
const bool testNeedReconnection1{ true };
manager->SendActionConnect(testIp1, testPort1, testNeedReconnection1);
client->WaitOpenConnectionActions(test, 50000, 1);
test.Assert(client->GetOpenConnectionActions(), 1u, "Correct number of open connection actions: 1");
test.Assert(client->GetLastOpenConnectionIp(), testIp1, "Open connection ip is correct");
test.Assert(client->GetLastOpenConnectionPort(), testPort1, "Open connection port is correct");
test.Assert(client->GetLastOpenConnectionNeedReconnection(), testNeedReconnection1,
"Open connection needReconnection is correct (true)");
client->WaitActionsNumber(test, 50000, 1);
test.Assert(actions, 1, "Correct number of total actions on client: 1");

//* 3) Manager sends second connect action with different parameters and needReconnection=false
const int32_t testIp2{ static_cast<int32_t>(0xC0A80001) };     // 192.168.0.1 in network byte order
const int16_t testPort2{ 9999 };               // Test port
const bool testNeedReconnection2{ false };
manager->SendActionConnect(testIp2, testPort2, testNeedReconnection2);
client->WaitOpenConnectionActions(test, 50000, 2);
test.Assert(client->GetOpenConnectionActions(), 2u, "Correct number of open connection actions: 2");
test.Assert(client->GetLastOpenConnectionIp(), testIp2, "Open connection ip2 is correct");
test.Assert(client->GetLastOpenConnectionPort(), testPort2, "Open connection port2 is correct");
test.Assert(client->GetLastOpenConnectionNeedReconnection(), testNeedReconnection2,
"Open connection needReconnection2 is correct (false)");
client->WaitActionsNumber(test, 50000, 2);
test.Assert(actions, 2, "Correct number of total actions on client: 2");

//* 4) Manager sends disconnect action to the client with first ip and port
manager->SendActionDisconnect(testIp1, testPort1);
client->WaitCloseConnectionActions(test, 50000, 1);
test.Assert(client->GetCloseConnectionActions(), 1u, "Correct number of close connection actions: 1");
test.Assert(client->GetLastCloseConnectionIp(), testIp1, "Close connection ip is correct");
test.Assert(client->GetLastCloseConnectionPort(), testPort1, "Close connection port is correct");
client->WaitActionsNumber(test, 50000, 3);
test.Assert(actions, 3, "Correct number of total actions on client: 3");

//* 5) Manager sends second disconnect action with second ip and port
manager->SendActionDisconnect(testIp2, testPort2);
client->WaitCloseConnectionActions(test, 50000, 2);
test.Assert(client->GetCloseConnectionActions(), 2u, "Correct number of close connection actions: 2");
test.Assert(client->GetLastCloseConnectionIp(), testIp2, "Close connection ip2 is correct");
test.Assert(client->GetLastCloseConnectionPort(), testPort2, "Close connection port2 is correct");
client->WaitActionsNumber(test, 50000, 4);
test.Assert(actions, 4, "Correct number of total actions on client: 4");

//* 6) Test with edge case: port 0
const int32_t testIp3{ 0x0A000001 };           // 10.0.0.1 in network byte order
const int16_t testPort3{ 0 };                  // Edge case: port 0
manager->SendActionConnect(testIp3, testPort3, true);
client->WaitOpenConnectionActions(test, 50000, 3);
test.Assert(client->GetOpenConnectionActions(), 3u, "Correct number of open connection actions: 3");
test.Assert(client->GetLastOpenConnectionIp(), testIp3, "Open connection ip3 is correct");
test.Assert(client->GetLastOpenConnectionPort(), testPort3, "Open connection port3 is correct (0)");
client->WaitActionsNumber(test, 50000, 5);
test.Assert(actions, 5, "Correct number of total actions on client: 5");

//* 7) Test with edge case: negative port (valid as int16_t)
const int16_t testPort4{ -1 };                 // Edge case: max unsigned port as signed
manager->SendActionConnect(testIp1, testPort4, false);
client->WaitOpenConnectionActions(test, 50000, 4);
test.Assert(client->GetOpenConnectionActions(), 4u, "Correct number of open connection actions: 4");
test.Assert(client->GetLastOpenConnectionPort(), testPort4, "Open connection port4 is correct (-1)");
client->WaitActionsNumber(test, 50000, 6);
test.Assert(actions, 6, "Correct number of total actions on client: 6");

//* Cleanup
managerPtr.reset();
clientPtr.reset();

return test.Passed<int32_t>();
}
